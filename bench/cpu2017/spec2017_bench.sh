#!/usr/bin/env bash
#
# run_spec_improved.sh
#
# Improved SPEC CPU2017 benchmark runner with multi-engine support
# Supports native, SDE, and Dr.avx execution engines with structured logging
# Based on the design patterns from llama_bench.sh
#

set -euo pipefail
export LC_ALL=C

# ========= Configuration =========
# Engine paths (relative to script directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDE64="$SCRIPT_DIR/../bin/sde-9.58/sde64"
DRRUN="$SCRIPT_DIR/../bin/dravx/bin64/dravx"

# Default values
BIND_CORES=""          # If empty, no CPU core binding
NUM_RUNS=1             # Default number of runs per benchmark
PARALLEL_MODE="false"  # Default is serial mode
ENGINES=("native")     # Default engine list
SPEC_BASE_DIR="."
USE_RESULTS_DIR="true" # Use ./results/spec directory instead of timestamped dir


# All available SPEC2017 CINT benchmarks
ALL_SPEC_BENCHMARKS=(
    "500.perlbench_r"
    "502.gcc_r" 
    "505.mcf_r"
    "520.omnetpp_r"
    "523.xalancbmk_r"
    "525.x264_r"
    "531.deepsjeng_r"
    "541.leela_r"
    "548.exchange2_r"
    "557.xz_r"
)


# Output directory with timestamp or results directory
if [[ "$USE_RESULTS_DIR" == "true" ]]; then
    OUTPUT_DIR="../results/spec/spec_$(date +%Y%m%d_%H%M%S)"
else
    OUTPUT_DIR="../spec_results_$(date +%Y%m%d_%H%M%S)"
fi
mkdir -p "$OUTPUT_DIR"


# ========= Helper Functions =========
log_message() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*" | tee -a "$OUTPUT_DIR/run.log"
}

usage() {
    echo "Usage: $0 [options] [benchmark_list]"
    echo
    echo "Options:"
    echo "  -h                Show this help message."
    echo "  -c <cores>        Specify CPU core(s) for binding (e.g., '0' or '0,1,2')."
    echo "  -e <engines>      Specify execution engines (comma-separated):"
    echo "                    native,sde,dravx (default: native)"
    echo "  -n <num_runs>     Number of runs per benchmark per engine (default: 1)."
    echo "  -p                Run in parallel mode (default: serial)."
    echo "  -r                Use ./results/spec directory for output."
    echo "  -a                Run all SPEC2017 CINT benchmarks."
    echo "  -o <output_dir>   Custom output directory (default: auto-generated)."
    echo
    echo "Arguments:"
    echo "  benchmark_list    One or more benchmark directories, e.g.:"
    echo "                    500.perlbench_r 502.gcc_r 505.mcf_r"
    echo "                    If no benchmarks specified and -a not used, shows available benchmarks."
    echo
    echo "Examples:"
    echo "  $0 -c 0 -n 3 -e native,sde 500.perlbench_r 502.gcc_r"
    echo "  $0 -p -c 0,1,2 -e native,dravx 500.perlbench_r 502.gcc_r 505.mcf_r"
    echo "  $0 -a -r -e native,sde,dravx -n 3  # Run all benchmarks with all engines"
    echo "  $0 -e sde,dravx 500.perlbench_r"
    exit 1
}

check_prerequisites() {
    local missing=0
    
    if [[ ! -d "$SPEC_BASE_DIR" ]]; then
        log_message "ERROR: SPEC base directory not found: $SPEC_BASE_DIR"
        missing=1
    fi
    
    # Check engine availability
    for engine in "${ENGINES[@]}"; do
        case "$engine" in
            "sde")
                if [[ ! -x "$SDE64" ]]; then
                    log_message "WARNING: SDE not found: $SDE64 (SDE tests will be skipped)"
                fi
                ;;
            "dravx")
                if [[ ! -x "$DRRUN" ]]; then
                    log_message "WARNING: Dr.avx not found: $DRRUN (Dr.avx tests will be skipped)"
                fi
                ;;
            "native")
                # Native always available
                ;;
            *)
                log_message "ERROR: Unknown engine: $engine"
                missing=1
                ;;
        esac
    done
    
    if ! command -v taskset >/dev/null 2>&1; then
        log_message "WARNING: taskset command not found (CPU binding will be disabled)"
    fi
    
    if (( missing > 0 )); then
        log_message "ERROR: Prerequisites check failed. Exiting."
        exit 1
    fi
}

get_cpu_binding_cmd() {
    local core="$1"
    if [[ -n "$core" ]] && command -v taskset >/dev/null 2>&1; then
        echo "taskset -c $core"
    else
        echo ""
    fi
}

needs_quick_rw() {
    local bench_dir="$1"
    for quick_bench in "${QUICK_RW_BENCHMARKS[@]}"; do
        if [[ "$bench_dir" == "$quick_bench" ]]; then
            return 0
        fi
    done
    return 1
}

parse_benchmark_cmd() {
    local bench_dir="$1"
    local cmd_file="$SPEC_BASE_DIR/$bench_dir/cmd.sh"
    
    if [[ ! -f "$cmd_file" ]]; then
        log_message "ERROR: cmd.sh not found in $bench_dir"
        return 1
    fi
    
    if [[ ! -x "$cmd_file" ]]; then
        chmod +x "$cmd_file" 2>/dev/null || {
            log_message "ERROR: Cannot make $cmd_file executable"
            return 1
        }
    fi
    
    # Source cmd.sh to get CMD variable, but avoid executing it
    local cmd_content
    pushd "$SPEC_BASE_DIR/$bench_dir" >/dev/null 2>&1
    
    # Extract CMD variable from the file without executing the script
    cmd_content=$(grep '^CMD=' cmd.sh | head -1 | cut -d'=' -f2- | sed 's/^"//' | sed 's/"$//')
    
    # If that fails, try sourcing but redirect output
    if [[ -z "$cmd_content" ]]; then
        # Create a temporary script that only sets the CMD variable
        local temp_script=$(mktemp)
        grep -E '^(ulimit|export|CMD=)' cmd.sh > "$temp_script"
        source "$temp_script" 2>/dev/null
        cmd_content="$CMD"
        rm -f "$temp_script"
    fi
    
    popd >/dev/null 2>&1
    
    if [[ -z "$cmd_content" ]]; then
        log_message "ERROR: CMD variable not defined in $cmd_file"
        return 1
    fi
    
    echo "$cmd_content"
}

run_single_benchmark() {
    local engine="$1"
    local bench_dir="$2"
    local core="$3"
    local iteration="$4"
    local output_file="$5"
    
    log_message "RUN: $engine $bench_dir iteration=$iteration core=$core"
    
    # Get base command
    local base_cmd
    base_cmd="$(parse_benchmark_cmd "$bench_dir")"
    if [[ $? -ne 0 ]]; then
        log_message "FAILED: Cannot parse command for $bench_dir"
        return 1
    fi
    
    # Get CPU binding command
    local cpu_bind_cmd
    cpu_bind_cmd="$(get_cpu_binding_cmd "$core")"
    
    # Build full command based on engine
    local full_cmd=""
    case "$engine" in
        "native")
            full_cmd="$cpu_bind_cmd $base_cmd"
            ;;
        "sde")
            if [[ ! -x "$SDE64" ]]; then
                log_message "SKIP: SDE not available for $engine $bench_dir"
                return 0
            fi
            full_cmd="$cpu_bind_cmd \"$SDE64\" -avx512 1 -- $base_cmd"
            ;;
        "dravx")
            if [[ ! -x "$DRRUN" ]]; then
                log_message "SKIP: Dr.avx not available for $engine $bench_dir"
                return 0
            else
                full_cmd="$cpu_bind_cmd \"$DRRUN\" -quiet -- $base_cmd"
            fi
            ;;
        *)
            log_message "ERROR: Unknown engine: $engine"
            return 1
            ;;
    esac
    
    # Create CSV header if file doesn't exist
    if [[ ! -f "$output_file" ]]; then
        echo "engine,benchmark,iteration,core,timestamp,elapsed_time,return_code,command,output_summary" > "$output_file"
    fi
    
    # Change to benchmark directory
    pushd "$SPEC_BASE_DIR/$bench_dir" >/dev/null 2>&1
    
    # Run the command and capture output
    local start_time
    start_time="$(date -Iseconds)"
    local start_epoch=$(date +%s.%N)
    local cmd_output=""
    local exit_code=0
    
    # Execute command with output redirected to /dev/null
    # We only need timing information, not the actual output
    if eval "$full_cmd" >/dev/null 2>&1; then
        exit_code=0
        cmd_output="Output redirected to /dev/null"
    else
        exit_code=$?
        cmd_output="Command failed with exit code $exit_code"
    fi
    
    local end_epoch=$(date +%s.%N)
    local elapsed=$(awk "BEGIN {print $end_epoch - $start_epoch}")
    
    popd >/dev/null 2>&1
    
    # Clean output for CSV (remove newlines and quotes, limit length)
    local clean_output
    clean_output="$(echo "$cmd_output" | tr '\n' ' ' | tr '"' "'" | head -c 200)"
    
    # Append result to CSV
    echo "\"$engine\",\"$bench_dir\",$iteration,\"$core\",\"$start_time\",$elapsed,$exit_code,\"$full_cmd\",\"$clean_output\"" >> "$output_file"
    
    if (( exit_code == 0 )); then
        log_message "SUCCESS: $engine $bench_dir iteration=$iteration "
    else
        log_message "FAILED: $engine $bench_dir iteration=$iteration (exit code: $exit_code, ${elapsed}s)"
    fi
    
    return $exit_code
}

run_benchmark_all_engines() {
    local bench_dir="$1"
    local core="$2"
    local output_file="$3"
    
    log_message "=== Starting benchmark: $bench_dir ==="
    
    # Check if benchmark directory exists
    if [[ ! -d "$SPEC_BASE_DIR/$bench_dir" ]]; then
        log_message "ERROR: Benchmark directory not found: $SPEC_BASE_DIR/$bench_dir"
        return 1
    fi
    
    local total_runs=0
    local failed_runs=0
    
    # Run with each engine
    for engine in "${ENGINES[@]}"; do
        log_message "--- Running $bench_dir with $engine engine ---"
        
        # Run multiple iterations
        for ((i=1; i<=NUM_RUNS; i++)); do
            if ! run_single_benchmark "$engine" "$bench_dir" "$core" "$i" "$output_file"; then
                failed_runs=$((failed_runs + 1))
            fi
            total_runs=$((total_runs + 1))
        done
    done
    
    log_message "=== Completed benchmark: $bench_dir ($((total_runs - failed_runs))/$total_runs successful) ==="
    return 0
}

# ========= Main Test Execution =========
main() {
    log_message "Starting SPEC CPU2017 benchmark suite"
    log_message "Output directory: $OUTPUT_DIR"
    log_message "Engines: ${ENGINES[*]}"
    log_message "Runs per benchmark per engine: $NUM_RUNS"
    log_message "Parallel mode: $PARALLEL_MODE"
    
    check_prerequisites
    
    # System info
    log_message "System info:"
    log_message "CPU: $(nproc) cores"
    if command -v lscpu >/dev/null 2>&1; then
        log_message "$(lscpu | grep 'Model name' || echo 'CPU info not available')"
    fi
    
    local results_csv="$OUTPUT_DIR/results.csv"
    
    if [[ "$PARALLEL_MODE" == "true" ]]; then
        log_message "=== Running in parallel mode ==="
        
        # Parse cores for parallel assignment
        IFS=',' read -ra CORE_ARRAY <<< "$BIND_CORES"
        local num_cores=${#CORE_ARRAY[@]}
        declare -a pids=()
        local idx=0
        
        for bench in "${BENCHMARKS[@]}"; do
            local assigned_core=""
            if [[ -n "$BIND_CORES" ]] && (( num_cores > 0 )); then
                assigned_core="${CORE_ARRAY[$((idx % num_cores))]}"
            fi
            
            run_benchmark_all_engines "$bench" "$assigned_core" "$results_csv" &
            pids+=($!)
            ((idx++))
        done
        
        # Wait for all parallel tasks
        for pid in "${pids[@]}"; do
            wait "$pid"
        done
        
        log_message "=== All parallel benchmarks completed ==="
    else
        log_message "=== Running in serial mode ==="
        
        # Use first core for all benchmarks in serial mode
        local assigned_core=""
        if [[ -n "$BIND_CORES" ]]; then
            IFS=',' read -ra CORE_ARRAY <<< "$BIND_CORES"
            assigned_core="${CORE_ARRAY[0]}"
        fi
        
        for bench in "${BENCHMARKS[@]}"; do
            run_benchmark_all_engines "$bench" "$assigned_core" "$results_csv"
        done
        
        log_message "=== All serial benchmarks completed ==="
    fi
    
    # Summary
    log_message "=== Test Suite Complete ==="
    log_message "Results saved to:"
    log_message "  - Results CSV: $results_csv"
    log_message "  - Full log: $OUTPUT_DIR/run.log"
    
    if [[ -f "$results_csv" ]]; then
        local total_runs
        total_runs=$(($(wc -l < "$results_csv") - 1))  # Subtract header
        log_message "Total benchmark runs completed: $total_runs"
        
        # Count successful runs
        local successful_runs
        successful_runs=$(awk -F',' 'NR>1 && $7==0 {count++} END {print count+0}' "$results_csv")
        log_message "Successful runs: $successful_runs/$total_runs"
    fi
}

# ========= Command Line Parsing =========
RUN_ALL_BENCHMARKS="false"

# Parse command-line options
while getopts "hc:e:n:prao:" opt; do
    case "$opt" in
        h) usage ;;
        c) BIND_CORES="$OPTARG" ;;
        e) IFS=',' read -ra ENGINES <<< "$OPTARG" ;;
        n) NUM_RUNS="$OPTARG" ;;
        p) PARALLEL_MODE="true" ;;
        r) USE_RESULTS_DIR="true" ;;
        a) RUN_ALL_BENCHMARKS="true" ;;
        o) OUTPUT_DIR="$OPTARG"; mkdir -p "$OUTPUT_DIR" ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

# Update output directory if results dir was requested
if [[ "$USE_RESULTS_DIR" == "true" ]] && [[ "$OUTPUT_DIR" == ../spec_results_* ]]; then
    OUTPUT_DIR="../results/spec/spec_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$OUTPUT_DIR"
fi

# Determine benchmark list
if [[ "$RUN_ALL_BENCHMARKS" == "true" ]]; then
    BENCHMARKS=("${ALL_SPEC_BENCHMARKS[@]}")
    echo "Running all SPEC2017 CINT benchmarks: ${BENCHMARKS[*]}"
else
    # Remaining arguments are benchmark directories
    BENCHMARKS=("$@")
    
    if [[ ${#BENCHMARKS[@]} -eq 0 ]]; then
        echo "Available SPEC2017 CINT benchmarks:"
        for bench in "${ALL_SPEC_BENCHMARKS[@]}"; do
            echo "  $bench"
        done
        echo
        echo "Use -a to run all benchmarks, or specify individual benchmarks."
        echo "Use -h for help."
        exit 1
    fi
fi

# Validate NUM_RUNS
if ! [[ "$NUM_RUNS" =~ ^[0-9]+$ ]] || (( NUM_RUNS < 1 )); then
    echo "Error: NUM_RUNS must be a positive integer."
    exit 1
fi

# ========= Script Entry Point =========
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi

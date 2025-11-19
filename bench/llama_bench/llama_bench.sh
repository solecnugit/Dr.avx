#!/usr/bin/env bash
# llama-bench script for specific testing scenarios
# Runs pp (prompt processing) and tg (token generation) tests with different engines
# For single-thread tests, binds to cores 0-8 using taskset

set -euo pipefail
export LC_ALL=C

# ========= Configuration =========
MODEL_PATH="../real-workloads/llama/models/llama2_xs_460m_experimental.q8_0.gguf"
LLAMA_BENCH="../real-workloads/llama/bin/llama-bench"
SDE64="../bin/sde-9.58/sde64"
DRRUN="../bin/dravx/bin64/dravx"

# Test parameters
THREADS=1
BATCH=512
REPS=${REPS:-5}  # Can be overridden via environment variable, default is 5
SEED=1

# Test configurations
PP_PROMPTS=(64 128 256 512)  # prompt processing
TG_NGENS=(64 128 256 512)    # token generation

# Output directory
OUTPUT_DIR="results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

# ========= Helper Functions =========
log_message() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $*" | tee -a "$OUTPUT_DIR/run.log"
}

check_prerequisites() {
    local missing=0
    
    if [[ ! -f "$MODEL_PATH" ]]; then
        log_message "ERROR: Model file not found: $MODEL_PATH"
        missing=1
    fi
    
    if [[ ! -x "$LLAMA_BENCH" ]]; then
        log_message "ERROR: llama-bench executable not found: $LLAMA_BENCH"
        missing=1
    fi
    
    if [[ ! -x "$SDE64" ]]; then
        log_message "WARNING: SDE not found: $SDE64 (SDE tests will be skipped)"
    fi
    
    if [[ ! -x "$DRRUN" ]]; then
        log_message "WARNING: DynamoRIO not found: $DRRUN (DynamoRIO tests will be skipped)"
    fi
    
    if ! command -v taskset >/dev/null 2>&1; then
        log_message "ERROR: taskset command not found (required for CPU binding)"
        missing=1
    fi
    
    if (( missing > 0 )); then
        log_message "ERROR: Prerequisites check failed. Exiting."
        exit 1
    fi
}

get_cpu_binding_cmd() {
    local threads="$1"
    if (( threads == 1 )); then
        echo ""
    else
        echo ""  # No special binding for multi-thread tests
    fi
}

run_benchmark() {
    local engine="$1"
    local mode="$2"     # "pp" or "tg"
    local p_val="$3"
    local n_val="$4"
    local output_file="$5"
    
    local cpu_bind_cmd
    cpu_bind_cmd="$(get_cpu_binding_cmd "$THREADS")"
    
    local base_cmd="\"$LLAMA_BENCH\" -m \"$MODEL_PATH\" -p $p_val -n $n_val -t $THREADS -b $BATCH -ngl 0 -r $REPS"
    local full_cmd=""
    
    case "$engine" in
        "native")
            full_cmd="$cpu_bind_cmd $base_cmd"
            ;;
        "sde")
            if [[ ! -x "$SDE64" ]]; then
                log_message "SKIP: SDE not available for $engine $mode p=$p_val n=$n_val"
                return 0
            fi
            full_cmd="$cpu_bind_cmd \"$SDE64\" -avx512 1 -- $base_cmd"
            ;;
        "dravx")
            if [[ ! -x "$DRRUN" ]]; then
                log_message "SKIP: Dr.avx not available for $engine $mode p=$p_val n=$n_val"
                return 0
            fi
            full_cmd="$cpu_bind_cmd \"$DRRUN\" -quiet -- $base_cmd"
            ;;
        *)
            log_message "ERROR: Unknown engine: $engine"
            return 1
            ;;
    esac
    
    log_message "RUN: $engine $mode p=$p_val n=$n_val threads=$THREADS"
    log_message "CMD: $full_cmd"
    
    # Create CSV header if file doesn't exist
    if [[ ! -f "$output_file" ]]; then
        echo "engine,mode,prompt_tokens,generation_tokens,threads,batch,reps,timestamp,command,output" > "$output_file"
    fi
    
    # Run the command and capture output
    local start_time
    start_time="$(date -Iseconds)"
    local cmd_output=""
    local exit_code=0
    
    # Execute command and capture both stdout and stderr
    if cmd_output=$(eval "$full_cmd" 2>&1); then
        exit_code=0
    else
        exit_code=$?
    fi
    
    # Extract relevant performance data from output
    local performance_line=""
    if echo "$cmd_output" | grep -q "llama_print_timings"; then
        performance_line=$(echo "$cmd_output" | grep -E "(eval time|prompt eval time|sample time)" | tr '\n' '; ')
    fi
    
    # Clean output for CSV (remove newlines and quotes)
    local clean_output
    clean_output="$(echo "$cmd_output" | tr '\n' ' ' | tr '"' "'" | head -c 500)"
    
    # Append result to CSV
    echo "\"$engine\",\"$mode\",$p_val,$n_val,$THREADS,$BATCH,$REPS,\"$start_time\",\"$full_cmd\",\"$clean_output\"" >> "$output_file"
    
    if (( exit_code == 0 )); then
        log_message "SUCCESS: $engine $mode p=$p_val n=$n_val"
        if [[ -n "$performance_line" ]]; then
            log_message "PERF: $performance_line"
        fi
    else
        log_message "FAILED: $engine $mode p=$p_val n=$n_val (exit code: $exit_code)"
    fi
    
    echo ""
    return $exit_code
}

# ========= Main Test Execution =========
main() {
    log_message "Starting llama-bench test suite"
    log_message "Output directory: $OUTPUT_DIR"
    log_message "Model: $MODEL_PATH"
    log_message "Configuration: Threads=$THREADS, Batch=$BATCH, Reps=$REPS"
    
    check_prerequisites
    
    # Test system info
    log_message "System info:"
    log_message "CPU: $(nproc) cores"
    if command -v lscpu >/dev/null 2>&1; then
        log_message "$(lscpu | grep 'Model name' || echo 'CPU info not available')"
    fi
    
    local pp_csv="$OUTPUT_DIR/results_pp.csv"
    local tg_csv="$OUTPUT_DIR/results_tg.csv"
    
    # Run prompt processing tests (pp mode)
    log_message "=== Starting Prompt Processing (PP) Tests ==="
    for prompt in "${PP_PROMPTS[@]}"; do
        for engine in "native" "sde" "dravx"; do
            run_benchmark "$engine" "pp" "$prompt" "0" "$pp_csv"
        done
    done
    
    # Run token generation tests (tg mode)
    log_message "=== Starting Token Generation (TG) Tests ==="
    for ngen in "${TG_NGENS[@]}"; do
        for engine in "native" "sde" "dravx"; do
            run_benchmark "$engine" "tg" "0" "$ngen" "$tg_csv"
        done
    done
    
    log_message "=== Test Suite Complete ==="
    log_message "Results saved to:"
    log_message "  - PP results: $pp_csv"
    log_message "  - TG results: $tg_csv"
    log_message "  - Full log: $OUTPUT_DIR/run.log"
    
    # Summary
    if [[ -f "$pp_csv" ]]; then
        local pp_count
        pp_count=$(($(wc -l < "$pp_csv") - 1))  # Subtract header
        log_message "PP tests completed: $pp_count runs"
    fi
    
    if [[ -f "$tg_csv" ]]; then
        local tg_count
        tg_count=$(($(wc -l < "$tg_csv") - 1))  # Subtract header
        log_message "TG tests completed: $tg_count runs"
    fi
}

# ========= Script Entry Point =========
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi

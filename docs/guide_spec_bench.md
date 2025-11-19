# SPEC2017 CINT Benchmark Analysis System

This system provides a complete workflow for running SPEC2017 CINT benchmarks with multiple execution engines and generating overhead analysis plots.

## Components

### 1. Enhanced Benchmark Runner (`run_spec_improved.sh`)

An improved version of the original `run_spec.sh` with the following features:

- **Multi-engine support**: Native, Intel SDE, and Dr.avx execution
- **Structured logging**: Timestamped logs with detailed progress tracking
- **Flexible configuration**: CPU binding, parallel/serial execution, multiple runs
- **Results management**: Organized output in CSV format with detailed metadata

#### Usage Examples

```bash
# Run a single benchmark with all engines
./run_spec_improved.sh -c 0 -e native,sde,dravx -n 3 500.perlbench_r

# Run all SPEC2017 CINT benchmarks (takes several hours)
./run_spec_improved.sh -a -r -c 0 -e native,sde,dravx -n 3

# Run specific benchmarks in parallel mode
./run_spec_improved.sh -p -c 0,1,2 -e native,dravx 500.perlbench_r 502.gcc_r 505.mcf_r

# Show available benchmarks
./run_spec_improved.sh
```

#### Command Line Options

- `-h`: Show help message
- `-c <cores>`: CPU core binding (e.g., '0' or '0,1,2')
- `-e <engines>`: Execution engines (native,sde,dravx)
- `-n <runs>`: Number of runs per benchmark per engine
- `-p`: Enable parallel execution mode
- `-r`: Use `./results/spec/` directory for output
- `-a`: Run all SPEC2017 CINT benchmarks
- `-o <dir>`: Custom output directory

### 2. Overhead Analysis Script (`scripts/spec2017_cint_overhead.py`)

Python script that processes benchmark results and generates overhead comparison plots.

#### Features

- **Automatic data loading**: Finds and processes the latest results
- **Statistical analysis**: Calculates means, standard deviations, and geometric means
- **Professional plotting**: Generates publication-quality figures
- **Detailed reporting**: Comprehensive overhead analysis summary

#### Usage Examples

```bash
# Auto-detect latest results and generate plots
python3 scripts/spec2017_cint_overhead.py

# Use specific results directory
python3 scripts/spec2017_cint_overhead.py -d results/spec/spec_20251030_114934

# Custom output directory
python3 scripts/spec2017_cint_overhead.py -o ./custom_figs/
```

### 3. Complete Analysis Workflow (`run_full_spec_analysis.sh`)

Automated script that runs the complete analysis pipeline:

1. Executes all SPEC2017 CINT benchmarks with all engines
2. Generates overhead analysis plots
3. Provides summary of results

```bash
./run_full_spec_analysis.sh
```

## Available SPEC2017 CINT Benchmarks

The system supports all SPEC2017 CINT benchmarks:

- `500.perlbench_r` (perl)
- `502.gcc_r` (gcc)
- `505.mcf_r` (mcf)
- `520.omnetpp_r` (omnetpp)
- `523.xalancbmk_r` (xalancbmk)
- `525.x264_r` (x264)
- `531.deepsjeng_r` (deepsjeng)
- `541.leela_r` (leela)
- `548.exchange2_r` (exchange2)
- `557.xz_r` (xz)

## Execution Engines

### Native
Direct execution of benchmarks without instrumentation.

### Intel SDE (Software Development Emulator)
- Path: `./bin/sde-9.58/sde64`
- Flags: `-avx512 1 -sync_avx512_state 1`
- Purpose: AVX-512 instruction emulation and analysis

### Dr.avx (DynamoRIO-based tool)
- Path: `./bin/dravx/bin64/dravx`
- Purpose: Dynamic binary instrumentation for AVX analysis

## Output Structure

### Results Directory Structure
```
./results/spec/
└── spec_YYYYMMDD_HHMMSS/
    ├── results.csv          # Detailed benchmark results
    └── run.log             # Complete execution log
```

### CSV Format
The results CSV contains the following columns:
- `engine`: Execution engine (native/sde/dravx)
- `benchmark`: Benchmark name (e.g., 500.perlbench_r)
- `iteration`: Run iteration number
- `core`: CPU core used
- `timestamp`: Execution start time
- `elapsed_time`: Execution time in seconds
- `return_code`: Exit code (0 = success)
- `command`: Full command executed
- `output_summary`: Brief output summary

### Generated Plots
- `./figs/fig9.png`: Overhead comparison plot (PNG format)
- `./figs/fig9.pdf`: Overhead comparison plot (PDF format)

## Prerequisites

### System Requirements
- Linux system with taskset support
- Python 3.6+ with matplotlib, pandas, numpy
- Intel SDE installed in `./bin/sde-9.58/`
- Dr.avx installed in `./bin/dravx/bin64/`
- SPEC2017 benchmarks compiled in `./cpu2017/`

### Python Dependencies
```bash
pip install matplotlib pandas numpy
```

## Troubleshooting

### Common Issues

1. **Missing executables**: Ensure SDE and Dr.avx are properly installed
2. **Permission errors**: Check that benchmark executables are executable
3. **Memory issues**: Some benchmarks require significant memory
4. **Timeout issues**: Long-running benchmarks may need increased timeouts

### Debug Mode
Add debug output by modifying the log level in the scripts or using verbose flags.

## Integration with Existing Workflow

This system is designed to integrate with the existing DR-avx evaluation framework:
- Results are stored in the standard `./results/` directory
- CSV format is compatible with existing analysis tools
- Plotting follows the established academic style guidelines
- Log format matches existing benchmark runners

## Future Enhancements

Potential improvements for future versions:
- Support for SPEC2017 FP benchmarks
- Additional statistical analysis options
- Interactive plot generation
- Automated report generation
- Integration with continuous integration systems

# LLaMA Benchmark Automated Testing Suite

This directory contains an automated workflow for benchmarking LLaMA inference performance across different execution engines (native, Intel SDE, Dr.avx).

## 📁 Files Overview

- **`run_llama_bench.py`** - Main automation script (orchestrates the entire workflow)
- **`llama_bench.sh`** - Bash script that runs the actual benchmarks
- **`process_results.py`** - Python script to process raw results into clean CSV format
- **`plot_tps.py`** - Python script to generate performance comparison plots

## 🚀 Quick Start

### Run Complete Workflow
Run benchmarks, process results, and generate plots in one command:

```bash
./run_llama_bench.py
```

Or with Python explicitly:

```bash
python3 run_llama_bench.py
```

### Adjust Test Repetitions
Control the number of repetitions for more stable results or faster testing:

```bash
# Quick test with fewer repetitions (faster)
./run_llama_bench.py --reps 3

# Thorough test with more repetitions (more stable, slower)
./run_llama_bench.py --reps 10
```

Default is 5 repetitions. Higher values provide more accurate variance measurements but take longer to complete.

### Use Existing Results
If you already have benchmark results and want to regenerate plots:

```bash
# Use the latest results directory automatically
./run_llama_bench.py --skip-benchmark --auto-find

# Or specify a specific results directory
./run_llama_bench.py --skip-benchmark --results results_20251030_091358
```

### Run Only Benchmarks
If you only want to collect data without processing:

```bash
./run_llama_bench.py --benchmark-only
```

## 📊 Output Structure

After running the complete workflow, you'll have:

```
llama_bench/
├── results_YYYYMMDD_HHMMSS/      # Benchmark results directory
│   ├── results_pp.csv            # Raw PP (Prefill) results
│   ├── results_tg.csv            # Raw TG (Token Generation) results
│   ├── results_pp_clean.csv      # Processed PP results
│   ├── results_tg_clean.csv      # Processed TG results
│   └── run.log                   # Benchmark execution log
│
└── figs/                         # Generated visualizations
    ├── llama_performance_comparison.pdf
    └── llama_performance_comparison.png
```

## 🎨 Features

### Colorful CLI Output
The automation script provides clear, colorful terminal output:
- ✓ Green for success messages
- ✗ Red for errors
- ⚠ Yellow for warnings
- • Blue for information
- ⟳ Yellow for running status

### Progress Tracking
See exactly which phase is running:
```
[1/3] Running Benchmarks
[2/3] Processing Results
[3/3] Generating Visualizations
```

### Automatic Error Handling
The script checks prerequisites, handles errors gracefully, and provides clear error messages.

## 🔧 Configuration

### Benchmark Parameters
Edit `llama_bench.sh` to customize:
- Test configurations (PP/TG lengths)
- Number of repetitions
- Thread count
- Batch size
- CPU binding

### Plot Appearance
Edit `plot_tps.py` to customize:
- Colors and styling
- Font sizes
- Figure dimensions
- Chart annotations

## 📝 Command-Line Options

```bash
usage: run_llama_bench.py [-h] [--skip-benchmark] [--benchmark-only]
                          [--results RESULTS] [--auto-find] [--reps N]

Automated LLaMA Benchmark Testing Suite

optional arguments:
  -h, --help         show this help message and exit
  --skip-benchmark   Skip running benchmarks, use existing results
  --benchmark-only   Only run benchmarks, skip processing and plotting
  --results RESULTS  Specify results directory to process
  --auto-find        Automatically find and use the latest results directory
  --reps N           Number of repetitions for each test (default: 5)
```

## 📋 Examples

### Example 1: First Time Run
```bash
# Run everything from scratch
./run_llama_bench.py
```

Output:
```
===========================================================================
          LLaMA Benchmark Automated Testing Suite
===========================================================================

▶ Checking Prerequisites
──────────────────────────────────────────────────────────────────────────
✓ Found llama_bench.sh: /path/to/llama_bench.sh
✓ Found process_results.py: /path/to/process_results.py
✓ Found plot_tps.py: /path/to/plot_tps.py
✓ Found bash shell

[1/3] Running Benchmarks
...
```

### Example 2: Regenerate Plots Only
```bash
# If you modified plot_tps.py and want to regenerate figures
./run_llama_bench.py --skip-benchmark --auto-find
```

### Example 3: Process Specific Results
```bash
# Process a specific results directory
./run_llama_bench.py --skip-benchmark --results results_20251030_091358
```

### Example 4: Collect Data Only
```bash
# Run benchmarks but don't process or plot yet
./run_llama_bench.py --benchmark-only
```

### Example 5: Quick Test with Fewer Repetitions
```bash
# Fast testing during development (3 reps instead of default 5)
./run_llama_bench.py --reps 3
```

### Example 6: Thorough Test with More Repetitions
```bash
# Production testing with more stable results (10 reps)
./run_llama_bench.py --reps 10
```

## 🔍 Workflow Details

### Phase 1: Running Benchmarks
- Executes `llama_bench.sh`
- Tests multiple PP lengths: 64, 128, 256, 512
- Tests multiple TG lengths: 64, 128, 256, 512
- Tests three engines: native, SDE, Dr.avx
- Creates timestamped results directory

### Phase 2: Processing Results
- Parses raw CSV output from benchmarks
- Extracts performance metrics (tokens/s, variance)
- Creates clean CSV files for plotting
- Generates summary statistics

### Phase 3: Generating Visualizations
- Creates side-by-side comparison plots
- Shows PP (Prefill) and TG (Token Generation) performance
- Normalizes to native baseline
- Saves both PDF and PNG formats

## 🛠️ Troubleshooting

### "Prerequisites check failed"
Make sure all required files exist:
- `llama_bench.sh`
- `process_results.py`
- `plot_tps.py`

### "No results directory found"
If using `--skip-benchmark --auto-find`, ensure you have at least one `results_*` directory.

### Plot Generation Fails
Check that your results directory contains:
- `results_pp_clean.csv`
- `results_tg_clean.csv`

These are created by the processing phase.

### Engine Name Mismatch
If you see "No data found for X engine", check that the engine names in your CSV files match what the plot script expects:
- `native`
- `sde`
- `dravx`

## 📦 Dependencies

- Python 3.6+
- Bash shell
- Required Python packages:
  - pandas
  - numpy
  - matplotlib
  
Install Python dependencies:
```bash
pip install pandas numpy matplotlib
```

## 🤝 Contributing

When modifying the workflow:
1. Test each script individually first
2. Ensure error handling is robust
3. Update this README with any new options
4. Keep the colorful CLI output consistent

## 📄 License

Part of the DR-avx evaluation suite.


# MongoDB Benchmark Suite

This guide describes the MongoDB benchmark system using YCSB to evaluate NoSQL database performance under different execution environments: native, DR-AVX, and Intel SDE.

## Overview

The MongoDB benchmark suite uses **YCSB** (Yahoo! Cloud Serving Benchmark) to measure database performance across three workload patterns. The system automatically runs benchmarks, collects metrics, and generates visualizations.

## Components

### 1. Main Benchmark Script (`mongodb_bench/mongodb_ycsb.sh`)

The main script orchestrates all benchmark executions with colorized output.

#### Features

- **Three execution modes**: Native (baseline), DR-AVX, Intel SDE (AVX-512 emulation)
- **Three YCSB workloads**: Workload B (read-update), Workload C (read-only), Workload D (read-insert)
- **Automated workflow**: Database initialization, data loading, benchmark execution, result collection, and visualization
- **Environment variable configuration**: Flexible parameter adjustment
- **Colorized output**: Enhanced terminal visibility with visual indicators

### 2. YCSB Workloads

#### Workload B - Read/Update Heavy (`workloadb`)
- **Operations**: 95% read, 5% update
- **Use case**: Session management, user profile cache

#### Workload C - Read Only (`workloadc`)
- **Operations**: 100% read
- **Use case**: Content delivery, read-only analytics

#### Workload D - Read Latest/Insert (`workloadd`)
- **Operations**: 95% read latest, 5% insert
- **Use case**: Social media feeds, time-series data

## Configuration

### Default Parameters

```bash
MongoDB_THREADS=16          # Number of concurrent threads
MongoDB_COUNT=7200000       # Number of operations per workload
MongoDB_SCENARIO=("workloadb" "workloadc" "workloadd")
```

### YCSB Test Configuration

- **Record count**: 100,000 records
- **Operation count**: 7,200,000 operations (configurable)
- **Database**: ycsb (MongoDB port 27017)
- **Write concern**: w=0 (maximum performance)

### Environment Variable Overrides

```bash
# Run with 32 threads and 10 million operations
MongoDB_THREADS=32 MongoDB_COUNT=10000000 ./mongodb_ycsb.sh

# Quick test with 8 threads and 1 million operations
MongoDB_THREADS=8 MongoDB_COUNT=1000000 ./mongodb_ycsb.sh
```

## Usage

### Running the Benchmark

```bash
# Navigate to the benchmark directory
cd /eval/mongodb_bench

# Run with default settings
./mongodb_ycsb.sh

# Run with custom parameters
MongoDB_THREADS=32 MongoDB_COUNT=10000000 ./mongodb_ycsb.sh
```

### Expected Duration

With default parameters (16 threads, 7.2M operations):

- **Native**: ~30-40 minutes
- **DR-AVX**: ~35-50 minutes
- **SDE**: ~60-90 minutes (+ 8 min initialization)
- **Total**: ~2-3 hours

## Execution Engines

### 1. Native (Baseline)

```bash
/eval/real-workloads/mongodb/bin/mongod \
  --dbpath /eval/real-workloads/mongodb/data \
  --logpath /eval/real-workloads/mongodb/mongodb.log
```

- No instrumentation overhead
- Initialization: ~10 seconds

### 2. DR-AVX (Quick R/W Mode)

```bash
/eval/bin/dravx/bin64/dravx -quick_rw 1 -- [MongoDB command]
```

- Dynamic binary instrumentation
- Overhead: 15-30%
- Initialization: ~10 seconds
- **Visual**: 🔥 Magenta/purple output

### 3. Intel SDE (AVX-512 Emulation)

```bash
/eval/bin/sde-9.58/sde64 -icl -avx512 1 -- [MongoDB command]
```

- Software emulation of AVX-512
- Overhead: 50-150%
- Initialization: ~480 seconds (8 minutes)
- **Visual**: ⚡ Blue output with warning

## Output Structure

```
/eval/results/mongodb/
└── mongodb_raw_data/
    ├── origin_mongodb_workloadb.txt
    ├── origin_mongodb_workloadc.txt
    ├── origin_mongodb_workloadd.txt
    ├── dravx_mongodb_workloadb.txt
    ├── dravx_mongodb_workloadc.txt
    ├── dravx_mongodb_workloadd.txt
    ├── sde_mongodb_workloadb.txt
    ├── sde_mongodb_workloadc.txt
    ├── sde_mongodb_workloadd.txt
    └── [chart files]
```

### Result Metrics

- **Throughput (ops/sec)**: Primary performance metric
- **Latency**: Average, min, max, 95th/99th percentile
- **Per-operation latency**: Read, Update, Insert statistics

## Terminal Output

### Color Scheme

- 🟢 **Green**: Native/baseline execution
- 🟣 **Magenta**: DR-AVX execution (highlighted)
- 🔵 **Blue**: Intel SDE execution (highlighted)
- 🟡 **Yellow**: Warnings and wait times
- 🟦 **Cyan**: Section headers

### Key Visual Indicators

- 🚀 Benchmark starting
- 🔥 DR-AVX mode (important)
- ⚡ Intel SDE mode (important)
- ⏳ Waiting/initialization
- ⏸️ Cooling down period
- 📊 Results processing
- ✓ Task completed
- ⚠️ Important notice

## Prerequisites

### System Requirements

- Linux system (Ubuntu/Debian recommended)
- MongoDB 4.0+ at `/eval/real-workloads/mongodb/`
- YCSB 0.17.0 with MongoDB binding
- Python 2.7 (for YCSB) and Python 3.6+ (for analysis)
- Minimum 16GB RAM recommended

### Required Tools

1. **MongoDB Server**: `/eval/real-workloads/mongodb/bin/mongod`
2. **YCSB**: `/eval/tools/ycsb-mongodb-binding-0.17.0/`
3. **DR-AVX**: `/eval/bin/dravx/bin64/dravx`
4. **Intel SDE**: `/eval/bin/sde-9.58/sde64`
5. **Python Scripts**: `cal_mongodb_output.py`, `draw_mongodb.py`

### Python Dependencies

```bash
# For YCSB (Python 2.7)
sudo apt-get install python2.7

# For analysis scripts (Python 3)
pip3 install matplotlib numpy pandas
```

## Troubleshooting

### Common Issues

#### MongoDB Fails to Start

```bash
# Check port usage
netstat -tlnp | grep 27017

# Kill existing processes
pkill -9 mongod

# Check MongoDB log
tail -f /eval/real-workloads/mongodb/mongodb.log
```

#### YCSB Connection Errors

```bash
# Test MongoDB connection
mongo --eval "db.version()"

# Verify Python 2.7
python2 --version
```

#### Python Version Conflicts

- YCSB requires Python 2.7: `python2`
- Analysis scripts require Python 3.6+: `python3`
- Use explicit `python2` and `python3` commands

#### Out of Memory

```bash
# Reduce operation count
MongoDB_COUNT=1000000

# Reduce threads
MongoDB_THREADS=8

# Close other applications
```

#### SDE Very Slow

- This is expected (50-150% overhead)
- Wait for 8-minute initialization
- Consider reducing MongoDB_COUNT for SDE tests

### Debug Mode

```bash
# Enable bash debug
set -x

# Or run with debug flag
bash -x mongodb_ycsb.sh

# YCSB debug output
-p mongodb.debug=true
```

## Performance Tuning

### For Faster Testing

```bash
# Reduce operations significantly
MongoDB_THREADS=8 MongoDB_COUNT=500000 ./mongodb_ycsb.sh

# Test single workload (edit script)
MongoDB_SCENARIO=("workloadc")  # Read-only is fastest

# Skip SDE (comment out in script)
# run_sde_MongoDB
```

### For Accurate Results

```bash
# Increase operations and threads
MongoDB_THREADS=32 MongoDB_COUNT=20000000 ./mongodb_ycsb.sh

# Multiple iterations
for i in {1..3}; do
  ./mongodb_ycsb.sh
  sleep 600
done

# Bind to specific CPU cores
taskset -c 0-15 ./mongodb_ycsb.sh
```

### System Preparation

```bash
# Disable CPU frequency scaling
sudo cpupower frequency-set -g performance

# Clear caches
sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches

# Disable swap
sudo swapoff -a
```

## Workload Performance

| Workload | Read % | Write % | Typical Throughput |
|----------|--------|---------|-------------------|
| Workload B | 95% | 5% | Medium-High |
| Workload C | 100% | 0% | Highest |
| Workload D | 95% | 5% | Medium |

## Expected Overhead

| Engine | Overhead | Initialization |
|--------|----------|----------------|
| Native | 0% (baseline) | ~10 seconds |
| DR-AVX | 15-30% | ~10 seconds |
| Intel SDE | 50-150% | ~480 seconds |

## Analysis Scripts

### Output Parser

```bash
python3 cal_mongodb_output.py /path/to/raw_output.txt
```

Returns throughput value (operations per second).

### Chart Generator

```bash
python3 draw_mongodb.py "$data_string"
```

Creates comparison visualizations.

## References

- [YCSB Documentation](https://github.com/brianfrankcooper/YCSB)
- [YCSB Workload Parameters](https://github.com/brianfrankcooper/YCSB/wiki/Core-Workloads)
- [MongoDB Performance](https://www.mongodb.com/docs/manual/administration/analyzing-mongodb-performance/)
- [Intel SDE](https://www.intel.com/content/www/us/en/developer/articles/tool/software-development-emulator.html)

## Quick Reference

### File Locations

| Component | Path |
|-----------|------|
| Benchmark script | `/eval/mongodb_bench/mongodb_ycsb.sh` |
| MongoDB binary | `/eval/real-workloads/mongodb/bin/mongod` |
| MongoDB data | `/eval/real-workloads/mongodb/data/` |
| YCSB binary | `/eval/tools/ycsb-mongodb-binding-0.17.0/bin/ycsb` |
| YCSB workloads | `/eval/tools/ycsb-mongodb-binding-0.17.0/workloads/` |
| DR-AVX binary | `/eval/bin/dravx/bin64/dravx` |
| Intel SDE binary | `/eval/bin/sde-9.58/sde64` |
| Raw results | `/eval/results/mongodb/mongodb_raw_data/` |

### Execution Time Estimates

Based on typical hardware (Intel Xeon, 32GB RAM, SSD):

| Configuration | Native | DR-AVX | SDE | Total |
|---------------|--------|--------|-----|-------|
| Default (7.2M ops, 16 threads) | 30-40 min | 35-50 min | 60-90 min | 2-3 hours |
| Quick (1M ops, 8 threads) | 5-8 min | 6-10 min | 10-15 min | 25-35 min |
| Intensive (20M ops, 32 threads) | 60-80 min | 75-105 min | 150-210 min | 5-7 hours |


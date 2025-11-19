# MySQL Benchmark Suite

This guide describes the MySQL benchmark system using Sysbench to evaluate database performance under different execution environments: native, DR-AVX, and Intel SDE.

## Overview

The MySQL benchmark suite uses **Sysbench OLTP** (Online Transaction Processing) tests to measure database performance across three workload scenarios. The system automatically runs benchmarks, collects metrics, and generates visualizations.

## Components

### 1. Main Benchmark Script (`mysql_bench/mysql_sysbench.sh`)

The main script orchestrates all benchmark executions with colorized output.

#### Features

- **Three execution modes**: Native (baseline), DR-AVX, Intel SDE (AVX-512 emulation)
- **Three workload scenarios**: Read-only, Read-write, Write-only
- **Automated workflow**: Database initialization, benchmark execution, result collection, and visualization
- **Environment variable configuration**: Flexible parameter adjustment
- **Colorized output**: Enhanced terminal visibility with visual indicators

### 2. Test Scenarios

#### Read-Only (`oltp_read_only.lua`)
- **Operations**: SELECT queries
- **Tests**: Read performance and query optimization

#### Read-Write (`oltp_read_write.lua`)
- **Operations**: SELECT, UPDATE, INSERT, DELETE (mixed)
- **Tests**: Transaction handling and concurrency

#### Write-Only (`oltp_write_only.lua`)
- **Operations**: INSERT, UPDATE, DELETE
- **Tests**: Write performance and transaction logging

## Configuration

### Default Parameters

```bash
MySQL_THREADS=8          # Number of concurrent threads
MySQL_TIME=180           # Duration per test scenario (seconds)
MySQL_SCENARIO=("read_only" "read_write" "write_only")
```

### Sysbench Test Configuration

- **Tables**: 50 tables per test
- **Table size**: 10,000 rows per table
- **Total data**: ~500,000 rows
- **Database**: test_db (MySQL port 3306)

### Environment Variable Overrides

```bash
# Run with 16 threads and 300 seconds per test
MySQL_THREADS=16 MySQL_TIME=300 ./mysql_sysbench.sh

# Quick test with 4 threads and 60 seconds
MySQL_THREADS=4 MySQL_TIME=60 ./mysql_sysbench.sh
```

## Usage

### Running the Benchmark

```bash
# Navigate to the benchmark directory
cd /eval/mysql_bench

# Run with default settings
./mysql_sysbench.sh

# Run with custom parameters
MySQL_THREADS=16 MySQL_TIME=300 ./mysql_sysbench.sh
```

### Expected Duration

With default parameters (8 threads, 180 seconds per scenario):

- **Native**: ~9 minutes
- **DR-AVX**: ~9-12 minutes
- **SDE**: ~15-20 minutes (+ 3 min initialization)
- **Total**: ~35-45 minutes

## Execution Engines

### 1. Native (Baseline)

```bash
/eval/real-workloads/mysql/bin/mysqld \
  --defaults-file=/eval/real-workloads/mysql/my.cnf \
  --user=root
```

- No instrumentation overhead
- Initialization: ~10 seconds

### 2. DR-AVX (Quick R/W Mode)

```bash
/eval/bin/dravx/bin64/dravx -quick_rw 1 -- [MySQL command]
```

- Dynamic binary instrumentation
- Moderate overhead
- Initialization: ~30 seconds
- **Visual**: 🔥 Magenta/purple output

### 3. Intel SDE (AVX-512 Emulation)

```bash
/eval/bin/sde-9.58/sde64 -icl -avx512 1 -- [MySQL command]
```

- Software emulation of AVX-512
- Significant overhead
- Initialization: ~180 seconds (3 minutes)
- **Visual**: ⚡ Blue output with warning

## Output Structure

```
/eval/results/mysql/
├── mysql_raw_data/              # Raw benchmark output
│   ├── origin_mysql_read_only.txt
│   ├── origin_mysql_read_write.txt
│   ├── origin_mysql_write_only.txt
│   ├── dravx_mysql_read_only.txt
│   ├── dravx_mysql_read_write.txt
│   ├── dravx_mysql_write_only.txt
│   ├── sde_mysql_read_only.txt
│   ├── sde_mysql_read_write.txt
│   └── sde_mysql_write_only.txt
└── mysql_draw_data/             # Generated charts
    └── [chart files]
```

### Result Metrics

- **Transactions per second (TPS)**: Primary performance metric
- **Queries per second (QPS)**: Total query throughput
- **Latency**: Average, 95th percentile, 99th percentile
- **Read/Write operations**: Operations per second

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
- 📈 Chart generation
- ✓ Task completed
- ⚠️ Important notice

## Prerequisites

### System Requirements

- Linux system (Ubuntu/Debian recommended)
- MySQL 8.0+ at `/eval/real-workloads/mysql/`
- Sysbench installed (`apt-get install sysbench`)
- Python 3.6+ with matplotlib, numpy
- Minimum 8GB RAM recommended

### Required Tools

1. **MySQL Server**: `/eval/real-workloads/mysql/bin/mysqld`
2. **Sysbench**: Lua scripts in `/usr/share/sysbench/`
3. **DR-AVX**: `/eval/bin/dravx/bin64/dravx`
4. **Intel SDE**: `/eval/bin/sde-9.58/sde64`
5. **Python Scripts**: `cal_mysql_output.py`, `draw_mysql.py`

### Python Dependencies

```bash
pip3 install matplotlib numpy pandas
```

## Troubleshooting

### Common Issues

#### MySQL Fails to Start

```bash
# Check port usage
netstat -tlnp | grep 3306

# Kill existing processes
pkill -9 mysqld

# Verify data directory
ls -la /eval/real-workloads/mysql/data
```

#### Sysbench Errors

```bash
# Ensure MySQL is running
mysql -e "SHOW DATABASES;"

# Check test_db exists
mysql -e "USE test_db; SHOW TABLES;"
```

#### DR-AVX Initialization Timeout

- Increase sleep time after MySQL start (line 83 in script)
- Check DR-AVX binary permissions
- Monitor system resources with `htop`

#### SDE Very Slow

- This is expected (20-100× slowdown)
- Consider reducing MySQL_TIME for SDE-only tests
- Ensure adequate system resources

### Debug Mode

```bash
# Enable bash debug
set -x

# Or run with debug flag
bash -x mysql_sysbench.sh
```

## Performance Tuning

### For Faster Testing

```bash
# Reduce test duration and threads
MySQL_THREADS=4 MySQL_TIME=60 ./mysql_sysbench.sh

# Skip SDE (comment out in script)
# run_sde_MySQL

# Reduce table count in script
--tables=10 --table-size=1000
```

### For Accurate Results

```bash
# Increase test duration
MySQL_THREADS=16 MySQL_TIME=600 ./mysql_sysbench.sh

# Multiple iterations
for i in {1..3}; do
  ./mysql_sysbench.sh
  sleep 300
done

# Bind to specific CPU cores
taskset -c 0-7 ./mysql_sysbench.sh
```

### System Preparation

```bash
# Disable CPU frequency scaling
sudo cpupower frequency-set -g performance

# Clear caches
sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches

# Stop unnecessary services
```

## Analysis Scripts

### Output Parser

```bash
python3 cal_mysql_output.py /path/to/raw_output.txt
```

Returns transactions per second (TPS) value.

### Chart Generator

```bash
python3 draw_mysql.py "$data_string" "$output_directory"
```

Creates comparison visualizations.

## References

- [Sysbench Documentation](https://github.com/akopytov/sysbench)
- [MySQL Performance](https://dev.mysql.com/doc/refman/8.0/en/optimization.html)
- [Intel SDE](https://www.intel.com/content/www/us/en/developer/articles/tool/software-development-emulator.html)

## Quick Reference

### File Locations

| Component | Path |
|-----------|------|
| Benchmark script | `/eval/mysql_bench/mysql_sysbench.sh` |
| MySQL binary | `/eval/real-workloads/mysql/bin/mysqld` |
| MySQL config | `/eval/real-workloads/mysql/my.cnf` |
| DR-AVX binary | `/eval/bin/dravx/bin64/dravx` |
| Intel SDE binary | `/eval/bin/sde-9.58/sde64` |
| Raw results | `/eval/results/mysql/mysql_raw_data/` |
| Charts | `/eval/results/mysql/mysql_draw_data/` |
| Parser script | `/eval/mysql_bench/cal_mysql_output.py` |
| Chart script | `/eval/mysql_bench/draw_mysql.py` |

### Parameter Reference

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| MySQL_THREADS | 8 | 1-64 | Number of concurrent threads |
| MySQL_TIME | 180 | 10-3600 | Duration per scenario (seconds) |

### Sysbench Command Template

```bash
sysbench /usr/share/sysbench/oltp_${scenario}.lua \
  --threads=${MySQL_THREADS} \
  --time=${MySQL_TIME} \
  --mysql-host=127.0.0.1 \
  --mysql-port=3306 \
  --mysql-user=root \
  --mysql-db=test_db \
  --tables=50 \
  --table-size=10000 \
  --range_selects=off \
  --db-ps-mode=disable \
  --report-interval=1 \
  [prepare|run|cleanup]
```


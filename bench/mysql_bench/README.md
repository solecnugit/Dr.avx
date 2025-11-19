# MySQL Benchmark Automated Testing Suite

Automated MySQL performance testing tool to compare performance across Native, Dr.avx, and Intel SDE64 execution modes.

## Features

- 🚀 **Full automation**: From database initialization to result visualization in one command
- 🎨 **Colored output**: Clear progress indicators and status messages
- ⚙️ **Flexible configuration**: Customizable thread count and test duration
- 📊 **Auto visualization**: Automatic result extraction and performance chart generation

## Workflow

The complete testing workflow includes:

1. **Phase 1: Initialize MySQL** (`init_mysql.sh`)
   - Create data directories
   - Initialize MySQL database
   - Create test database `test_db`

2. **Phase 2: Run Benchmarks** (`mysql_sysbench.sh`)
   - Run sysbench tests under three modes: Native, Dr.avx, Intel SDE64
   - Test three scenarios: read_only, read_write, write_only
   - Auto-call `cal_mysql_output.py` to extract performance metrics
   - Auto-call `draw_mysql.py` to generate comparison charts

## Quick Start

### Basic Usage

```bash
# Run complete workflow (init + benchmark)
./run_mysql.py

# Or with python3
python3 run_mysql.py
```

### Common Options

```bash
# Quick test (fewer threads and shorter duration)
./run_mysql.py --threads 4 --time 60

# Skip database initialization (use existing database)
./run_mysql.py --skip-init

# Initialize database only
./run_mysql.py --init-only

# Custom configuration
./run_mysql.py --threads 16 --time 300
```

## Command Line Arguments

| Argument | Description | Default |
|----------|-------------|---------|
| `--skip-init` | Skip MySQL initialization, use existing database | - |
| `--init-only` | Initialize MySQL only, skip benchmarking | - |
| `--skip-benchmark` | Skip benchmark tests (for debugging) | - |
| `--threads N` | Number of threads for sysbench tests | 8 |
| `--time SECONDS` | Duration for each test in seconds | 180 |

## Examples

```bash
# Full workflow (8 threads, 180s per test)
./run_mysql.py

# Quick test mode (4 threads, 60s per test)
./run_mysql.py --threads 4 --time 60

# High load test (16 threads, 300s per test)
./run_mysql.py --threads 16 --time 300

# Skip initialization (database already exists)
./run_mysql.py --skip-init --threads 8

# Initialize database only
./run_mysql.py --init-only
```

## Time Estimates

Complete workflow duration depends on configuration:

- **Default config** (8 threads, 180s): ~30-60 minutes
- **Quick test** (4 threads, 60s): ~10-20 minutes  
- **High load test** (16 threads, 300s): ~60-90 minutes

Formula: `Total time ≈ 3 modes × 3 scenarios × test duration + startup overhead`

## Output Results

### Raw Data
- **Location**: `/eval/results/mysql/mysql_raw_data/`
- **Files**: 
  - `origin_mysql_read_only.txt`
  - `origin_mysql_read_write.txt`
  - `origin_mysql_write_only.txt`
  - `dravx_mysql_read_only.txt`
  - `dravx_mysql_read_write.txt`
  - `dravx_mysql_write_only.txt`
  - `sde_mysql_read_only.txt`
  - `sde_mysql_read_write.txt`
  - `sde_mysql_write_only.txt`

### Visualization Charts
- **Location**: `../figs/` (relative to `mysql_bench/` directory)
- **Files**:
  - `Fig10.a_mysql.pdf` - Vector format
  - `Fig10.a_mysql.png` - Bitmap format

## Test Scenarios

1. **read_only**: Read-only workload, simulates pure query load
2. **read_write**: Mixed read/write workload, simulates real application load
3. **write_only**: Write-only workload, simulates insert/update operations

## Test Configuration

### MySQL Settings
- Database: test_db
- Tables: 50 tables
- Rows per table: 10,000
- Port: 3306
- User: root (no password)

### Sysbench Settings
- Threads: Configurable via `--threads` (default: 8)
- Test duration: Configurable via `--time` (default: 180s)
- Report interval: 1 second
- Prepared statements: disabled
- Range selects: disabled

## Environment Variables

Control test parameters via environment variables:

```bash
# Set thread count
export MySQL_THREADS=16

# Set test duration
export MySQL_TIME=300

# Run tests
./run_mysql.py --skip-init
```

## Troubleshooting

### MySQL Won't Start

**Issue**: MySQL initialization or startup fails

**Solution**:
```bash
# Clean old data
rm -rf /eval/real-workloads/mysql/data
rm -rf /eval/real-workloads/mysql/tmp

# Re-initialize
./run_mysql.py --init-only
```

### Port Already in Use

**Issue**: MySQL port 3306 is occupied

**Solution**:
```bash
# Find process using the port
lsof -i :3306

# Kill MySQL processes
pkill mysqld

# Or specify PID
kill -9 <PID>
```

### Sysbench Not Found

**Issue**: `sysbench not found`

**Solution**:
```bash
# Ubuntu/Debian
apt-get install sysbench

# CentOS/RHEL
yum install sysbench
```

### Insufficient Disk Space

**Issue**: Out of disk space during initialization or testing

**Solution**:
```bash
# Check disk space
df -h /eval

# Clean old test results
rm -rf /eval/results/mysql/mysql_raw_data/*
```

## Performance Tuning

1. **Reduce test time**: Lower `--time` parameter (but results may be less stable)
2. **Reduce threads**: Lower `--threads` parameter (suitable for quick validation)
3. **Skip initialization**: Use `--skip-init` when repeating tests to save time
4. **Increase stability**: Increase `--time` and `--threads` for more reliable results

## Related Files

- `run_mysql.py` - Main orchestration script
- `init_mysql.sh` - MySQL initialization script
- `mysql_sysbench.sh` - Sysbench test script
- `cal_mysql_output.py` - Result extraction script
- `draw_mysql.py` - Chart generation script

## Support

If you encounter issues, check:
1. MySQL installation path is correct
2. Sysbench is installed
3. Sufficient disk space available
4. Port 3306 is not occupied
5. Review error logs for details

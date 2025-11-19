# MongoDB Benchmark Automated Testing Suite

Automated pipeline for MongoDB YCSB benchmarks with DR.avx and Intel SDE64.

## Quick Start

```bash
# Run complete workflow with default settings (16 threads, 720K records)
./run_mongodb.py

# Run with custom configuration (faster)
./run_mongodb.py --threads 8 --count 500000

# Skip initialization if database already exists
./run_mongodb.py --skip-init
```

## Command Line Options

- `--threads N` - Number of YCSB threads (default: 16)
- `--count N` - Number of records (default: 720,000)
- `--skip-init` - Skip MongoDB initialization
- `--init-only` - Only initialize, skip benchmarking
- `--skip-benchmark` - Skip benchmarking (for testing)

## Workflow

1. **Initialize** - Creates MongoDB data directory
2. **Benchmark** - Runs YCSB workloads (b, c, d) on 3 engines:
   - Origin (native)
   - DR.avx
   - Intel SDE64
3. **Extract** - Parses throughput from results (automatic)
4. **Visualize** - Generates performance comparison chart (automatic)

## Output

- Raw data: `/eval/results/mongodb/mongodb_raw_data/`
- Figure: `/eval/figs/Fig10.b_mongodb.pdf` and `.png`

## Performance Tips

- **Faster testing**: Use `--threads 8 --count 500000`
- **Production**: Use default settings or higher
- **Duration**: ~30-60 minutes for default configuration

## Prerequisites

- MongoDB installation: `/eval/real-workloads/mongodb/`
- YCSB: `/eval/tools/ycsb-mongodb-binding-0.17.0/`
- Python 2 (required by YCSB)
- Python 3 (for this script)

## Examples

```bash
# Quick test (8 threads, 500K records)
./run_mongodb.py --threads 8 --count 500000

# Standard test (default)
./run_mongodb.py

# High-load test (32 threads, 1M records)
./run_mongodb.py --threads 32 --count 1000000

# Re-run benchmark with existing database
./run_mongodb.py --skip-init
```

## Troubleshooting

**Issue**: "MongoDB not found"
- Check MongoDB installation at `/eval/real-workloads/mongodb/bin/mongod`

**Issue**: "YCSB not found"
- Check YCSB installation at `/eval/tools/ycsb-mongodb-binding-0.17.0/`

**Issue**: "python2 not found"
- YCSB requires Python 2, install it if missing




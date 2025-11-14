# Dr.avx

[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20x86--64-lightgrey.svg)]()

> **Run AVX‑512 binaries on processors without native AVX‑512 support — transparently and efficiently.**

Dr.avx is an open‑source **dynamic compilation / translation** system build atop [DynamoRIO](https://dynamorio.org/) 10.0.0, which rewrites AVX‑512 instructions at runtime so that binaries compiled for AVX‑512 can run on hardware that lacks native support. It addresses **Generational ISA Fragmentation (GIF)** — when newer CPU generations drop support for instructions present in earlier parts of the same ISA family.


## Table of Contents

- [Features](#features)
- [Prerequisites](#prerequisites)
- [Build](#build)
- [Quick Start](#quick-start)
- [Usage](#usage)
- [Benchmarks](#benchmarks)
- [Limitations & Notes](#limitations--notes)
- [Contributing](#contributing)
- [Related Work](#related-work)
- [License](#license)
- [Roadmap](#roadmap)


## Features

- **Transparent execution:** Run unmodified AVX‑512 binaries on x86‑64 systems without AVX‑512.
- **Dynamic rewriting:** Per‑instruction translation to semantically equivalent sequences (DynamoRIO IR in debug).
- **Near-Native Performance on Real-World Workloads:** Achieve near-native performance on real-world workloads. 
- **Open ecosystem:** Built on widely used open‑source tooling; easy to extend and evaluate.



## Prerequisites

- **Hardware:** x86‑64 CPU
- **OS:** Linux (tested on Ubuntu 20.04, linux kernel 5.4.0; other distributions likely work)
- **Toolchain:** GCC **9.4.0+** (or compatible), CMake **3.16+**
- **Libraries:** `libunwind-dev`, `libsnappy-dev`, `liblz4-dev`, `libxxhash-dev`

> Debian/Ubuntu one‑liner:
```bash
sudo apt-get update && \
sudo apt-get install -y build-essential cmake git \
    libunwind-dev libsnappy-dev liblz4-dev libxxhash-dev
```

## Build

We recommend out‑of‑source builds and modern CMake invocation:

### Release
```bash
git clone https://github.com/solecnugit/Dr.avx.git
cd dravx
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

### Debug
```bash
git clone https://github.com/solecnugit/Dr.avx.git
cd dravx
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_BUILD_TYPE=Debug -DDEBUG=ON -DDR_FAST_IR=ON
cmake --build build -j"$(nproc)"
```

**What Debug mode does:** emits, for each AVX‑512 instruction, the semantically equivalent rewritten instruction sequence (DynamoRIO‑IR). This mode is noticeably slower than Release; some workloads (e.g., GCC or Perl) may run **3–5×** longer than native.

> Build artifacts: the `dravx` launcher is typically located under `build/bin64/`.


## Quick Start

Run Dr.avx as a **compatibility layer** (similar in spirit to user‑mode dynamic translation tools like Intel SDE, QEMU user‑mode, or DynamoRIO):

```bash
cd build/bin64
# Execute a unit test that contains AVX‑512 instructions
./dravx -- ../../unittests/vadd-512
```

The `--` separates Dr.avx options from the target **program** and its **arguments**; everything after `--` is forwarded to the target.

Use `./dravx -h` to inspect runtime options (if available in your build).


## Usage

standalone running:
```bash
# General form
./dravx -- <program> [args...]
```

### Examples (Unit Tests & Debug Output)

**Vector Add (unit test).** If your machine supports **AVX‑512**, you can also run the binary **natively** to cross‑check correctness.

From the **repository root** (change the relative path, if you are in other directory):
```bash
# Native run (only if the CPU supports AVX-512)
./unittests/vadd-512

# Dr.avx (compatibility layer)
./build/bin64/dravx -- ./unittests/vadd-512
```

**Debug‑mode rewrite samples** 

Below are two representative AVX‑512 instruction rewrites printed in Debug builds.
```bash
[REWRITE INFO]: ==== Rewriting vpaddd at 0x0000000000000000 ====
vpaddd {%k0} %zmm0 %zmm1 -> %zmm0
  mask: %k0
  src1: %zmm0
  src2: %zmm1
  dst: %zmm0
[DEBUG]: ==== INSTRUCTION SEQUENCE ====
vmovdqu %ymm10 -> %gs:0x00000300[32byte]
vmovdqu %ymm11 -> %gs:0x00000340[32byte]
vmovdqu %gs:0xa0[32byte] -> %ymm10
vmovdqu %gs:0xe0[32byte] -> %ymm11
vpaddd %ymm0 %ymm1 -> %ymm0
vpaddd %ymm10 %ymm11 -> %ymm10
vmovdqu %ymm0 -> %gs:0x80[32byte]
vmovdqu %ymm10 -> %gs:0xa0[32byte]
vmovdqu %gs:0x00000300[32byte] -> %ymm10
vmovdqu %gs:0x00000340[32byte] -> %ymm11
[DEBUG]: ==============================

[REWRITE INFO]: ==== Rewriting vmovdqa64 at 0x0000000000000000 ====
vmovdqa64 {%k0} %zmm0 -> 0x40(%rsp)[64byte]
  mask: %k0
  src1: %zmm0
  dst: 0x40(%rsp)
[DEBUG]: ==== INSTRUCTION SEQUENCE ====
vmovdqu %ymm10 -> %gs:0x00000300[32byte]
vmovdqu %gs:0xa0[32byte] -> %ymm10
vmovdqu %gs:0x80[32byte] -> %ymm0
vmovdqu %ymm0 -> 0x40(%rsp)[32byte]
vmovdqu %ymm10 -> 0x60(%rsp)[32byte]
vmovdqu %gs:0x00000300[32byte] -> %ymm10
[DEBUG]: ==============================
```

## Benchmarks

Below are illustrative results from our evaluations. 

### 1) XZ Compression Example 

- **Dr.avx:** 106.64 s  
- **Intel SDE:** 115.91 s

Dr.avx completed this workload faster than Intel SDE under the same setup (computed from the times above). In the figures below, the **upper image shows Dr.avx performance** while the **lower image shows Intel SDE performance**.

```bash
# Dr.avx
❯ taskset -c 2 time ~/Dr.avx/bin64/dravx -- ./xz_r_base.mytest-m64 cld.tar.xz 160 19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd5454852652806a657bb56e07d61128434b474 59796407 61004416 6
SPEC CPU XZ driver: input=cld.tar.xz insize=160
Loading Input Data
Compressed size: 82270464; Uncompressed size: 226775040
SHA-512 of decompressed data compared successfully!
Decompressed data too large; truncating to requested size 167772160
SHA-512 of input file: 4c6ce670df299f1bcc630778ee98388d179745b3fe28c959b7fe185db01bd9923f445ac2dc3e73d680956dcda35301c26fc753fa111bca3ba526b00a81f4a2c0
Input data 167772160 bytes in length
Compressing Input Data, level 6
Compressed data 60400412 bytes in length
Compressed data is between 59796407 and 61004416 bytes in length
Finished compressing data
Uncompressing previously compressed data
Uncompressed data 167772160 bytes in length
Uncompressed data compared correctly
Tested 160 MiB buffer: OK!
105.95user 0.57system 1:46.64elapsed 99%CPU (0avgtext+0avgdata 747420maxresident)k
168408inputs+8outputs (32major+310551minor)pagefaults 0swaps

# Intel SDE64
❯ taskset -c 2 time ~/tools/sde-9.58/sde64 -icl -avx512 1 -- ./xz_r_base.mytest-m64 cld.tar.xz 160 19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd5454852652806a657bb56e07d61128434b474 59796407 61004416 6
SPEC CPU XZ driver: input=cld.tar.xz insize=160
Loading Input Data
Compressed size: 82270464; Uncompressed size: 226775040
SHA-512 of decompressed data compared successfully!
Decompressed data too large; truncating to requested size 167772160
SHA-512 of input file: 4c6ce670df299f1bcc630778ee98388d179745b3fe28c959b7fe185db01bd9923f445ac2dc3e73d680956dcda35301c26fc753fa111bca3ba526b00a81f4a2c0
Input data 167772160 bytes in length
Compressing Input Data, level 6
Compressed data 60400412 bytes in length
Compressed data is between 59796407 and 61004416 bytes in length
Finished compressing data
Uncompressing previously compressed data
Uncompressed data 167772160 bytes in length
Uncompressed data compared correctly
Tested 160 MiB buffer: OK!
114.42user 1.41system 1:55.91elapsed 99%CPU (0avgtext+0avgdata 790468maxresident)k
43560inputs+0outputs (151major+377357minor)pagefaults 0swaps
```

### 2) `llama.cpp` Token Generation
The following benchmark results were generated using `llama_bench` directly and are presented in their original tabular format.


native run commands as below:
```
./build/bin/llama-bench -m ./models/llama2_xs_460m_experimental.q8_0.gguf -p 0 -n 64 -t 1 -b 512 -ngl 0 -r 5
```

**Native (baseline)**

| Model          | Size       | Params   | Backend | Threads | Test  | Tokens/s (↑) |
|----------------|------------|----------|---------|--------:|-------|-------------:|
| llama ?B Q8_0  | 467.96 MiB | 461.69 M | CPU     |       1 | tg 64 | 25.11 ± 0.03 |

**Dr.avx**

| Model          | Size       | Params   | Backend | Threads | Test  | Tokens/s (↑) |
|----------------|------------|----------|---------|--------:|-------|-------------:|
| llama ?B Q8_0  | 467.96 MiB | 461.69 M | CPU     |       1 | tg 64 | 24.92 ± 0.11 |

**Intel SDE**

| Model          | Size       | Params   | Backend | Threads | Test  | Tokens/s (↑) |
|----------------|------------|----------|---------|--------:|-------|-------------:|
| llama ?B Q8_0  | 467.96 MiB | 461.69 M | CPU     |       1 | tg 64 | 9.78 ± 0.00  |


---



## Limitations & Notes

- Instruction coverage continues to evolve; some AVX‑512 subsets and instructions emulation may be partially implemented.
- Debug builds are significantly slower due to IR emission and instrumentation.
- Certain programs with intensive floating‑point hot paths may still show noticeable gaps to native.

We actively track coverage and performance gaps via issues and regression tests.


## Contributing

We welcome contributions! Areas of particular interest:

- **Extended coverage:** additional AVX‑512 subsets
- **Performance:** faster FP paths, reduced TLS/metadata traffic, hot‑path specialization
- **Portability (experimental):** mappings toward ARM SVE/SVE2, RISC‑V V
- **Validation:** more end‑to‑end real‑world workloads

Please open an issue or a discussion before large changes. We recommend:
- Consistent formatting (`clang-format`) and static checks
- Adding unit tests and microbenchmarks for new translations
- Including before/after performance numbers for optimizations

For detailed instructions on how to add support for a new instruction, please refer to our [guide.md](docs/guide.md) in the docs directory.

## Related Work

- **Intel SDE** — widely used closed‑source dynamic emulation baseline  
- **DynamoRIO** — open‑source dynamic instrumentation foundation used by Dr.avx  
- **QEMU (user‑mode)** — general dynamic translation for cross‑ISA execution


## License

Licensed under the **BSD 3‑Clause License**. See [LICENSE](LICENSE) for details.


## Roadmap

- [ ] Faster floating‑point implementations in hot paths
- [ ] Broaden AVX‑512 subset coverage (priority by real‑world demand)
- [ ] End‑to‑end regression + perf CI (representative workloads)
- [ ] Optional cross‑ISA backends (exploratory): ARM SVE/SVE2, RISC‑V V

## Appendix

For a detailed list of currently supported AVX-512 instructions, please see our [coverage.md](docs/coverage.md) document.


# Cache and Memory Hierarchy Simulator

A trace-driven cache simulator written in modern C++17. Models direct-mapped, set-associative, and fully-associative caches with configurable replacement policies, write policies, and multi-level hierarchies including an optional victim cache.

Built as a portfolio project targeting memory systems, computer architecture, and SDE roles at companies like AMD, Intel, NVIDIA, Qualcomm, ARM, and Synopsys.

---

## Features

### Phase 1 — Single Cache
- Direct-mapped, N-way set-associative, and fully-associative caches (unified implementation — see design notes below)
- Configurable cache size, block size, and associativity
- Replacement policies: LRU, FIFO, Random (all pluggable via Strategy pattern)
- Per-access statistics: reads, writes, hits, misses, hit rate, miss rate

### Phase 2 — Hierarchy and Write Policies
- L1 + L2 two-level hierarchy
- Write-back and write-through policies
- Write-allocate and no-write-allocate allocation policies
- Victim cache (8-entry fully-associative buffer between L1 and L2)
- Dirty-eviction write-back routing between levels
- AMAT (Average Memory Access Time) computation

### Trace-driven simulation
- Plain-text memory trace parser (`R`/`W` + hex address per line)
- JSON-based cache configuration — no recompilation needed for parameter sweeps
- Four sample traces: sequential, random, loop-locality, matrix-multiply

---

## Build

**Requirements:** CMake ≥ 3.16, a C++17-capable compiler (GCC ≥ 9 or Clang ≥ 10).  
Dependencies (GoogleTest, nlohmann/json) are fetched automatically by CMake.

```bash
git clone https://github.com/<your-username>/cache-simulator.git
cd cache-simulator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

**Run tests:**
```bash
ctest --test-dir build --output-on-failure
```

Expected: **66/66 tests passed**.

---

## Usage

```bash
./build/cachesim_cli <config.json> <trace.trace>
```

**Examples:**

```bash
# Single-level 4-way LRU cache, loop-locality trace
./build/cachesim_cli configs/l1_4way_lru.json traces/loop_locality.trace

# Two-level write-back hierarchy, matrix multiply trace
./build/cachesim_cli configs/l1_l2_writeback.json traces/matrix_multiply.trace

# Same but with victim cache enabled
./build/cachesim_cli configs/l1_l2_victim.json traces/matrix_multiply.trace
```

**Sample output (loop_locality, 4-way LRU):**
```
Loaded 1600 accesses from traces/loop_locality.trace

====== Cache Simulation Results ======

[L1 Cache]
  Reads        : 1600
  Writes       : 0
  Hits         : 1584
  Misses       : 16
  Hit Rate     : 99.00%
  Miss Rate    : 1.00%

[Overall]
  Total Cycles : 9600
  Main Mem I/O : 16
  AMAT         : 6.00 cycles
======================================
```

---

## Config format

```json
{
  "l1": {
    "cache_size_bytes": 32768,
    "block_size_bytes": 64,
    "associativity": 4,
    "replacement_policy": "LRU",
    "write_policy": "WriteBack",
    "allocation_policy": "WriteAllocate"
  },
  "l2": {
    "cache_size_bytes": 262144,
    "block_size_bytes": 64,
    "associativity": 8,
    "replacement_policy": "LRU",
    "write_policy": "WriteBack",
    "allocation_policy": "WriteAllocate"
  },
  "victim_cache": false,
  "latency": {
    "l1_hit_cycles": 4,
    "l2_hit_cycles": 12,
    "mem_cycles": 200
  }
}
```

`replacement_policy`: `"LRU"` | `"FIFO"` | `"Random"`  
`write_policy`: `"WriteBack"` | `"WriteThrough"`  
`allocation_policy`: `"WriteAllocate"` | `"NoWriteAllocate"`

---

## Trace format

```
# Lines starting with # are comments and are ignored
R 0x1A2B3C      # read
W 0x00FF00      # write
r 1000          # lowercase and no 0x prefix are both accepted
```

Generate traces with:
```bash
python3 scripts/generate_traces.py
```

---

## Project structure

```
cache-simulator/
├── include/cachesim/       # Public headers — one per class
├── src/                    # Implementations
├── tests/                  # GoogleTest unit tests — one file per class/feature
├── traces/                 # Sample memory access traces
├── configs/                # JSON hierarchy configurations
├── scripts/                # Trace generation + result plotting (Python)
└── docs/                   # Design document, address math worked examples
```

---

## Design notes

**Unified cache model.** Direct-mapped (`associativity=1`), N-way set-associative, and fully-associative (`numSets=1`) are all handled by a single `Cache`/`CacheSet` implementation. The "mode" falls out of the configuration numbers, not from separate code paths. This is correct structurally: direct-mapped is just 1-way, fully-associative is just 1-set.

**Replacement policy as Strategy pattern.** `ReplacementPolicy` is an abstract interface; `LRUPolicy`, `FIFOPolicy`, and `RandomPolicy` implement it. `CacheSet` delegates all eviction decisions to whichever policy is injected at construction — adding a new policy (e.g. Pseudo-LRU) requires no changes to `CacheSet` or `Cache`.

**Counter-based LRU.** Each way stores a logical timestamp. On eviction, scan all ways for the minimum. O(associativity) — for realistic associativity (≤16-way) this is faster in practice than a pointer-chasing linked list due to cache-friendly access patterns. A real hardware implementation compares all ways in parallel via dedicated comparators; we model this as a sequential scan since we're not doing gate-level timing simulation.

**Address decomposition.** Given block size B and number of sets S (both required to be powers of two, enforced at construction):
```
offset = address & (B - 1)                    // low log2(B) bits
index  = (address >> log2(B)) & (S - 1)       // next log2(S) bits  
tag    = address >> (log2(B) + log2(S))        // remaining high bits
```

**Dirty write-back routing.** `Cache` only stores and reports dirty-eviction metadata (`writeBackNeeded`, `writeBackAddress`). `MemoryHierarchy` is the only component that acts on it — routing the write-back to L2, victim cache, or main memory as appropriate. This keeps `Cache` topology-blind and independently testable.

---

## Benchmark results

| Trace           | Config              | Hit Rate | AMAT (cycles) |
|-----------------|---------------------|----------|---------------|
| loop_locality   | L1 4-way LRU        | 99.00%   | 6.00          |
| matrix_multiply | L1+L2 write-back    | 98.86%   | 6.41          |
| sequential      | L1 4-way LRU        | —        | run it        |
| random          | L1 4-way LRU        | —        | run it        |

Run the remaining benchmarks yourself and add your results here.

---

## Future work (Phase 3 / Phase 4)

- Stream prefetcher and stride prefetcher (Phase 3)
- MESI cache coherence protocol for multi-core simulation (Phase 4)
- Pseudo-LRU replacement policy
- TLB simulation
- CSV stats export and matplotlib visualization dashboard
- Inclusive vs exclusive cache inclusion policies

---

## Resume bullets (after you run your own benchmarks)

```
• Designed and implemented a trace-driven cache hierarchy simulator in C++17 (CMake,
  GoogleTest) modelling direct-mapped, set-associative, and fully-associative caches with
  LRU/FIFO/Random eviction policies, write-back/write-through, and a victim cache

• Achieved 99% L1 hit rate on temporal-locality workloads; demonstrated 6.00-cycle AMAT
  vs 204-cycle worst-case for loop-heavy access patterns

• Applied Strategy pattern for pluggable replacement policies; unified three cache
  configurations (direct-mapped, set-associative, fully-associative) in one parameterised
  implementation; 66 GoogleTest unit tests, zero warnings under -Wall -Wextra -Wshadow
```

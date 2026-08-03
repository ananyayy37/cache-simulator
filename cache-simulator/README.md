# Cache and Memory Hierarchy Simulator

A trace-driven cache simulator written in modern C++17. Models direct-mapped, set-associative, and fully-associative caches with configurable replacement policies, write policies

## Features

### Single Cache
- Direct-mapped, N-way set-associative, and fully-associative caches 
- Configurable cache size, block size, and associativity
- Replacement policies: LRU, FIFO, Random
- Per-access statistics: reads, writes, hits, misses, hit rate, miss rate

###  Hierarchy and Write Policies
- L1 + L2 two-level hierarchy
- Write-back and write-through policies
- Write-allocate and no-write-allocate allocation policies
- Victim cache (8-entry fully-associative buffer between L1 and L2)
- Dirty-eviction write-back routing between levels
- AMAT (Average Memory Access Time) computation

### Trace-driven simulation
- Plain-text memory trace parser (`R`/`W` + hex address per line)
- JSON-based cache configuration - no recompilation needed for parameter sweeps
- Four sample traces: sequential, random, loop-locality, matrix-multiply

---

---

```bash
./build/cachesim_cli <config.json> <trace.trace>
```

**Examples:**

```bash
# Single-level 4-way LRU cache, loop-locality trace
./build/cachesim_cli configs/l1_4way_lru.json traces/loop_locality.trace

# Two-level write-back hierarchy, matrix multiply trace
./build/cachesim_cli configs/l1_l2_writeback.json traces/matrix_multiply.trace
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

---

## Trace format

```
R 0x1A2B3C      # read
W 0x00FF00      # write
r 1000          
```

Generate traces with:
```bash
python3 scripts/generate_traces.py
```

---

## Benchmark results

| Trace           | Config              | Hit Rate | AMAT (cycles) |
|-----------------|---------------------|----------|---------------|
| loop_locality   | L1 4-way LRU        | 99.00%   | 6.00          |
| matrix_multiply | L1+L2 write-back    | 98.86%   | 6.41          |
| sequential      | L1 4-way LRU        | 0%       | 204           |
| random          | L1 4-way LRU        | 0.6%     | 99.40%        |
```

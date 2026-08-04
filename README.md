# Cache and Memory Hierarchy Simulator

A trace-driven cache simulator written in modern C++. It models direct-mapped, set-associative, and fully associative cache organizations.

## Features

### Single Cache
- Direct-mapped, N-way set-associative, and fully associative caches
- Configurable cache size, block size, and associativity
- Replacement policies: LRU, FIFO, Random
- Per-access statistics:
  - Reads
  - Writes
  - Hits
  - Misses
  - Hit rate
  - Miss rate

### Trace-Driven Simulation
- Plain-text memory trace parser (`R`/`W`)
- JSON-based cache configuration
- Three sample traces:
  - Random
  - Loop locality
  - Matrix multiplication

## Sample Output

```text
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

## Configuration Format

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

# Trace Format

Each `.trace` file is plain text, one memory access per line:

```
<R|W> <hex address>
```

Example:
```
R 0x1000
W 0x1008
R 0x2000
```

- `R` = read, `W` = write
- Addresses are hex, no `0x` prefix required but accepted

## Files in this directory

| File | Pattern | Purpose |
|---|---|---|
| `sequential.trace` | Strictly increasing addresses | Spatial-locality baseline; good prefetcher test in Phase 3 |
| `random.trace` | Uniformly random addresses | Worst case for any cache; stresses eviction policy choice |
| `loop_locality.trace` | Small working set, repeated | Temporal-locality baseline; LRU should shine here |
| `matrix_multiply.trace` | Derived from a real small matmul access pattern | Realistic mixed locality |

Generated via `scripts/generate_traces.py`. Regenerate with:
```
python3 scripts/generate_traces.py
```

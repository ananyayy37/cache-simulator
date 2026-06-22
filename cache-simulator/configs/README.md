# Cache Configuration Files

JSON configs describing a cache (or hierarchy) setup, loaded at runtime by `cachesim_cli`.

## Schema (Phase 1)

```json
{
  "cache_size_bytes": 32768,
  "block_size_bytes": 64,
  "associativity": 4,
  "replacement_policy": "LRU"
}
```

- `replacement_policy`: one of `"LRU"`, `"FIFO"`, `"Random"`
- `associativity`: 1 = direct-mapped; `cache_size_bytes / block_size_bytes` = fully associative; anything else = set-associative

Phase 2 configs add `write_policy` (`"WriteThrough"` / `"WriteBack"`) and `allocation_policy` (`"WriteAllocate"` / `"NoWriteAllocate"`), plus an `l2` block for hierarchy setups.

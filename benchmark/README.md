# Benchmarks

Performance suite for `bin_serializer`, built on [Google Benchmark v1.9.5](https://github.com/google/benchmark). Three executables cover throughput, stress, and per-message latency. All benchmarks are compiled in Release mode and use `DoNotOptimize` / `ClobberMemory` barriers to prevent the optimizer from eliminating serialization work.

---

## Building

From the repo root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)   # Linux
cmake --build build --config Release -j$(sysctl -n hw.logicalcpu)  # macOS
```

Each `.cpp` file in this directory compiles to a separate executable under `build/benchmark/`.

---

## Executables

### `benchmark_compare` — Fast Path vs Naive vs FixedBuffer

The primary comparison benchmark. Pits three serialization strategies against each other across multiple struct sizes, then adds deserialization and string payload sweeps.

**What it tests:**

| Benchmark | What it measures |
|---|---|
| `BM_Vec_Fast` | Fast path: `Vec{float,float,float}` via single `memcpy` (trivially copyable) |
| `BM_Vec_Naive` | Naive path: same `Vec` via field-by-field reflect dispatch |
| `BM_Vec_Memcpy` | Raw `buffer.write(&val, sizeof(val))` baseline — no serializer overhead |
| `BM_LargeStruct_Fast` | Fast path: 80-byte, 20-field `LargeStruct` as one bulk copy |
| `BM_LargeStruct_Naive` | Naive path: same struct, 20 separate field writes |
| `BM_Vec_Fixed` | Fast path into `FixedBuffer<128>` (stack-allocated, no heap) |
| `BM_LargeStruct_Fixed` | Fast path into `FixedBuffer<1024>` |
| `BM_Vec_Deserialize` | Deserialization of `Vec` back from a pre-serialized `Buffer` |
| `BM_LargeStruct_Deserialize` | Deserialization of `LargeStruct` |
| `BM_StringPayload/N` | `std::string` of length N: 0, 8, 32, 128, 1024, 4096 bytes |
| `BM_Vec_FreshAllocation` | Fast path with a brand-new `Buffer` each iteration (allocation cost visible) |

```bash
./build/benchmark/benchmark_compare
```

**Results** (Apple Silicon, ARM64, Release):

```
Benchmark                           Time             CPU   Iterations  Throughput
---------------------------------------------------------------------------------
BM_Vec_Fast                      7.40 ns         7.39 ns   92388507   1.51 GB/s
BM_Vec_Naive                     19.7 ns         19.6 ns   34943043   585 MB/s
BM_Vec_Memcpy                    7.36 ns         7.36 ns   95644095   1.52 GB/s
BM_LargeStruct_Fast              8.96 ns         8.96 ns   78907914   8.32 GB/s
BM_LargeStruct_Naive             75.5 ns         75.5 ns    9345794   1.01 GB/s
BM_Vec_Fixed                     3.54 ns         3.54 ns  197244772   3.15 GB/s
BM_LargeStruct_Fixed             3.63 ns         3.63 ns  192485371  20.5 GB/s
BM_Vec_Deserialize               4.39 ns         4.39 ns  159286397   2.55 GB/s
BM_LargeStruct_Deserialize       2.66 ns         2.66 ns  263341045  28.0 GB/s
BM_StringPayload/0               8.32 ns         8.32 ns   84245998   917 MB/s
BM_StringPayload/8               14.3 ns         14.3 ns   49869982   1.04 GB/s
BM_StringPayload/32              20.2 ns         18.4 ns   37370804   2.02 GB/s
BM_StringPayload/128             15.4 ns         15.3 ns   42865104   8.26 GB/s
BM_StringPayload/1024            22.4 ns         22.4 ns   31083619  43.0 GB/s
BM_StringPayload/4096            78.8 ns         78.8 ns    8811349  48.5 GB/s
BM_Vec_FreshAllocation           30.7 ns         30.7 ns   22654528   373 MB/s
```

**Reading the numbers:**

- `BM_Vec_Fast` vs `BM_Vec_Memcpy`: 7.40 ns vs 7.36 ns. The fast path is statistically indistinguishable from a raw `memcpy`. The `is_trivially_copyable` dispatch is free at runtime — the `if constexpr` branch resolves at compile time.

- `BM_Vec_Naive` at 19.7 ns: the naive serializer checks `Reflectable` *before* `is_trivially_copyable`, so even a trivially-copyable `Vec` enters the reflect path and suffers three separate `buffer.write()` calls instead of one. **2.7× slower** than the fast path.

- `BM_LargeStruct_Fast` vs `BM_LargeStruct_Naive`: 8.96 ns vs 75.5 ns. 80 bytes in one `memcpy` vs 20 individual field writes. **8.4× gap** — the penalty scales with field count, not struct size.

- `BM_Vec_Fixed` at 3.54 ns: `FixedBuffer` lives on the stack. No heap allocation, no capacity check beyond a single size comparison. **2.1× faster** than heap-backed `Buffer` for the same struct.

- `BM_LargeStruct_Fixed` at 3.63 ns / 20.5 GB/s: The 80-byte struct on a stack buffer pushes throughput close to memory bandwidth limits. Overhead approaches zero.

- `BM_Vec_Deserialize` faster than `BM_Vec_Fast`? Not quite — the pre-serialized buffer stays cache-hot across iterations, removing the write-path overhead. Both are a single `memcpy`.

- `BM_StringPayload/0` at 8.32 ns: An empty string still writes an 8-byte `uint64_t` length prefix. The fixed overhead of the string path is ~8 ns regardless of content.

- `BM_StringPayload/128` at 15.3 ns / 8.26 GB/s vs `BM_StringPayload/8` at 14.3 ns / 1.04 GB/s: Both have the same 8 ns fixed overhead. At 128 bytes, `memcpy` efficiency dominates and throughput jumps 8×.

- `BM_Vec_FreshAllocation` at 30.7 ns vs `BM_Vec_Fast` at 7.40 ns: A cold `Buffer` allocation costs ~23 ns per call. Pre-reserving and reusing a `Buffer` — or switching to `FixedBuffer` — eliminates this entirely.

---

### `benchmark_stress` — Sustained Throughput at Scale

Serializes large batches of a `Tick` struct (a realistic market data message: `uint64_t timestamp_ns`, `uint32_t id`, `double price`, `uint32_t quantity`, `bool buy`). Tests include a million-item stream, a burst-traffic pattern, and a `FixedBuffer` batch path.

```bash
./build/benchmark/benchmark_stress
```

**Results:**

```
Benchmark                    Time             CPU   Iterations  Throughput
--------------------------------------------------------------------------
BM_Tick               12788526 ns     12611981 ns           52  79.3M ticks/s
BM_Tick_Naive         26076708 ns     26054481 ns           27  38.4M ticks/s
BM_BurstTraffic        1259905 ns      1258520 ns          548  79.5M ticks/s
BM_Tick_FixedBuffer        937 ns          933 ns       747823  274M ticks/s
```

- `BM_Tick` vs `BM_Tick_Naive`: 1M heterogeneous ticks (randomized fields). Fast path: **79.3M ticks/s**. Naive: **38.4M ticks/s**. The `Tick` struct is trivially copyable, so the fast path memcpy's all 29 bytes (+ padding) in a single call while naive issues 5 separate writes. **2.1× difference** sustained over 1M iterations.

- `BM_BurstTraffic`: Simulates a normal batch of 1,000 followed by a burst of 100,000 in the same benchmark iteration. Throughput holds at 79.5M ticks/s — consistent with the steady-state `BM_Tick` result, confirming there is no spike penalty from the sudden load increase when the buffer is pre-reserved.

- `BM_Tick_FixedBuffer`: 256-tick batch on a stack-allocated `FixedBuffer`. **274M ticks/s** — **3.5× faster** than heap-backed `Buffer`. This represents the ceiling for this struct at this batch size: no allocator, no capacity growth, pure `memcpy` throughput.

**Struct layout note:** `Tick` has a `bool buy` field at the end. `bool` is 1 byte but the struct's natural alignment pads it to `sizeof(Tick) = 29` bytes (or more depending on the compiler). The serializer bulk-copies the full `sizeof(Tick)` including padding — which is what makes it fast, and also what makes the wire representation non-portable.

---

### `benchmark_latency` — Per-Message Latency Distribution

Measures per-message serialize time using batched timing (256 messages per timing sample) over 1M samples. Reports mean, p50, p95, p99, p999, and max. Uses `mach_absolute_time()` on macOS for sub-nanosecond resolution. A 10,000-iteration warmup precedes measurement to stabilize caches and branch predictors.

```bash
./build/benchmark/benchmark_latency
```

**Results:**

```
fast path
Mean : 2.29 ns
p50  : 2.28 ns
p95  : 2.60 ns
p99  : 3.09 ns
p999 : 3.58 ns
Max  : 183  ns

naive
Mean : 6.37 ns
p50  : 6.35 ns
p95  : 6.67 ns
p99  : 8.63 ns
p999 : 9.77 ns
Max  : 993  ns
```

- **Fast path mean 2.29 ns, p99 3.09 ns:** The 0.81 ns spread from p50 to p99 reflects pure measurement noise and cache-line eviction variance. The fast path has no branching, no allocation, and no heap access — there is nothing to introduce latency spikes.

- **Naive mean 6.37 ns, p99 8.63 ns:** The 2.28 ns spread from p50 to p99 is wider in both absolute and relative terms. Five separate `buffer.write()` calls means five opportunities for cache interference. The 993 ns max spike reflects a rare OS preemption hitting between field writes.

- **Max outlier ratio:** Fast path max (183 ns) / p999 (3.58 ns) = **51×**. Naive max (993 ns) / p999 (9.77 ns) = **102×**. The fast path's max outlier is proportionally half as severe — fewer write calls means fewer preemption windows.

---

## Test Structs (`common.hpp`)

All benchmarks share these types:

| Struct | Fields | Size | Notes |
|---|---|---|---|
| `Vec` | `float x, y, z` | 12 B | Trivially copyable, also Reflectable |
| `LargeStruct` | `float f0…f19` | 80 B | Trivially copyable, 20 fields |
| `Nested` | `Vec posn, Vec vel, float[4], uint64_t` | 44 B | Trivially copyable, nested trivials |
| `Mixed` | `Nested, std::string, uint64_t` | variable | Not trivially copyable (string member) |
| `StringPayload` | `std::string` | variable | Reflectable wrapper around a string |
| `Tick` | `uint64_t, uint32_t, double, uint32_t, bool` | 29 B + padding | Trivially copyable; used in stress/latency |

---

## Benchmark Design Notes

**Why `DoNotOptimize` and `ClobberMemory`?**  
Without `benchmark::DoNotOptimize(buffer.data())`, the compiler can prove the serialized bytes are never read and eliminate the write entirely. `ClobberMemory()` flushes the compiler's view of memory state between iterations, preventing cross-iteration optimization across `buffer.clear()`.

**Why mutate fields each iteration?**  
The `FixedBuffer` benchmarks increment `tick.timestamp_ns`, `tick.id`, etc. inside the loop. This prevents the optimizer from scalarizing the struct into registers and issuing a `memcpy` from a compile-time constant — which would not reflect real-world behavior.

**Why batch timing in `benchmark_latency`?**  
A single `mach_absolute_time()` call costs ~3–5 ns on Apple Silicon. Timing one serialization call individually would measure the timer overhead, not the serializer. Batching 256 calls per sample amortizes timer cost to < 0.02 ns per measurement.

**Why `FixedBuffer<batch_size * sizeof(Tick)>`?**  
The buffer is sized to hold exactly one batch. It never grows, never reallocates, and `clear()` is a single `size_ = 0` assignment — not a destructive operation. Matching capacity to batch size also keeps the buffer in L1 cache for small structs.

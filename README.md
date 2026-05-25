# bin_serializer

A C++23 binary serialization library designed for latency-sensitive systems. It trades wire-size compactness for speed by using fixed-width, native-endian encoding and exploiting trivially-copyable memory layouts to replace field-by-field writes with single `memcpy` calls. Designed for homogeneous deployment environments — same architecture, same compiler — where serialization sits on the critical path and every nanosecond counts.

---

## How It Works

### Dispatch Path 1 — Trivially Copyable Types

`std::is_trivially_copyable_v<T>` is a compile-time guarantee that the object's value representation is fully contained in its bytes: no vtable, no internal pointers, no allocator state. For any type that satisfies this trait, the serializer issues a single `buffer.write(&field, sizeof(T))` call — one `memcpy` to flatten the entire object into the wire buffer. This eliminates per-field dispatch entirely. The check is evaluated first in the `if constexpr` chain, so a `Vec{float x, y, z}` with a `reflect()` method still hits this path. On the 80-byte `LargeStruct`, this reduces 20 separate field writes to a single bulk copy, yielding **8.4×** throughput over the naive field-by-field path.

### Dispatch Path 2 — `std::string`

`std::string` cannot be bulk-copied: its value lives on the heap, not inside the object's byte representation. The slow path serializes a `uint64_t` length prefix followed by the raw character bytes. On deserialization, the length is read first, `field.resize(size)` allocates the necessary storage, then the character bytes are read directly into the string's buffer. This is the only path that touches the heap at serialization time, and throughput scales with string length due to `memcpy` efficiency on large payloads.

### Dispatch Path 3 — Nested Reflectable Structs

A type is `Reflectable` if it exposes a `reflect(visitor)` method that passes all its fields as a variadic pack to the visitor. The serializer recurses into the pack using a fold expression over `&&`: `(... && serialize_field(buffer, fields))`. The `&&` fold guarantees left-to-right evaluation order (field layout order on the wire) and short-circuits on the first failure, so a buffer-overflow in a deeply nested field stops serialization immediately without corrupting later fields. Crucially, recursion bottoms out at the trivially-copyable check — a nested `Vec` inside a `Player` still hits the fast path. The `Reflectable` path adds no overhead beyond function call depth for trivial leaf fields.

---

## Wire Format

The format is intentionally minimal: no magic bytes, no version header, no field tags. Fields appear in the order `reflect()` exposes them. The receiver must know the schema.

| Type | Wire Layout | Size |
|---|---|---|
| `bool` | 1 byte, value 0 or 1 | 1 B |
| `uint8_t` / `int8_t` | raw byte, native endian | 1 B |
| `uint32_t` / `int32_t` / `float` | raw bytes, native endian | 4 B |
| `uint64_t` / `int64_t` / `double` | raw bytes, native endian | 8 B |
| Any trivially copyable `T` | `sizeof(T)` raw bytes, including any struct padding | `sizeof(T)` B |
| `float arr[N]` (C-style array) | `N × sizeof(float)` raw bytes, bulk-copied | `N × 4` B |
| `std::string` | `[uint64_t length][length raw bytes]` | `8 + length` B |
| Nested `Reflectable` struct | Fields serialized recursively, in `reflect()` order | sum of field sizes |

Struct padding bytes are included verbatim for trivially copyable types — the entire `sizeof(T)` is copied. There is no padding stripping.

---

## Benchmark Results

Measured on Apple Silicon (ARM64), Release build (`-O2`), Google Benchmark. All timings are per-operation wall-clock time.

| Benchmark | Time | Throughput | Notes |
|---|---|---|---|
| `BM_Vec_Fast` | 7.40 ns | 1.51 GB/s | Single `memcpy` for 12-byte struct |
| `BM_Vec_Memcpy` | 7.36 ns | 1.52 GB/s | Raw `memcpy` baseline — fast path matches it |
| `BM_Vec_Naive` | 19.7 ns | 585 MB/s | Field-by-field dispatch; 2.7× slower than fast path |
| `BM_Vec_Fixed` | 3.54 ns | 3.15 GB/s | `FixedBuffer` eliminates heap allocation; 2.1× faster than `Buffer` |
| `BM_LargeStruct_Fast` | 8.96 ns | 8.32 GB/s | 80 bytes in one shot — throughput scales with payload size |
| `BM_LargeStruct_Naive` | 75.5 ns | 1.01 GB/s | 20 separate field writes; **8.4× slower** than fast path |
| `BM_LargeStruct_Fixed` | 3.63 ns | 20.5 GB/s | Stack buffer + trivial type = near-optimal memory bandwidth |
| `BM_Vec_Deserialize` | 4.39 ns | 2.55 GB/s | Deserialization mirrors serialization; one `memcpy` |
| `BM_LargeStruct_Deserialize` | 2.66 ns | 28.0 GB/s | Large read amortizes fixed overhead; fastest operation |
| `BM_StringPayload/0` | 8.32 ns | 917 MB/s | Empty string: only the 8-byte length prefix |
| `BM_StringPayload/8` | 14.3 ns | 1.04 GB/s | Short string: length prefix + small copy |
| `BM_StringPayload/128` | 15.3 ns | 8.26 GB/s | `memcpy` efficiency kicks in at moderate sizes |
| `BM_StringPayload/1024` | 22.4 ns | 43.0 GB/s | Large string: throughput dominated by bulk copy |
| `BM_StringPayload/4096` | 78.8 ns | 48.5 GB/s | 4 KB string at near-memory-bandwidth |
| `BM_Vec_FreshAllocation` | 30.7 ns | 373 MB/s | Cold heap alloc per call; ~23 ns of overhead vs pre-reserved `Buffer` |
| `BM_Tick` (1M ticks) | 12.6 ms | 79.3M ticks/s | Mixed struct with 5 trivial fields; sustained throughput |
| `BM_Tick_Naive` (1M ticks) | 26.1 ms | 38.4M ticks/s | Same struct, field-by-field; **2.1× slower** |
| `BM_Tick_FixedBuffer` (256-tick batch) | 933 ns | 274M ticks/s | Stack buffer eliminates all heap traffic |

**Latency distribution** (1M samples, `FixedBuffer`, `Tick` struct):

| Percentile | Fast Path | Naive |
|---|---|---|
| p50 | 2.28 ns | 6.35 ns |
| p95 | 2.60 ns | 6.67 ns |
| p99 | 3.09 ns | 8.63 ns |
| p999 | 3.58 ns | 9.77 ns |
| Max | 183 ns | 993 ns |

The fast path's p50–p99 spread is only 0.81 ns; the naive path's spread is 2.28 ns. Tail latency is tighter at every percentile when you avoid per-field dispatch.

---

## Design Decisions

**Fixed-width encoding → no varint complexity.**  
Every primitive occupies exactly `sizeof(T)` bytes on the wire. There is no variable-length integer encoding, no zigzag, no bit-packing. This eliminates branch-heavy decode loops and makes every field's wire offset statically computable. The cost is larger wire size for small integers — a tradeoff that favors CPU time over bandwidth.

**Native endianness → no byte-swapping overhead.**  
The serializer writes bytes in the host's native byte order and assumes the receiver shares it. This removes the conditional swap from every field, which is meaningful at high message rates. The library is intentionally scoped to homogeneous deployments; cross-endian support would require a different design choice at the concept layer.

**No exceptions → `bool` return everywhere.**  
Every `serialize` and `deserialize` call returns `bool`. Failure propagates through `&&` fold expressions and explicit `if(!success)` guards. This keeps the serializer safe for use in environments where exceptions are disabled (embedded, latency-critical paths, `-fno-exceptions` builds) and makes the error contract visible in the type system.

**`reflect()` as the metadata interface → no macros, no code generation.**  
Struct field layout is described by a templated `reflect(Visitor&&)` method. The visitor receives all fields as a variadic pack. This is pure library-side C++ — no preprocessor, no external schema file, no build step beyond the compiler. Adding a field means adding it to `reflect()`. Removing it means removing it from both the struct and `reflect()`. The `Reflectable` concept enforces that both const and non-const overloads exist at compile time.

**`FixedBuffer<N>` for zero-allocation paths.**  
`FixedBuffer<N>` is a stack-allocated `std::array<uint8_t, N>` with a size cursor. `write()` and `read()` are `memcpy` calls with a bounds check and no heap involvement. For fixed-schema messages with a known maximum size, it eliminates every allocator call from the hot path. The `BM_Tick_FixedBuffer` benchmark achieves 274M ticks/s against 79M ticks/s for heap-backed `Buffer` — a 3.5× difference driven purely by allocation elimination.

**`BufferLike` concept → compile-time buffer type switching.**  
The serializer and deserializer are templated on any type satisfying `BufferLike`. Switching from `Buffer` to `FixedBuffer` — or to a custom memory-mapped buffer — requires changing one type at the call site with zero runtime cost and no virtual dispatch.

---

## Limitations

**No `std::vector` or dynamic-length sequences.** Only `std::string` is treated as a non-trivial type with a known length-prefix protocol. A `std::vector<float>` field will fail to compile — there is no wire format defined for it.

**No pointer types.** Pointers are trivially copyable but serializing their value (an address) is meaningless. There is no check for this; passing a struct containing a pointer will compile and silently serialize the raw address value.

**No cross-architecture portability.** Native endianness and native `sizeof` values are baked into every wire byte. A `uint32_t` serialized on little-endian x86 cannot be correctly deserialized on big-endian SPARC.

**Failed `serialize` leaves the buffer dirty.** If serialization of field N fails (e.g., `FixedBuffer` overflow), fields 0 through N-1 have already been written. There is no rollback. Reading from a buffer after a failed serialize produces partial, malformed data. The `test_rountrip_property.cpp` test `"Failed serialization leaves partial bytes"` documents this behavior explicitly.

**No struct padding stripping.** Trivially copyable structs are bulk-copied including any compiler-inserted padding bytes. Two structs with the same named fields but different packing will produce different wire bytes.

**No versioning or schema evolution.** There is no version field, no field tag, no optional field marker. Adding, removing, or reordering a field in `reflect()` silently breaks wire compatibility with any previously serialized data.

---

## Building

**Requirements:** C++23-capable compiler (Clang 17+ or GCC 13+), CMake 3.30+, internet access for dependency fetch (Catch2, Google Benchmark).

```bash
git clone https://github.com/<your-username>/Binary-Serialization-Engine.git
cd Binary-Serialization-Engine

# Configure (Release for benchmarks, Debug adds ASan + UBSan)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build everything
cmake --build build --config Release -j$(nproc)   # Linux
cmake --build build --config Release -j$(sysctl -n hw.logicalcpu)  # macOS
```

### Run Tests

```bash
# Individual test executables (Catch2)
./build/tests/test_fixedbuffer
./build/tests/test_rountrip_property
./build/tests/test_serialize_deserialize
./build/tests/test_string
./build/tests/test_trivial
```

### Run Benchmarks

```bash
# Throughput comparison: fast path vs naive vs FixedBuffer
./build/benchmark/benchmark_compare

# Sustained throughput: 1M tick stream
./build/benchmark/benchmark_stress

# Latency distribution: p50 / p95 / p99 / p999
./build/benchmark/benchmark_latency
```

### Debug Build (with sanitizers)

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j$(nproc)
./build-debug/tests/test_rountrip_property
```

---

## Project Layout

```
bin_serializer/
├── include/bin_serializer/
│   ├── buffer_concept.hpp   # BufferLike concept
│   ├── buffer.hpp           # Heap-backed Buffer (std::vector<uint8_t>)
│   ├── fixed_buffer.hpp     # Stack-backed FixedBuffer<N>
│   ├── reflect.hpp          # Reflectable concept
│   ├── serializable.hpp     # Serializable concept
│   ├── serializer.hpp       # serialize() and serialize_field()
│   └── deserializer.hpp     # deserialize() and deserialize_field()
├── src/
│   └── buffer.cpp           # Buffer implementation
├── tests/                   # Catch2 test suite
├── benchmark/               # Google Benchmark suite
└── CMakeLists.txt
```

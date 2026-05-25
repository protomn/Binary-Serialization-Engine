# Tests

Test suite for `bin_serializer`, built on [Catch2 v3.15](https://github.com/catchorg/Catch2). Five test executables cover buffer behavior, type dispatch, roundtrip correctness across edge cases, and explicit failure-mode documentation. All tests pass in both Release and Debug (ASan + UBSan) builds.

---

## Building

From the repo root:

```bash
# Release
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# Debug — enables AddressSanitizer and UBSanitizer
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j$(nproc)
```

---

## Running

Each test file compiles to a standalone Catch2 executable. Run them individually:

```bash
./build/tests/test_fixedbuffer
./build/tests/test_rountrip_property
./build/tests/test_serialize_deserialize
./build/tests/test_string
./build/tests/test_trivial
```

Pass `--reporter compact` for a condensed view, or `--list-tests` to enumerate all test cases:

```bash
./build/tests/test_rountrip_property --list-tests
./build/tests/test_rountrip_property --reporter compact
```

---

## Test Files

### `test_trivial.cpp` — Trivially Copyable Field Serialization

**1 test case, 3 assertions.**

Verifies the fast path at the byte level. Serializes `TrivialPoint{float x, float y, uint32_t id}` and checks that:
- `buffer.size()` equals `sizeof(float) * 2 + sizeof(uint32_t)`
- The raw bytes at offset 0 match `&point.x` exactly (via `memcmp`)
- The raw bytes at offset 8 match `&point.id` exactly

This is a protocol-level test: it pins the wire representation to the struct's in-memory layout and will catch any accidental byte-swapping, field reordering, or padding insertion.

**Note:** `TrivialPoint::reflect()` does not return the visitor's result (returns `void`). The serializer accepts this — the `Reflectable` concept only requires the method to exist; the struct still hits the `is_trivially_copyable_v` branch first and is serialized as a bulk copy.

---

### `test_string.cpp` — String Field Deserialization Roundtrip

**1 test case, 3 assertions.**

Serializes a `Player{uint32_t id, std::string name}` with `id = 42` and `name = "pratham"`, then deserializes and checks field equality. The `Player` struct is not trivially copyable (it contains `std::string`), so it goes through the reflect path, which dispatches `id` to the trivial branch and `name` to the string branch. Tests that the `uint64_t` length prefix and raw character bytes compose and decompose correctly.

---

### `test_serialize_deserialize.cpp` — Reflect-Path Struct Roundtrip

**1 test case, 4 assertions.**

Roundtrip test for a `TrivialPoint` via `deserialize()`. Extends `test_trivial.cpp` by verifying that the deserialized `restored.x`, `restored.y`, and `restored.id` match the original — confirming the deserialization offset arithmetic advances correctly through a multi-field struct.

---

### `test_fixedbuffer.cpp` — FixedBuffer Capacity Enforcement

**2 test cases, 6 assertions.**

Tests `FixedBuffer`'s boundary conditions:

| Test | What it checks |
|---|---|
| `FixedBuffer Roundtrip` | Serialize `Vec` into `FixedBuffer<64>`, deserialize, verify all three fields |
| `FixedBuffer rejects overflow` | Serialize `Vec` (12 bytes) into `FixedBuffer<4>` returns `false` |

The overflow test confirms that `FixedBuffer::write()` rejects writes that would exceed capacity before writing any bytes — preventing partial writes that corrupt the buffer for subsequent reads.

---

### `test_rountrip_property.cpp` — Property-Based Roundtrip and Edge Cases

**17 test cases, 3,461 assertions.**

The primary correctness suite. Uses Catch2's `GENERATE` macro for data-driven cases across boundary values. Covers the full type dispatch tree, both buffer types, and explicitly documents known failure behaviors.

#### Roundtrip Correctness

| Test | Types | Values |
|---|---|---|
| `uint64_t Roundtrip Test` | `uint64_t` | `0`, `1`, `UINT64_MAX`, `0xDEADBEEFCAFEBABE`, `42` |
| `float Roundtrip Test` | `float` | `0.0f`, `-0.0f`, `1.0f`, `-1.0f`, `+inf`, `-inf`, `NaN` |
| `Vec Roundtrip` | `Vec{float,float,float}` | 5 × 5 × 5 = 125 combinations across {-1000, -1, 0, 1, 1000} |
| `Transform Roundtrip Test` | `Transform{Vec, Vec, uint64_t}` | 4 timestamp values |
| `Player Roundtrip Test` | `Player{Transform, string, uint64_t}` | 4 string values: `""`, `"a"`, `"pratham"`, 128 `'x'` |

The `float` roundtrip uses `memcmp` rather than `==` to handle `NaN` (which does not compare equal to itself). The test verifies bit-exact preservation of the IEEE 754 representation, including the sign bit of `-0.0f`.

#### Buffer Type Correctness

| Test | What it checks |
|---|---|
| `Fixed-buffer Exact Fit Test` | `sizeof(Vec)` capacity: serialize returns true, `buffer.size() == sizeof(Vec)` |
| `Fixed Buffer 1-Byte Overflow` | `sizeof(Vec) - 1` capacity: serialize returns false |
| `Truncated Buffer Test` | Serializes into a full `Buffer`, copies `sizeof(Vec) - 1` bytes to a `FixedBuffer`, deserialize returns false |
| `FixedBuffer Reuse Correctness` | 1,000 clear/serialize/deserialize cycles on the same `FixedBuffer<128>`, checks field equality each iteration |
| `Raw Byte Equivalency` | `buffer.size() == sizeof(Vec)` and `memcmp(buffer.data(), &vec, sizeof(Vec)) == 0` |
| `Cross buffer byte equivalence` | Serializes `Player` into both `Buffer` and `FixedBuffer<1024>`, checks `size` and `memcmp` match |

`Cross buffer byte equivalence` is the portability invariant: both buffer types must produce identical wire bytes for the same input. Any divergence would indicate a buffer-specific encoding path.

#### Failure Mode Documentation

These tests document behaviors that callers must account for — they pass precisely because the described behavior is correct-by-design.

| Test | Documented behavior |
|---|---|
| `FixedBuffer String Payload Overflow Fails.` | `FixedBuffer` with capacity for `uint64_t + 4` bytes rejects a 6-char string: the length prefix fits, the chars don't; returns `false` |
| `Nested string overflow propagates failure` | A `Player` with a 64-char string into `FixedBuffer<64>` fails; the `&&` fold short-circuits at the string field |
| `Failed serialization leaves partial bytes` | After a failed serialize of a 32-byte string into a `FixedBuffer<sizeof(uint64_t)>`, `buffer.size() == sizeof(uint64_t)` — the length prefix was written before the failure |
| `Failure of corrupted string length` | A `Buffer` containing only a `uint64_t` fake size of `1,000,000` with no payload bytes: `deserialize` returns `false` rather than reading out of bounds |
| `Correct serialization of empty string` | `std::string{}` serializes to exactly 8 bytes (the zero-valued length prefix); `buffer.size() == sizeof(uint64_t)` |
| `Deserializer reuse overwrites previous state` | Deserializing a 128-char string into a `std::string op{"pnayak"}` replaces the prior content: `op == second` |

The `"Failed serialization leaves partial bytes"` test is particularly important for callers using `FixedBuffer`: a failed `serialize` does not reset the buffer. The caller must call `buffer.clear()` before retrying.

#### Compile-Time Concept Checks

```cpp
static_assert(bin_serializer::BufferLike<bin_serializer::Buffer>);
static_assert(bin_serializer::BufferLike<bin_serializer::FixedBuffer<128>>);
static_assert(bin_serializer::Serializable<Vec>);
static_assert(bin_serializer::Serializable<Player>);
static_assert(bin_serializer::Serializable<std::string>);
```

These `static_assert` statements at file scope verify the concept machinery at compile time. If any of these fail to compile, the type system has broken an invariant — not the runtime logic.

---

## Test Summary

| Executable | Test Cases | Assertions | Covers |
|---|---|---|---|
| `test_trivial` | 1 | 3 | Fast-path byte layout |
| `test_string` | 1 | 3 | String roundtrip via reflect path |
| `test_serialize_deserialize` | 1 | 4 | Reflect-path deserialization |
| `test_fixedbuffer` | 2 | 6 | FixedBuffer capacity enforcement |
| `test_rountrip_property` | 17 | 3,461 | Property-based roundtrip, failure modes, concept checks |
| **Total** | **22** | **3,477** | |

All 3,477 assertions pass on Apple Silicon (ARM64, macOS 15), Release and Debug builds.

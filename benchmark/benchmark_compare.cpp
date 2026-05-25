#include "common.hpp"
#include "naive_serializer.hpp"
#include "helpers.hpp"

#include "../include/bin_serializer/buffer.hpp"
#include "../include/bin_serializer/fixed_buffer.hpp"
#include "../include/bin_serializer/serializer.hpp"
#include "../include/bin_serializer/deserializer.hpp"

#include "benchmark/benchmark.h"

#include <cstring>

static void memcpy_baseline(
    bin_serializer::Buffer &buffer,
    const auto &val)
{
    (void)buffer.write(&val, sizeof(val));
}

static void BM_Vec_Fast(
    benchmark::State &state)
{
    Vec vec{1.0f, 2.0f, 3.0f};

    bin_serializer::Buffer buffer;
    buffer.reserve(sizeof(Vec));

    for(auto _ : state)
    {
        buffer.clear();
        bin_serializer::serialize(
            buffer,
            vec
        );

        benchmark::DoNotOptimize(
        buffer.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * sizeof(Vec));
    state.SetItemsProcessed(state.iterations());
}

static void BM_Vec_Naive(
    benchmark::State &state)
{
    Vec vec{1.0f, 2.0f, 3.0f};

    bin_serializer::Buffer buffer;
    buffer.reserve(sizeof(Vec));

    for(auto _ : state)
    {
        buffer.clear();

        naive_serializer::serialize(buffer, vec);

        benchmark::DoNotOptimize(buffer.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * sizeof(Vec));
    state.SetItemsProcessed(state.iterations());
}

static void BM_Vec_Memcpy(
    benchmark::State &state)
{
    Vec vec{1.0f, 2.0f, 3.0f};

    bin_serializer::Buffer buffer;
    buffer.reserve(sizeof(Vec));

    for(auto _ : state)
    {
        buffer.clear();

        memcpy_baseline(
            buffer,
            vec
        );

        benchmark::DoNotOptimize(
            buffer.data()
        );
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * sizeof(Vec));
    state.SetItemsProcessed(state.iterations());
}

static void BM_LargeStruct_Fast(
    benchmark::State &state)
{
    LargeStruct ls{};

    bin_serializer::Buffer buffer;
    buffer.reserve(sizeof(LargeStruct));

    for(auto _ : state)
    {
        buffer.clear();

        bin_serializer::serialize(buffer, ls);

        benchmark::DoNotOptimize(
            buffer.data()
        );
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * sizeof(LargeStruct));
    state.SetItemsProcessed(state.iterations());
}

static void BM_LargeStruct_Naive(
    benchmark::State &state)
{
    LargeStruct ls;

    bin_serializer::Buffer buffer;
    buffer.reserve(sizeof(LargeStruct));

    for(auto _ : state)
    {
        buffer.clear();

        naive_serializer::serialize(buffer, ls);

        benchmark::DoNotOptimize(
            buffer.data()
        );
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * sizeof(LargeStruct));
    state.SetItemsProcessed(state.iterations());
}

static void BM_Vec_Fixed(
    benchmark::State& state)
{
    Vec vec{1.f, 2.f, 3.f};

    bin_serializer::FixedBuffer<128> buffer;

    for (auto _ : state)
    {
        buffer.clear();

        benchmark::DoNotOptimize(vec);

        bin_serializer::serialize(buffer, vec);

        benchmark::DoNotOptimize(
            buffer.data()
        );

        benchmark::ClobberMemory();

        vec.x += 1.f;
    }

    state.SetBytesProcessed(
        state.iterations() * sizeof(Vec)
    );

    state.SetItemsProcessed(
        state.iterations()
    );
}

static void BM_LargeStruct_Fixed(
    benchmark::State& state)
{
    LargeStruct ls{};

    bin_serializer::FixedBuffer<1024> buffer;

    for (auto _ : state)
    {
        buffer.clear();

        benchmark::DoNotOptimize(ls);

        bin_serializer::serialize(buffer, ls);

        benchmark::DoNotOptimize(
            buffer.data()
        );

        benchmark::ClobberMemory();

        ls.f0 += 1.f;
    }

    state.SetBytesProcessed(
        state.iterations() *
        sizeof(LargeStruct)
    );

    state.SetItemsProcessed(
        state.iterations()
    );
}

static void BM_Vec_Deserialize(
    benchmark::State& state)
{
    Vec input{1.f, 2.f, 3.f};

    bin_serializer::Buffer buffer;
    buffer.reserve(sizeof(Vec));

    bin_serializer::serialize(
        buffer,
        input
    );

    for (auto _ : state)
    {
        Vec output{};

        benchmark::DoNotOptimize(output);

        bin_serializer::deserialize(
            buffer,
            output
        );

        benchmark::DoNotOptimize(output);

        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() * sizeof(Vec)
    );

    state.SetItemsProcessed(
        state.iterations()
    );
}

static void BM_LargeStruct_Deserialize(
    benchmark::State& state)
{
    LargeStruct input{};

    bin_serializer::Buffer buffer;
    buffer.reserve(sizeof(LargeStruct));

    bin_serializer::serialize(
        buffer,
        input
    );

    for (auto _ : state)
    {
        LargeStruct output{};

        benchmark::DoNotOptimize(output);

        bin_serializer::deserialize(
            buffer,
            output
        );

        benchmark::DoNotOptimize(output);

        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() *
        sizeof(LargeStruct)
    );

    state.SetItemsProcessed(
        state.iterations()
    );
}

static void BM_StringPayload(
    benchmark::State& state)
{
    const std::size_t size =
        static_cast<std::size_t>(
            state.range(0)
        );

    StringPayload payload{
        std::string(size, 'x')
    };

    bin_serializer::Buffer buffer;

    buffer.reserve(
        sizeof(uint64_t) + size
    );

    const bool success =
    bin_serializer::serialize(
        buffer,
        payload);

    if (!success)
    {
        state.SkipWithError(
            "Serialization failed"
        );
    }

    for (auto _ : state)
    {
        buffer.clear();

        benchmark::DoNotOptimize(payload);

        bin_serializer::serialize(
            buffer,
            payload
        );

        benchmark::DoNotOptimize(
            buffer.data()
        );

        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() *
        (sizeof(uint64_t) + size)
    );

    state.SetItemsProcessed(
        state.iterations()
    );
}

static void BM_Vec_FreshAllocation(
    benchmark::State& state)
{
    Vec vec{1.f, 2.f, 3.f};

    for (auto _ : state)
    {
        bin_serializer::Buffer buffer;

        bin_serializer::serialize(
            buffer,
            vec
        );

        benchmark::DoNotOptimize(
            buffer.data()
        );

        benchmark::ClobberMemory();

        vec.x += 1.f;
    }

    state.SetBytesProcessed(
        state.iterations() * sizeof(Vec)
    );

    state.SetItemsProcessed(
        state.iterations()
    );
}

BENCHMARK(BM_Vec_Fast);
BENCHMARK(BM_Vec_Naive);
BENCHMARK(BM_Vec_Memcpy);

BENCHMARK(BM_LargeStruct_Fast);
BENCHMARK(BM_LargeStruct_Naive);

BENCHMARK(BM_Vec_Fixed);
BENCHMARK(BM_LargeStruct_Fixed);

BENCHMARK(BM_Vec_Deserialize);
BENCHMARK(BM_LargeStruct_Deserialize);

BENCHMARK(BM_StringPayload)
    ->Arg(0)
    ->Arg(8)
    ->Arg(32)
    ->Arg(128)
    ->Arg(1024)
    ->Arg(4096);

BENCHMARK(BM_Vec_FreshAllocation);

BENCHMARK_MAIN();
#pragma once

#include <benchmark/benchmark.h>

template<typename Serializer, typename Buffer, typename T>
inline void benchmark_serialize(
    benchmark::State& state,
    Buffer& buffer,
    T& value,
    Serializer serializer)
{
    for (auto _ : state)
    {
        buffer.clear();

        benchmark::DoNotOptimize(value);

        serializer(buffer, value);

        benchmark::DoNotOptimize(buffer.data());
        benchmark::ClobberMemory();

        ++value.x;
    }

    state.SetBytesProcessed(
        state.iterations() * sizeof(T)
    );

    state.SetItemsProcessed(
        state.iterations()
    );
}


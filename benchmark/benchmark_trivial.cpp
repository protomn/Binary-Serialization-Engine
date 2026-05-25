#include <benchmark/benchmark.h>

static void BM(benchmark::State &state)
{
    for(auto _ : state)
    {
        benchmark::DoNotOptimize(state.iterations());
    }
}

BENCHMARK(BM);
BENCHMARK_MAIN();
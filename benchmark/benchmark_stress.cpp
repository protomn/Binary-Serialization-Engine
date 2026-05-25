#include "../include/bin_serializer/buffer.hpp"
#include "../include/bin_serializer/fixed_buffer.hpp"
#include "../include/bin_serializer/serializer.hpp"
#include "naive_serializer.hpp"

#include <benchmark/benchmark.h>

#include <vector>
#include <random>

struct Tick
{
    uint64_t timestamp_ns;
    uint32_t id;
    double price;
    uint32_t quantity;
    bool buy;

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor)
    {
        return visitor(
            timestamp_ns,
            id,
            price,
            quantity,
            buy
        );
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor) const
    {
        return visitor(
            timestamp_ns,
            id,
            price,
            quantity,
            buy
        );
    }
};

static void BM_Tick(
    benchmark::State &state)
{
    std::vector<Tick> ticks(1'000'000);

    std::mt19937_64 rng(42);

    std::uniform_int_distribution<uint64_t> time_dist(
        1'700'000'000'000'000ULL,
        1'800'000'000'000'000ULL
    );

    std::uniform_int_distribution<uint32_t> id_dist(
        1,
        10'000
    );

    std::uniform_real_distribution<double> price_dist(
        50.0,
        500.0
    );

    std::uniform_int_distribution<uint32_t> quant_dist(
        1,
        10'000
    );

    std::bernoulli_distribution side_dist(0.5);

    for(auto &tick : ticks)
    {
        tick.timestamp_ns = time_dist(rng);
        tick.id = id_dist(rng);
        tick.price = price_dist(rng);
        tick.quantity = quant_dist(rng);
        tick.buy = side_dist(rng);
    }

    bin_serializer::Buffer buffer;
    buffer.reserve(ticks.size() * sizeof(Tick));

    for(auto _ : state)
    {
        buffer.clear();

        for(const auto &tick : ticks)
        {
            bin_serializer::serialize(buffer, tick);
        }

        benchmark::DoNotOptimize(buffer.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() *
        sizeof(Tick) * ticks.size());

    state.SetItemsProcessed(
        state.iterations() *
        ticks.size());
}

static void BM_Tick_Naive(
    benchmark::State &state)
{
    std::vector<Tick> ticks(1'000'000);

    std::mt19937_64 rng(42);

    std::uniform_int_distribution<uint64_t> time_dist(
        1'700'000'000'000'000ULL,
        1'800'000'000'000'000ULL
    );

    std::uniform_int_distribution<uint32_t> id_dist(
        1,
        10'000
    );

    std::uniform_real_distribution<double> price_dist(
        50.0,
        500.0
    );

    std::uniform_int_distribution<uint32_t> quant_dist(
        1,
        10'000
    );

    std::bernoulli_distribution side_dist(0.5);

    for(auto &tick : ticks)
    {
        tick.timestamp_ns = time_dist(rng);
        tick.id = id_dist(rng);
        tick.price = price_dist(rng);
        tick.quantity = quant_dist(rng);
        tick.buy = side_dist(rng);
    }

    bin_serializer::Buffer buffer;
    buffer.reserve(ticks.size() * sizeof(Tick));

    for(auto _ : state)
    {
        buffer.clear();

        for(const auto &tick : ticks)
        {
            naive_serializer::serialize(buffer, tick);
        }

        benchmark::DoNotOptimize(buffer.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() *
        sizeof(Tick) * ticks.size());

    state.SetItemsProcessed(
        state.iterations() *
        ticks.size());
}

static void BM_BurstTraffic(
    benchmark::State& state)
{
    constexpr std::size_t normal_batch = 1000;
    constexpr std::size_t burst_batch  = 100000;

    std::vector<Tick> ticks(burst_batch);

    for (std::size_t i = 0; i < burst_batch; ++i)
    {
        ticks[i] = Tick{
            i,
            static_cast<uint32_t>(i),
            100.0 + i,
            10,
            true
        };
    }

    bin_serializer::Buffer buffer;

    buffer.reserve(
        burst_batch * sizeof(Tick)
    );

    for (auto _ : state)
    {
        buffer.clear();

        for (std::size_t i = 0;
             i < normal_batch;
             ++i)
        {
            bin_serializer::serialize(
                buffer,
                ticks[i]
            );
        }

        for (std::size_t i = 0;
             i < burst_batch;
             ++i)
        {
            bin_serializer::serialize(
                buffer,
                ticks[i]
            );
        }

        benchmark::DoNotOptimize(
            buffer.data()
        );

        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() *
        burst_batch *
        sizeof(Tick)
    );

    state.SetItemsProcessed(
        state.iterations() *
        burst_batch
    );
}

static void BM_Tick_FixedBuffer(
    benchmark::State& state)
{
    Tick tick{
        1'700'000'000'000'000ULL,
        42,
        150.25,
        100,
        true
    };

    constexpr std::size_t batch_size = 256;

    bin_serializer::FixedBuffer<
        batch_size * sizeof(Tick)
    > buffer;

    /*
        Validation step.
    */

    const bool success =
        bin_serializer::serialize(
            buffer,
            tick
        );

    if (!success)
    {
        state.SkipWithError(
            "FixedBuffer serialization failed"
        );

        return;
    }

    buffer.clear();

    for (auto _ : state)
    {
        buffer.clear();

        for (std::size_t i = 0;
             i < batch_size;
             ++i)
        {
            /*
                Mutate fields every iteration
                to prevent aggressive constant
                propagation and scalarization.
            */

            ++tick.timestamp_ns;
            ++tick.id;

            tick.price += 0.01;

            tick.quantity =
                (tick.quantity % 10'000) + 1;

            tick.buy = !tick.buy;

            benchmark::DoNotOptimize(
                tick
            );

            bin_serializer::serialize(
                buffer,
                tick
            );
        }

        benchmark::DoNotOptimize(
            buffer.data()
        );

        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        state.iterations() *
        batch_size *
        sizeof(Tick)
    );

    state.SetItemsProcessed(
        state.iterations() *
        batch_size
    );
}

BENCHMARK(BM_Tick);
BENCHMARK(BM_Tick_Naive);
BENCHMARK(BM_BurstTraffic);
BENCHMARK(BM_Tick_FixedBuffer);
BENCHMARK_MAIN();
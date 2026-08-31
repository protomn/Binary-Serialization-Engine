#include "common.hpp"

#include "../include/bin_serializer/buffer.hpp"
#include "../include/bin_serializer/fixed_buffer.hpp"

#include "../include/bin_serializer/serializer.hpp"

#include "naive_serializer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

#include <benchmark/benchmark.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

struct Tick
{
    uint64_t timestamp_ns;
    uint32_t id;
    double price;
    uint32_t quantity;
    bool buy;

    template<typename Visitor>
    decltype(auto) reflect(Visitor&& visitor)
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
    decltype(auto) reflect(Visitor&& visitor) const
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

/*
    Monotonic high-resolution timer.

    On Apple platforms mach_absolute_time()
    returns raw timebase ticks, which must
    be scaled to nanoseconds.

    Elsewhere steady_clock already counts
    in nanoseconds, so the conversion is
    the identity.
*/
class HighResTimer
{
    public:

    #if defined(__APPLE__)

        HighResTimer()
        {
            mach_timebase_info(
                &timebase_
            );
        }

        uint64_t now() const
        {
            return mach_absolute_time();
        }

        double to_nanoseconds(
            uint64_t ticks
        ) const
        {
            return static_cast<double>(
                ticks *
                timebase_.numer
            ) / timebase_.denom;
        }

    private:

        mach_timebase_info_data_t timebase_{};

    #else

        uint64_t now() const
        {
            return static_cast<uint64_t>(
                std::chrono::duration_cast<
                    std::chrono::nanoseconds
                >(
                    std::chrono::steady_clock::now()
                        .time_since_epoch()
                ).count()
            );
        }

        double to_nanoseconds(
            uint64_t ticks
        ) const
        {
            return static_cast<double>(ticks);
        }

    #endif
};

template<typename Serializer>
void run_latency_benchmark(
    const std::string& name,
    Serializer serializer)
{
    constexpr std::size_t iterations =
        1'000'000;

    /*
        Batch timing is CRITICAL.

        Single serialize calls are
        too fast for stable timing.
    */

    constexpr std::size_t batch_size =
        256;

    std::vector<double> samples;

    samples.reserve(iterations);

    Tick tick{
        1'700'000'000'000'000ULL,
        42,
        150.25,
        100,
        true
    };

    bin_serializer::FixedBuffer<
        batch_size * sizeof(Tick)
    > buffer;

    HighResTimer timer;

    /*
        Validation pass.
    */

    const bool success =
        serializer(
            buffer,
            tick
        );

    if (!success)
    {
        std::cerr
            << "Benchmark setup failed\n";

        return;
    }

    buffer.clear();

    /*
        Warmup phase.

        Eliminates:
        - cold caches
        - startup noise
        - branch predictor cold state
    */

    for (std::size_t i = 0;
         i < 10000;
         ++i)
    {
        buffer.clear();

        serializer(
            buffer,
            tick
        );

        ++tick.timestamp_ns;
        ++tick.id;

        tick.price += 0.0001;

        tick.quantity =
            (tick.quantity % 10000) + 1;

        tick.buy = !tick.buy;
    }

    /*
        Actual measurement phase.
    */

    for (std::size_t i = 0;
         i < iterations;
         ++i)
    {
        buffer.clear();

        const uint64_t start =
            timer.now();

        for (std::size_t j = 0;
             j < batch_size;
             ++j)
        {
            ++tick.timestamp_ns;
            ++tick.id;

            tick.price += 0.0001;

            tick.quantity =
                (tick.quantity % 10000) + 1;

            tick.buy = !tick.buy;

            serializer(
                buffer,
                tick
            );
        }

        const uint64_t end =
            timer.now();

        const double total_ns =
            timer.to_nanoseconds(
                end - start
            );

        const double per_message_ns =
            total_ns /
            static_cast<double>(
                batch_size
            );

        samples.push_back(
            per_message_ns
        );

        /*
            Prevent optimizer games.
        */

        benchmark::DoNotOptimize(
            buffer.data()
        );

        benchmark::ClobberMemory();
    }

    std::sort(
        samples.begin(),
        samples.end()
    );

    auto percentile =
        [&](double p)
        {
            const std::size_t idx =
                static_cast<std::size_t>(
                    p *
                    static_cast<double>(
                        samples.size() - 1
                    )
                );

            return samples[idx];
        };

    const double mean =
        std::accumulate(
            samples.begin(),
            samples.end(),
            0.0
        ) /
        static_cast<double>(
            samples.size()
        );

    const double p50 =
        percentile(0.50);

    const double p95 =
        percentile(0.95);

    const double p99 =
        percentile(0.99);

    const double p999 =
        percentile(0.999);

    const double max =
        samples.back();

    std::cout << "\n";

    std::cout
        << name
        << "\n";

    std::cout
        << "Mean : "
        << mean
        << " ns\n";

    std::cout
        << "p50  : "
        << p50
        << " ns\n";

    std::cout
        << "p95  : "
        << p95
        << " ns\n";

    std::cout
        << "p99  : "
        << p99
        << " ns\n";

    std::cout
        << "p999 : "
        << p999
        << " ns\n";

    std::cout
        << "Max  : "
        << max
        << " ns\n";
}

int main()
{
    run_latency_benchmark(
        "fast path",
        [](auto& buffer,
           const auto& tick)
        {
            return bin_serializer::serialize(
                buffer,
                tick
            );
        }
    );

    run_latency_benchmark(
        "naive",
        [](auto& buffer,
           const auto& tick)
        {
            return naive_serializer::serialize(
                buffer,
                tick
            );
        }
    );

    return 0;
}
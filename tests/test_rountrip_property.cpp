#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "bin_serializer/buffer.hpp"
#include "bin_serializer/fixed_buffer.hpp"
#include "bin_serializer/serializer.hpp"
#include "bin_serializer/deserializer.hpp"

#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <bit>
#include <limits>

struct Vec
{
    float x;
    float y;
    float z;

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor)
    {
        return visitor(x, y, z);
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor) const
    {
        return visitor(x, y, z);
    }

    bool operator==(const Vec &) const = default;
};

struct Transform
{
    Vec position;
    Vec velocity;
    uint64_t timestamp;

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor)
    {
        return visitor(position, velocity, timestamp);
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor) const
    {
        return visitor(position, velocity, timestamp);
    }

    bool operator==(const Transform &) const = default;
};

struct Player
{
    Transform transform;
    std::string name;
    uint64_t score;

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor)
    {
        return visitor(transform, name, score);
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor) const
    {
        return visitor(transform, name, score);
    }

    bool operator==(const Player &) const = default;
};

//helper test func
template<typename BufferType, typename T>
void roundtrip_test(const T &input)
{
    BufferType buffer;

    REQUIRE(
        bin_serializer::serialize(
            buffer, 
            input)
        );

    T output{};

    REQUIRE(
        bin_serializer::deserialize(
            buffer,
            output)
        );

    REQUIRE(output == input);
}

static_assert(bin_serializer::BufferLike<bin_serializer::Buffer>);
static_assert(bin_serializer::BufferLike<bin_serializer::FixedBuffer<128>>);
static_assert(bin_serializer::Serializable<Vec>);
static_assert(bin_serializer::Serializable<Player>);
static_assert(bin_serializer::Serializable<std::string>);

TEST_CASE("uint64_t Roundtrip Test")
{
    const auto val = 
        GENERATE(
            0ull,
            1ull,
            std::numeric_limits<uint64_t>::max(),
            0xDEADBEEFCAFEBABEull,
            42ull
        );

    roundtrip_test<bin_serializer::Buffer, uint64_t>(val);
}

TEST_CASE("float Roundtrip Test")
{
    const auto val = 
        GENERATE(
            0.0f,
            -0.0f,
            1.0f,
            -1.0f,
            std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN()
        );

    bin_serializer::Buffer buffer;

    REQUIRE(
        bin_serializer::serialize(
            buffer,
            val
        )
    );

    float op{};

    REQUIRE(
        bin_serializer::deserialize(
            buffer,
            op
        )
    );

    REQUIRE(
        std::memcmp(
            &val,
            &op,
            sizeof(float)
        ) == 0
    );
}

TEST_CASE("Vec Roundtrip")
{
    Vec ip{
        GENERATE(-1000.f, -1.f, 0.0f, 1.f, 1000.f),
        GENERATE(-1000.f, -1.f, 0.0f, 1.f, 1000.f),
        GENERATE(-1000.f, -1.f, 0.0f, 1.f, 1000.f)
    };

    roundtrip_test<bin_serializer::Buffer, Vec>(ip);
}

TEST_CASE("Transform Roundtrip Test")
{
    Transform ip {{1.f, 2.f, 3.f}, {1.f, 2.f, 3.f}, GENERATE(
        0ull,
        1ull,
        999999ull,
        std::numeric_limits<uint64_t>::max()
    )};

    roundtrip_test<bin_serializer::Buffer, Transform>(ip);
}

TEST_CASE("Player Roundtrip Test")
{
    const auto name = GENERATE(
        std::string{},
        std::string{"a"},
        std::string{"pratham"},
        std::string(128, 'x')
    );

    Player ip{
        {
            {1.f, 2.f, 3.f},
            {1.f, 2.f, 3.f},
            123456
        },
        name,
        99999
    };

    roundtrip_test<bin_serializer::Buffer, Player>(ip);
}

TEST_CASE("Fixed-buffer Exact Fit Test")
{
    Vec vec{1.f, 2.f, 3.f};

    bin_serializer::FixedBuffer<sizeof(Vec)> buffer;

    REQUIRE(
        bin_serializer::serialize(
            buffer,
            vec
        )
    );

    REQUIRE(buffer.size() == sizeof(Vec));
}

TEST_CASE("Fixed Buffer 1-Byte Overflow")
{
    Vec vec{1.f, 2.f, 3.f};

    bin_serializer::FixedBuffer<sizeof(Vec) - 1> buffer;

    REQUIRE_FALSE(
        bin_serializer::serialize(
            buffer,
            vec
        )
    );
}

TEST_CASE("Truncated Buffer Test")
{
    Vec vec{1.f, 2.f, 3.f};

    bin_serializer::Buffer full;

    REQUIRE(
        bin_serializer::serialize(
            full,
            vec
        )
    );

    bin_serializer::FixedBuffer<sizeof(Vec) - 1> truncated;

    REQUIRE(
        truncated.write(full.data(), sizeof(Vec) - 1)
    );

    Vec op{};

    REQUIRE_FALSE(
        bin_serializer::deserialize(
            truncated,
            op
        )
    );
}

TEST_CASE("FixedBuffer Reuse Correctness")
{
    bin_serializer::FixedBuffer<128> buffer;

    for(int i{}; i < 1000; ++i)
    {
        buffer.clear();

        Vec ip{
            static_cast<float>(i),
            static_cast<float>(i + 1),
            static_cast<float>(i + 2)
        };

        REQUIRE(
            bin_serializer::serialize(
                buffer,
                ip
            )
        );

        Vec op{};

        REQUIRE(
            bin_serializer::deserialize(
                buffer,
                op
            )
        );

        REQUIRE(op == ip);
    }
}

TEST_CASE("Raw Byte Equivalency")
{
    Vec vec{1.f, 2.f, 3.f};

    bin_serializer::Buffer buffer;

    REQUIRE(
        bin_serializer::serialize(
            buffer,
            vec
        )
    );

    REQUIRE(buffer.size() == sizeof(Vec));

    REQUIRE(
        std::memcmp(
            buffer.data(),
            &vec,
            sizeof(Vec)
        ) == 0
    );
}

TEST_CASE("FixedBuffer String Payload Overflow Fails.")
{
    const std::string name = "abcdef";

    /*
    Wire format
    [payload][chars]

    length prefixes the payload
    */

    constexpr size_t capacity = sizeof(uint64_t) + 4;
    bin_serializer::FixedBuffer<capacity> buffer;

    REQUIRE_FALSE(
        bin_serializer::serialize(
            buffer,
            name
        )
    );
}

TEST_CASE("Nested string overflow propagates failure")
{
    Player player{
        {
            {1.f, 2.f, 3.f},
            {4.f, 5.f, 6.f},
            123,
        },
        std::string(64, 'x'),
        999
    };

    /*
    add enough space for 
        - trivially copyable fields
        - string legnth prefix
    not for the string payload
    */

    constexpr size_t capacity{64};

    bin_serializer::FixedBuffer<capacity> buffer;
    
    REQUIRE_FALSE(
        bin_serializer::serialize(
            buffer,
            player
        )
    );
}

TEST_CASE("Failed serialization leaves partial bytes")
{
    const std::string val(32, 'x');

    constexpr size_t cap{sizeof(uint64_t)};

    bin_serializer::FixedBuffer<cap> buffer;

    REQUIRE_FALSE(
        bin_serializer::serialize(
            buffer,
            val
        )
    );

    REQUIRE(buffer.size() == sizeof(uint64_t)); //length prefix alr written
}

TEST_CASE("Failure of corrupted string length")
{
    bin_serializer::Buffer buffer;

    const uint64_t fake_size = 1'000'000;

    REQUIRE(
        buffer.write(
            &fake_size,
            sizeof(fake_size)
        )
    );

    // No payload bytes

    std::string op{};

    REQUIRE_FALSE(
        bin_serializer::deserialize(
            buffer,
            op
        )
    );
}

TEST_CASE("Correct serialization of empty string")
{
    std::string val{};

    bin_serializer::Buffer buffer;

    REQUIRE(
        bin_serializer::serialize(
            buffer,
            val
        )
    );

    REQUIRE(buffer.size() == sizeof(uint64_t));

    uint64_t length{};

    REQUIRE(
        buffer.read(
            0,
            &length,
            sizeof(length)
        )
    );

    REQUIRE(length == 0);
}

TEST_CASE("Deserializer reuse overwrites previous state")
{
    std::string first = "pnayak";
    std::string second(128, 'x');

    bin_serializer::Buffer buffer;

    REQUIRE(
        bin_serializer::serialize(
            buffer,
            second
        )
    );

    std::string op{first};

    REQUIRE(
        bin_serializer::deserialize(
            buffer,
            op
        )
    );

    REQUIRE(op == second);
}

TEST_CASE("Cross buffer byte equivalence")
{
    Player player{
        {
            {1.f, 2.f, 3.f},
            {4.f, 5.f, 6.f},
            123,
        },
        std::string(64, 'x'),
        999
    };

    bin_serializer::Buffer dyn_buff;
    bin_serializer::FixedBuffer<1024> fixed_buff;

    REQUIRE(
        bin_serializer::serialize(
            dyn_buff,
            player
        )
    );

    REQUIRE(
        bin_serializer::serialize(
            fixed_buff,
            player
        )
    );

    REQUIRE(dyn_buff.size() == fixed_buff.size());

    REQUIRE(
        std::memcmp(
            dyn_buff.data(),
            fixed_buff.data(),
            dyn_buff.size()
        ) == 0
    );
}
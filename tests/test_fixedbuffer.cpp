#include "../include/bin_serializer/fixed_buffer.hpp"
#include "../include/bin_serializer/serializer.hpp"
#include "bin_serializer/buffer.hpp"
#include "bin_serializer/deserializer.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

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
};

TEST_CASE("FixedBuffer Roundtrip")
{
    Vec vec{1.f, 2.f, 3.f};

    bin_serializer::FixedBuffer<64> buffer;

    REQUIRE(bin_serializer::serialize(
        buffer,
        vec
    ));

    Vec op{};

    REQUIRE(bin_serializer::deserialize(
        buffer,
        op));

    REQUIRE(op.x == vec.x);
    REQUIRE(op.y == vec.y);
    REQUIRE(op.z == vec.z);
}

TEST_CASE("FixedBuffer rejects overflow")
{
    Vec vec{1.f, 2.f, 3.f};

    bin_serializer::FixedBuffer<4> buffer;

    REQUIRE_FALSE(bin_serializer::serialize(
        buffer,
        vec
    ));
}
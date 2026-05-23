#include "../include/bin_serializer/buffer.hpp"
#include "../include/bin_serializer/serializer.hpp"
#include "bin_serializer/deserializer.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

struct TrivialPoint
{
    float x;
    float y;
    uint32_t id;

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor)
    {
        visitor(x, y, id);
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor) const
    {
        visitor(x, y, id);
    }
};

TEST_CASE("Trivial Point Roundtrip")
{
    TrivialPoint point{1.5f, 2.5f, 42};

    bin_serializer::Buffer buffer;
    bin_serializer::serialize(buffer, point);

    TrivialPoint restored{};

    const bool success = bin_serializer::deserialize(buffer, restored);

    REQUIRE(success);
    REQUIRE(restored.x == point.x);
    REQUIRE(restored.y == point.y);
    REQUIRE(restored.id == point.id);

}
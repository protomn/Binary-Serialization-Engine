#include "../include/bin_serializer/buffer.hpp"
#include "../include/bin_serializer/serializer.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

struct TrivialPoint
{
    float x;
    float y;
    uint32_t id;

    template<typename Visitor>
    void reflect(Visitor &&visitor)
    {
        visitor(x, y, id);
    }

    template<typename Visitor>
    void reflect(Visitor &&visitor) const
    {
        visitor(x, y, id);
    }
};

TEST_CASE("Trivially copyable fields serialize into the buffer")
{
    TrivialPoint point{1.5f, 2.5f, 42};

    bin_serializer::Buffer buffer;
    bin_serializer::serialize(buffer, point);

    REQUIRE(
        buffer.size() == sizeof(float) * 2 + sizeof(uint32_t)
    );

    const auto *bytes = buffer.data();

    REQUIRE(
        std::memcmp(bytes, &point.x, sizeof(float)) == 0
    );

    REQUIRE(
        std::memcmp(bytes + (sizeof(float) * 2), &point.id, sizeof(uint32_t)) == 0
    );
}
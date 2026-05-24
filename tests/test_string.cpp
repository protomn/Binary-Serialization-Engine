#include <../include/bin_serializer/buffer.hpp>
#include <../include/bin_serializer/serializer.hpp>
#include <../include/bin_serializer/deserializer.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <cstdint>

struct Player
{
    uint32_t id;
    std::string name;

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor)
    {
        return visitor(id, name);
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor) const
    {
        return visitor(id, name);
    }
};

TEST_CASE("String Roundtrip test")
{
    Player player{42, "pratham"};

    bin_serializer::Buffer buffer;
    bin_serializer::serialize(buffer, player);

    Player restored{};

    const bool success = bin_serializer::deserialize(
        buffer,
        restored);

    REQUIRE(success);
    REQUIRE(restored.id == player.id);
    REQUIRE(restored.name == player.name);
}
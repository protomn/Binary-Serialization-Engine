#pragma once
#include <cstdint>
#include <string>

struct Vec
{
    //small trivial struct
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

struct LargeStruct
{
    //large trivial struct
    float f0, f1, f2, f3;
    float f4, f5, f6, f7;
    float f8, f9, f10, f11;
    float f12, f13, f14, f15;
    float f16, f17, f18, f19;

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor)
    {
        return visitor(
            f0, f1, f2, f3,
            f4, f5, f6, f7,
            f8, f9, f10, f11,
            f12, f13, f14, f15,
            f16, f17, f18, f19
        );
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor) const
    {
        return visitor(
            f0, f1, f2, f3,
            f4, f5, f6, f7,
            f8, f9, f10, f11,
            f12, f13, f14, f15,
            f16, f17, f18, f19
        );
    }
};

struct Nested
{
    //nested struct
    Vec posn;
    Vec vel;
    float rotation[4];
    uint64_t timestamp;

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor)
    {
        return visitor(posn, vel, rotation, timestamp);
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor) const
    {
        return visitor(posn, vel, rotation, timestamp);
    }
};

struct Mixed
{
    //struct with mixed semantics
    Nested nested;
    std::string name;
    uint64_t score;

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor)
    {
        return visitor(nested, name, score);
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor &&visitor) const
    {
        return visitor(nested, name, score);
    }
};

struct StringPayload
{
    std::string payload;

    template<typename Visitor>
    decltype(auto) reflect(Visitor&& visitor)
    {
        return visitor(payload);
    }

    template<typename Visitor>
    decltype(auto) reflect(Visitor&& visitor) const
    {
        return visitor(payload);
    }
};
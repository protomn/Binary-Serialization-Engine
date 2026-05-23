#pragma once

#include <concepts>

/*
defined what field can be considered as reflectable at compile time
type is reflectable if it exposes a callable reflect(visitor) interface
*/

namespace bin_serializer
{
    template<typename T>
    concept Reflectable = requires(T &mutable_val, const T &const_val) // serialization should not mutate objects
    {
        mutable_val.reflect([](auto &...) { });

        const_val.reflect([](const auto &...) { });
    };
}
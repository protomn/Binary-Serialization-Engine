#pragma once

#include <bit>
#include <type_traits>
#include <concepts>
#include <string>
#include "reflect.hpp"

namespace bin_serializer
{
    template<typename T>
    concept Serializable = 
        std::is_trivially_copyable_v<T> || Reflectable<T> || std::is_same_v<T, std::string>;
} //bin_serializer
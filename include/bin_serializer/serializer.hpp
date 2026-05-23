#pragma once

#include "buffer.hpp"
#include "reflect.hpp"
#include <type_traits>
#include <stdexcept>

namespace bin_serializer
{
    template<typename T>
    void serialize_field(Buffer &buffer, const T& field)
    {
        if constexpr(std::is_trivially_copyable_v<T>)
        {
            buffer.write(&field, sizeof(T));
        }
        else if constexpr(Reflectable<T>)
        {
            field.reflect(
                [&](const auto &... fields)
                {
                    (
                        serialize_field(
                            buffer,
                            fields
                        ), ...
                    );
                }
            );
        }
        
        else
        {
            static_assert(
                std::is_same_v<T, void>,
                "unsupported type in serializer"
            );
        }
    }


    template<Reflectable T>
    void serialize(Buffer &buffer, const T &obj)
    {
        serialize_field(
            buffer,
            obj
        );
    }
}
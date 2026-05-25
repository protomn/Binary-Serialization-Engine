#pragma once

#include "../include/bin_serializer/buffer.hpp"
#include "../include/bin_serializer/reflect.hpp"
#include "../include/bin_serializer/buffer_concept.hpp"

#include <cstring>
#include <string>
#include <type_traits>

namespace naive_serializer
{
    template<bin_serializer::BufferLike B, typename T>
    bool serialize_one(
        B &buffer,
        const T &val
    )
    {
        if constexpr(std::is_same_v<T, std::string>)
        {
            const uint64_t size = val.size();

            if(!buffer.write(&size, sizeof(size)))
            {
                return false;
            }

            return buffer.write(val.data(), val.size());
        }

        else if constexpr(bin_serializer::Reflectable<T>)
        {
            bool success{true};
            val.reflect(
                [&](const auto &... fields)
                {
                    success = (
                        serialize_one(
                            buffer,
                            fields
                        ) && ...
                    );
                }
            );

            return success;
        }

        else if constexpr(std::is_trivially_copyable_v<T>)
        {
            return buffer.write(&val, sizeof(T));
        }

        else
        {
            static_assert(
                std::is_same_v<T, void>,
                "unsupported type"
            );

            return false;
        }
    }

    template<bin_serializer::BufferLike B, typename T>
    bool serialize(
        B &buffer,
        const T &val
    )
    {
        return serialize_one(
            buffer,
            val
        );
    }
} 
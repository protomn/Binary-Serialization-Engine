#pragma once

#include "buffer_concept.hpp"
#include "serializable.hpp"
#include <type_traits>
#include <stdexcept>
#include <string>

namespace bin_serializer
{
    template<BufferLike B, typename T>
    bool serialize_field(B &buffer, const T& field)
    {
        if constexpr(std::is_trivially_copyable_v<T>)
        {
            return buffer.write(&field, sizeof(T));
        }

        else if constexpr(std::is_same_v<T, std::string>)
        {
            //support for std::string
            const uint64_t size = field.size();

            bool success = serialize_field(buffer, size);

            if(success)
            {
                return buffer.write(field.data(), size);
            }
            
            return success;
        }

        else if constexpr(Reflectable<T>)
        {
            return field.reflect(
                [&](const auto &... fields)
                {
                    return (... && serialize_field(
                        buffer,
                        fields
                    ));
                }
            );
        }
        
        else
        {
            static_assert(
                std::is_same_v<T, void>,
                "unsupported type in serializer"
            );

            return false;
        }
    }


    template<BufferLike B, Serializable T>
    bool serialize(B &buffer, const T &obj)
    {
        return serialize_field(
            buffer,
            obj
        );
    }
}
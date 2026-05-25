#pragma once

#include "buffer_concept.hpp"
#include "serializable.hpp"
#include <type_traits>
#include <string>
#include <cstddef>

namespace bin_serializer
{   
    template<BufferLike B, typename T>
    bool deserialize_field(const B &buffer, size_t &offset, T &field)
    {
        if constexpr(std::is_trivially_copyable_v<T>)
        {
            const bool success = buffer.read(
                offset,
                &field,
                sizeof(T)
            );

            if(success)
            {
                offset += sizeof(T);
            }

            return success;
        }

        else if constexpr(std::is_same_v<T, std::string>)
        {
            //std::string support for deserialization
            uint64_t size{0};

            if(!deserialize_field(
                buffer,
                offset,
                size))
            {
                return false;
            }

            //allocate string storage
            field.resize(size);

            const bool success = buffer.read(
                                        offset,
                                        field.data(),
                                        size);
            if(success)
            {
                offset += size;
            }

            return success;

        }

        else if constexpr(Reflectable<T>)
        {
            return field.reflect(
                [&](auto &... fields)
                {
                    
                        /**
                        logical short circuit eval
                        fold over && so eval stops at first failure
                        and because && short-circuits
                        && guarantees left-to-right eval so we don't
                        need to worry about offset advancement
                        */
                        return (... && deserialize_field(
                            buffer,
                            offset,
                            fields
                        )
                    );
                
                }
            );
        }
        else
        {
            static_assert(
                std::is_same_v<T, void>,
                "unsupported type in deserializer"
            );

            return false;
        }
    }

    template<BufferLike B, Serializable T>
    bool deserialize(const B &buffer, T &obj)
    {
        size_t offset{0};

        return deserialize_field(
                            buffer,
                            offset,
                            obj
                            );
    }
} //namespace bin_serializer
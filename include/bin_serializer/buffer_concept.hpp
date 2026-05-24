#pragma once

#include <concepts>
#include <cstdint>
#include <cstddef>

namespace bin_serializer
{
    template<typename B>
    concept BufferLike = requires(
        B buffer,
        const B const_buffer,
        const void *src,
        void *dest,
        size_t offset,
        size_t size
    )
    {
        { buffer.write(src, size) } ->  std::same_as<bool>; 
        { const_buffer.read(offset, dest, size) } -> std::same_as<bool>;
        { const_buffer.data() } -> std::same_as<const uint8_t*>;
        { const_buffer.size() } -> std::same_as<size_t>;
        { buffer.clear() } -> std::same_as<void>;
    };
} //namespace bin_serializer
#include "../include/bin_serializer/buffer.hpp"
#include <cstring>
#include <cstdint>

namespace bin_serializer
{
    bool Buffer::write(const void *data, size_t size)
    {
        const auto *bytes = static_cast<const std::uint8_t*>(data);

        data_.insert(data_.end(), bytes, bytes + size);

        return true;
    }

    bool Buffer::read(size_t offset, void *dest, size_t size) const
    {
        if(offset + size > data_.size())
        {
            return false;
        }

        std::memcpy(dest, data_.data() + offset, size);
        return true;
    }

    size_t Buffer::size() const
    {
        return data_.size();
    }

    const uint8_t *Buffer::data() const
    {
        return data_.data();
    }

    void Buffer::clear()
    {
        data_.clear();
    }

    void Buffer::reserve(size_t size)
    {
        data_.reserve(size);
    }

} //namespace bin_serializer
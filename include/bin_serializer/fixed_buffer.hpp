#pragma once

#include "buffer_concept.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace bin_serializer
{
    template<size_t Capacity>
    class FixedBuffer
    {
        private:

            std::array<uint8_t, Capacity> data_;
            size_t size_{0};

        public:

            [[nodiscard]] bool write(const void *data, size_t size)
            {
                if(size_ + size > Capacity)
                {
                    return false;
                }
                
                std::memcpy(data_.data() + size_, data, size);
                size_ += size;
                return true;
            }

            bool read(size_t offset, void *dest, size_t size) const
            {
                if(offset + size > size_)
                {
                    return false;
                }

                std::memcpy(dest, data_.data() + offset, size);
                return true;
            }

            void clear()
            {
                size_ = 0;
            }

            [[nodiscard]] static constexpr size_t capacity()
            {
                return Capacity;
            }

            [[nodiscard]] const uint8_t *data() const
            {
                return data_.data();
            }

            [[nodiscard]] size_t size() const
            {
                return size_;
            }
    };
} //namespace bin_serializer
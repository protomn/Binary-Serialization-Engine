#pragma once

/*
base idea:
    - append bytes
    - grow storage
    - expose size and data
*/

#include <cstdint>
#include <cstddef>
#include <vector> // ?? move to custom implementation later

namespace bin_serializer
{
    class Buffer
    {
        private:
            std::vector<uint8_t> data_;

        public:

            [[nodiscard]] bool write(const void *data, size_t size);
            [[nodiscard]] bool read(size_t offset, void *dest, size_t size) const;

            [[nodiscard]] const uint8_t *data() const;
            [[nodiscard]] size_t size() const;

            void clear();
            void reserve(size_t size);
    };
} //namespace bin_serializer
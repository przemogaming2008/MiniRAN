#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace miniran {

class ByteBufferWriter {
public:
    void writeU8(std::uint8_t value);
    void writeU32(std::uint32_t value);
    void writeU64(std::uint64_t value);
    void writeBytes(const std::vector<std::uint8_t>& bytes);

    const std::vector<std::uint8_t>& bytes() const;

private:
    std::vector<std::uint8_t> bytes_;
};

class ByteBufferReader {
public:
    explicit ByteBufferReader(const std::vector<std::uint8_t>& bytes);

    bool readU8(std::uint8_t& value);
    bool readU32(std::uint32_t& value);
    bool readU64(std::uint64_t& value);
    bool readBytes(std::size_t length, std::vector<std::uint8_t>& out);

    bool hasRemaining() const;
    std::size_t remaining() const;

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t offset_ = 0;
};

}  // namespace miniran

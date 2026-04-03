#include "miniran/common/byte_buffer.h"

namespace miniran {

void ByteBufferWriter::writeU8(std::uint8_t value) {
    bytes_.push_back(value);
}

void ByteBufferWriter::writeU32(std::uint32_t value) {
    bytes_.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
    bytes_.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    bytes_.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

void ByteBufferWriter::writeU64(std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        bytes_.push_back(static_cast<std::uint8_t>((value >> static_cast<std::uint64_t>(shift)) & 0xFFU));
    }
}

void ByteBufferWriter::writeBytes(const std::vector<std::uint8_t>& bytes) {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
}

const std::vector<std::uint8_t>& ByteBufferWriter::bytes() const {
    return bytes_;
}

ByteBufferReader::ByteBufferReader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

bool ByteBufferReader::readU8(std::uint8_t& value) {
    if (remaining() < 1) {
        return false;
    }
    value = bytes_[offset_++];
    return true;
}

bool ByteBufferReader::readU32(std::uint32_t& value) {
    if (remaining() < 4) {
        return false;
    }
    value = (static_cast<std::uint32_t>(bytes_[offset_]) << 24U) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 1]) << 16U) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 2]) << 8U) |
            static_cast<std::uint32_t>(bytes_[offset_ + 3]);
    offset_ += 4;
    return true;
}

bool ByteBufferReader::readU64(std::uint64_t& value) {
    if (remaining() < 8) {
        return false;
    }
    value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8U) | static_cast<std::uint64_t>(bytes_[offset_ + static_cast<std::size_t>(i)]);
    }
    offset_ += 8;
    return true;
}

bool ByteBufferReader::readBytes(std::size_t length, std::vector<std::uint8_t>& out) {
    if (remaining() < length) {
        return false;
    }
    out.assign(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
               bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + length));
    offset_ += length;
    return true;
}

bool ByteBufferReader::hasRemaining() const {
    return offset_ < bytes_.size();
}

std::size_t ByteBufferReader::remaining() const {
    return bytes_.size() - offset_;
}

}  // namespace miniran

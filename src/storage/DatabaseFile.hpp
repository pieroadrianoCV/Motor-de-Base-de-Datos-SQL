#pragma once

#include "Tuple.hpp"
#include "TupleSerializer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace storage {

struct DatabaseImage {
  std::size_t degree{0};
  std::vector<Tuple> tuples;
};

class DatabaseFile {
public:
  static constexpr std::uint32_t FORMAT_VERSION = 1;

  static void save(const std::filesystem::path& path,
                   std::size_t degree,
                   const std::vector<Tuple>& tuples)
  {
    if (path.empty()) {
      throw std::invalid_argument("DatabaseFile: empty path");
    }
    if (degree < 2) {
      throw std::invalid_argument("DatabaseFile: B-Tree degree must be >= 2");
    }

    Buffer bytes;
    bytes.insert(bytes.end(), MAGIC.begin(), MAGIC.end());
    appendUint32(bytes, FORMAT_VERSION);
    appendUint64(bytes, static_cast<std::uint64_t>(degree));
    appendUint64(bytes, static_cast<std::uint64_t>(tuples.size()));
    for (const auto& tuple : tuples) {
      const auto serialized = TupleSerializer::serialize(tuple);
      appendUint64(bytes, static_cast<std::uint64_t>(serialized.size()));
      bytes.insert(bytes.end(), serialized.begin(), serialized.end());
    }
    appendUint64(bytes, checksum(bytes));

    const auto parent = path.parent_path();
    if (!parent.empty()) {
      std::filesystem::create_directories(parent);
    }
    auto temporary = path;
    temporary += ".tmp";

    try {
      std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
      if (!output) {
        throw std::runtime_error("DatabaseFile: cannot open temporary file");
      }
      output.write(reinterpret_cast<const char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
      output.flush();
      if (!output) {
        throw std::runtime_error("DatabaseFile: failed while writing data");
      }
      output.close();
      std::filesystem::rename(temporary, path);
    } catch (...) {
      std::error_code ignored;
      std::filesystem::remove(temporary, ignored);
      throw;
    }
  }

  static DatabaseImage load(const std::filesystem::path& path)
  {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
      throw std::runtime_error("DatabaseFile: cannot open " + path.string());
    }
    Buffer bytes((std::istreambuf_iterator<char>(input)),
                 std::istreambuf_iterator<char>());
    if (!input.eof() && input.fail()) {
      throw std::runtime_error("DatabaseFile: failed while reading data");
    }
    if (bytes.size() < HEADER_SIZE + sizeof(std::uint64_t)) {
      throw std::runtime_error("DatabaseFile: truncated header");
    }

    const auto storedChecksum = readUint64At(bytes, bytes.size() - 8);
    const Buffer content(bytes.begin(), bytes.end() - 8);
    if (checksum(content) != storedChecksum) {
      throw std::runtime_error("DatabaseFile: checksum mismatch");
    }

    std::size_t offset = 0;
    for (const auto expected : MAGIC) {
      if (readByte(content, offset) != expected) {
        throw std::runtime_error("DatabaseFile: invalid file signature");
      }
    }
    const auto version = readUint32(content, offset);
    if (version != FORMAT_VERSION) {
      throw std::runtime_error("DatabaseFile: unsupported format version");
    }
    const auto degree = readUint64(content, offset);
    const auto count = readUint64(content, offset);
    if (degree < 2 || count > MAX_RECORDS) {
      throw std::runtime_error("DatabaseFile: invalid metadata");
    }

    DatabaseImage image;
    image.degree = static_cast<std::size_t>(degree);
    image.tuples.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t record = 0; record < count; ++record) {
      const auto size = readUint64(content, offset);
      if (size > MAX_TUPLE_SIZE || size > content.size() - offset) {
        throw std::runtime_error("DatabaseFile: invalid tuple size");
      }
      TupleSerializer::Buffer tupleBytes(
        content.begin() + static_cast<std::ptrdiff_t>(offset),
        content.begin() + static_cast<std::ptrdiff_t>(offset + size));
      image.tuples.push_back(TupleSerializer::deserialize(tupleBytes));
      offset += static_cast<std::size_t>(size);
    }
    if (offset != content.size()) {
      throw std::runtime_error("DatabaseFile: trailing data");
    }
    return image;
  }

private:
  using Buffer = std::vector<std::uint8_t>;
  static constexpr std::array<std::uint8_t, 8> MAGIC{
    'E', 'D', 'A', 'D', 'B', '0', '0', '1'};
  static constexpr std::size_t HEADER_SIZE = 8 + 4 + 8 + 8;
  static constexpr std::uint64_t MAX_RECORDS = 10'000'000;
  static constexpr std::uint64_t MAX_TUPLE_SIZE = 64ULL * 1024ULL * 1024ULL;

  static void appendUint32(Buffer& bytes, std::uint32_t value)
  {
    for (int shift = 0; shift < 32; shift += 8) {
      bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
  }

  static void appendUint64(Buffer& bytes, std::uint64_t value)
  {
    for (int shift = 0; shift < 64; shift += 8) {
      bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
  }

  static std::uint8_t readByte(const Buffer& bytes, std::size_t& offset)
  {
    if (offset >= bytes.size()) {
      throw std::runtime_error("DatabaseFile: truncated data");
    }
    return bytes[offset++];
  }

  static std::uint32_t readUint32(const Buffer& bytes, std::size_t& offset)
  {
    std::uint32_t value = 0;
    for (int shift = 0; shift < 32; shift += 8) {
      value |= static_cast<std::uint32_t>(readByte(bytes, offset)) << shift;
    }
    return value;
  }

  static std::uint64_t readUint64(const Buffer& bytes, std::size_t& offset)
  {
    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
      value |= static_cast<std::uint64_t>(readByte(bytes, offset)) << shift;
    }
    return value;
  }

  static std::uint64_t readUint64At(const Buffer& bytes, std::size_t offset)
  {
    return readUint64(bytes, offset);
  }

  static std::uint64_t checksum(const Buffer& bytes)
  {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto byte : bytes) {
      hash ^= byte;
      hash *= 1099511628211ULL;
    }
    return hash;
  }
};

}  // namespace storage

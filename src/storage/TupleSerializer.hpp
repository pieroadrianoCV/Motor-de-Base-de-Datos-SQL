#pragma once

#include "Tuple.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace storage {

class TupleSerializer {
public:
  using Buffer = std::vector<std::uint8_t>;

  static Buffer serialize(const Tuple& tuple)
  {
    Buffer buffer;

    writeUint32(buffer, static_cast<std::uint32_t>(tuple.size()));

    for (const auto& value : tuple) {
      serializeValue(buffer, value);
    }

    return buffer;
  }

  static Tuple deserialize(const Buffer& buffer)
  {
    std::size_t offset = 0;

    const auto fieldCount = readUint32(buffer, offset);

    Tuple tuple;
    tuple.reserve(fieldCount);

    for (std::uint32_t i = 0; i < fieldCount; ++i) {
      tuple.push_back(deserializeValue(buffer, offset));
    }

    if (offset != buffer.size()) {
      throw std::runtime_error(
        "TupleSerializer: trailing data after tuple");
    }

    return tuple;
  }

private:
  enum class ValueType : std::uint8_t {
    Int64 = 0,
    Double = 1,
    String = 2
  };

  static void writeUint32(
    Buffer& buffer,
    std::uint32_t value)
  {
    for (int i = 0; i < 4; ++i) {
      buffer.push_back(
        static_cast<std::uint8_t>((value >> (i * 8)) & 0xFF));
    }
  }

  static void writeUint64(
    Buffer& buffer,
    std::uint64_t value)
  {
    for (int i = 0; i < 8; ++i) {
      buffer.push_back(
        static_cast<std::uint8_t>((value >> (i * 8)) & 0xFF));
    }
  }

  static std::uint32_t readUint32(
    const Buffer& buffer,
    std::size_t& offset)
  {
    ensureAvailable(buffer, offset, 4);

    std::uint32_t value = 0;

    for (int i = 0; i < 4; ++i) {
      value |=
        static_cast<std::uint32_t>(buffer[offset++])
        << (i * 8);
    }

    return value;
  }

  static std::uint64_t readUint64(
    const Buffer& buffer,
    std::size_t& offset)
  {
    ensureAvailable(buffer, offset, 8);

    std::uint64_t value = 0;

    for (int i = 0; i < 8; ++i) {
      value |=
        static_cast<std::uint64_t>(buffer[offset++])
        << (i * 8);
    }

    return value;
  }

  static void serializeValue(
    Buffer& buffer,
    const Value& value)
  {
    std::visit(
      [&buffer](const auto& current) {
        using T = std::decay_t<decltype(current)>;

        if constexpr (std::is_same_v<T, std::int64_t>) {
          buffer.push_back(
            static_cast<std::uint8_t>(ValueType::Int64));

          writeUint64(
            buffer,
            static_cast<std::uint64_t>(current));
        }
        else if constexpr (std::is_same_v<T, double>) {
          buffer.push_back(
            static_cast<std::uint8_t>(ValueType::Double));

          std::uint64_t bits;
          std::memcpy(&bits, &current, sizeof(bits));

          writeUint64(buffer, bits);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
          buffer.push_back(
            static_cast<std::uint8_t>(ValueType::String));

          writeUint32(
            buffer,
            static_cast<std::uint32_t>(current.size()));

          buffer.insert(
            buffer.end(),
            current.begin(),
            current.end());
        }
      },
      value);
  }

  static Value deserializeValue(
    const Buffer& buffer,
    std::size_t& offset)
  {
    ensureAvailable(buffer, offset, 1);

    const auto type =
      static_cast<ValueType>(buffer[offset++]);

    switch (type) {
    case ValueType::Int64: {
      const auto bits = readUint64(buffer, offset);
      return static_cast<std::int64_t>(bits);
    }

    case ValueType::Double: {
      const auto bits = readUint64(buffer, offset);

      double value;
      std::memcpy(&value, &bits, sizeof(value));

      return value;
    }

    case ValueType::String: {
      const auto size = readUint32(buffer, offset);

      ensureAvailable(buffer, offset, size);

      std::string value(
        reinterpret_cast<const char*>(&buffer[offset]),
        size);

      offset += size;

      return value;
    }

    default:
      throw std::runtime_error(
        "TupleSerializer: unknown value type");
    }
  }

  static void ensureAvailable(
    const Buffer& buffer,
    std::size_t offset,
    std::size_t amount)
  {
    if (offset > buffer.size() ||
      amount > buffer.size() - offset) {
      throw std::runtime_error(
        "TupleSerializer: invalid or truncated buffer");
    }
  }
};

}


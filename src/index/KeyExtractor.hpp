#ifndef KEYEXTRACTOR_H
#define KEYEXTRACTOR_H

#include "BTreeNode.hpp"
#include "../storage/Tuple.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <variant>

namespace db::index {

  inline Key keyFromTuple(const storage::Tuple& tuple, std::size_t column) {
    if (column >= tuple.size()) {
      throw std::out_of_range("KeyExtractor: la columna indexada no existe");
    }
    const auto* value = std::get_if<std::int64_t>(&tuple[column]);
    if (value == nullptr) {
      throw std::invalid_argument(
          "KeyExtractor: la columna indexada debe ser de tipo Int64");
    }
    return *value;
  }

}

#endif //KEYEXTRACTOR_H

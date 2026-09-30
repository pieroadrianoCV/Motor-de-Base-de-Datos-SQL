#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace storage {

using Value = std::variant<
    std::int64_t,
    double,
    std::string
>;

}


#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

namespace fixtures {

struct Record {
    std::int64_t key;
    std::int64_t rowId;
};

inline std::vector<Record> sequentialRecords(std::size_t count) {
    std::vector<Record> records;
    records.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        records.push_back({static_cast<std::int64_t>(index),
                           static_cast<std::int64_t>(index)});
    }
    return records;
}

inline std::vector<Record> shuffledRecords(std::size_t count,
                                           std::uint32_t seed = 42) {
    auto records = sequentialRecords(count);
    std::mt19937 generator(seed);
    std::shuffle(records.begin(), records.end(), generator);
    return records;
}

inline std::vector<std::int64_t> queryKeys(std::size_t recordCount,
                                           std::size_t queryCount,
                                           std::uint32_t seed = 73) {
    if (recordCount == 0 && queryCount != 0) {
        throw std::invalid_argument("no se pueden generar consultas sin registros");
    }

    std::vector<std::int64_t> keys;
    keys.reserve(queryCount);
    std::mt19937 generator(seed);
    if (recordCount != 0) {
        std::uniform_int_distribution<std::size_t> distribution(0, recordCount - 1);
        for (std::size_t index = 0; index < queryCount; ++index) {
            keys.push_back(static_cast<std::int64_t>(distribution(generator)));
        }
    }
    return keys;
}

}  // namespace fixtures


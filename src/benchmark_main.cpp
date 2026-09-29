#include "benchmark/BulkLoader.hpp"
#include "benchmark/ScanBenchmark.hpp"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace {

struct Record {
    std::int64_t key;
    std::int64_t rowId;
};

// Adaptador temporal con la misma complejidad de búsqueda que el B-Tree.
// En integración se reemplaza el callback lookup por BTree::search.
class OrderedIndex {
public:
    bool insert(const Record& record) {
        entries_.push_back(record);
        sorted_ = false;
        return true;
    }

    bool validate() {
        if (!sorted_) {
            std::sort(entries_.begin(), entries_.end(),
                      [](const auto& left, const auto& right) {
                          return left.key < right.key;
                      });
            sorted_ = true;
        }
        return std::adjacent_find(entries_.begin(), entries_.end(),
                                  [](const auto& left, const auto& right) {
                                      return left.key == right.key;
                                  }) == entries_.end();
    }

    db::benchmark::LookupResult<std::int64_t> lookup(std::int64_t key) const {
        std::size_t first = 0;
        std::size_t last = entries_.size();
        std::size_t accesses = 0;
        while (first < last) {
            ++accesses;
            const auto middle = first + (last - first) / 2;
            if (entries_[middle].key == key) {
                return {true, entries_[middle].rowId, accesses};
            }
            if (entries_[middle].key < key) {
                first = middle + 1;
            } else {
                last = middle;
            }
        }
        return {false, 0, accesses};
    }

private:
    std::vector<Record> entries_;
    bool sorted_{true};
};

std::size_t parseCount(int argc, char** argv, int position,
                       std::size_t defaultValue) {
    if (argc <= position) {
        return defaultValue;
    }
    return static_cast<std::size_t>(std::stoull(argv[position]));
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const auto recordCount = parseCount(argc, argv, 1, 100'000);
        const auto queryCount = parseCount(argc, argv, 2, 5'000);
        if (recordCount == 0) {
            throw std::invalid_argument("la cantidad de registros debe ser positiva");
        }

        std::vector<Record> records(recordCount);
        for (std::size_t index = 0; index < recordCount; ++index) {
            records[index] = {static_cast<std::int64_t>(index),
                              static_cast<std::int64_t>(index)};
        }
        std::mt19937 generator(42);
        std::shuffle(records.begin(), records.end(), generator);

        OrderedIndex index;
        const auto load = db::benchmark::bulkLoad(
            records, [&](const auto& record) { return index.insert(record); },
            [&] { return index.validate(); });

        std::vector<std::int64_t> keys(queryCount);
        std::uniform_int_distribution<std::size_t> keyDistribution(0,
                                                                   recordCount - 1);
        for (auto& key : keys) {
            key = static_cast<std::int64_t>(keyDistribution(generator));
        }

        const auto comparison = db::benchmark::compareScans(
            records, keys, [&](auto key) { return index.lookup(key); },
            [](const auto& record) { return record.key; },
            [](const auto& record) { return record.rowId; });

        std::cout << "Carga masiva: " << load.inserted << " registros en "
                  << std::fixed << std::setprecision(3)
                  << db::benchmark::milliseconds(load.elapsed) << " ms\n\n";
        std::cout << std::left << std::setw(20) << "Metodo" << std::right
                  << std::setw(14) << "Tiempo (ms)" << std::setw(18)
                  << "Accesos" << '\n';
        std::cout << std::left << std::setw(20) << "Index Scan" << std::right
                  << std::setw(14)
                  << db::benchmark::milliseconds(comparison.indexScan.elapsed)
                  << std::setw(18) << comparison.indexScan.pageAccesses << '\n';
        std::cout << std::left << std::setw(20) << "Full Table Scan" << std::right
                  << std::setw(14)
                  << db::benchmark::milliseconds(comparison.fullTableScan.elapsed)
                  << std::setw(18) << comparison.fullTableScan.pageAccesses << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}


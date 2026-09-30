#include "benchmark/IndexedBulkLoader.hpp"
#include "benchmark/ScanBenchmark.hpp"
#include "index/BTree.hpp"
#include "index/KeyExtractor.hpp"
#include "storage/RecordManager.hpp"
#include "storage/Tuple.hpp"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::size_t parseCount(int argc, char** argv, int position,
                       std::size_t defaultValue, const char* name) {
    if (argc <= position) {
        return defaultValue;
    }
    const auto value = static_cast<std::size_t>(std::stoull(argv[position]));
    if (value == 0) {
        throw std::invalid_argument(std::string{name} + " debe ser positivo");
    }
    return value;
}

std::vector<storage::Tuple> createWorkload(std::size_t recordCount) {
    std::vector<storage::Tuple> tuples;
    tuples.reserve(recordCount);
    for (std::size_t position = 0; position < recordCount; ++position) {
        tuples.push_back(storage::Tuple{
            static_cast<std::int64_t>(position),
            std::string{"record-"} + std::to_string(position)});
    }
    std::mt19937 generator(42);
    std::shuffle(tuples.begin(), tuples.end(), generator);
    return tuples;
}

void printMetrics(const char* method, const db::benchmark::ScanMetrics& metrics) {
    std::cout << std::left << std::setw(20) << method << std::right
              << std::setw(14) << db::benchmark::milliseconds(metrics.elapsed)
              << std::setw(18) << metrics.pageAccesses << std::setw(14)
              << metrics.found << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const auto recordCount = parseCount(argc, argv, 1, 100'000, "registros");
        const auto queryCount = parseCount(argc, argv, 2, 5'000, "consultas");
        const auto degree = parseCount(argc, argv, 3, 64, "grado minimo");
        if (degree < 2) {
            throw std::invalid_argument("grado minimo debe ser al menos 2");
        }

        const auto tuples = createWorkload(recordCount);
        storage::RecordManager records;
        db::index::BTree index(degree);
        const auto load = db::benchmark::loadIndexedTuples(
            tuples, records, index, 0);
        if (load.inserted != recordCount || records.size() != recordCount ||
            index.size() != recordCount || !index.validate()) {
            throw std::logic_error("la carga masiva no produjo un indice valido");
        }

        std::mt19937 generator(73);
        std::uniform_int_distribution<std::size_t> distribution(0,
                                                                 recordCount - 1);
        std::vector<std::int64_t> keys;
        keys.reserve(queryCount);
        for (std::size_t query = 0; query < queryCount; ++query) {
            keys.push_back(static_cast<std::int64_t>(distribution(generator)));
        }

        const auto storedRecords = records.scan();
        const auto comparison = db::benchmark::compareScans(
            storedRecords, keys, [&](auto key) { return index.search(key); },
            [](const storage::Record& record) {
                return db::index::keyFromTuple(record.tuple, 0);
            },
            [](const storage::Record& record) { return record.id; });

        std::cout << "B-Tree real: t=" << index.degree()
                  << ", altura=" << index.height() << '\n';
        std::cout << "Carga real: " << load.inserted << " tuplas y RowIDs en "
                  << std::fixed << std::setprecision(3)
                  << db::benchmark::milliseconds(load.elapsed) << " ms\n\n";
        std::cout << std::left << std::setw(20) << "Metodo" << std::right
                  << std::setw(14) << "Tiempo (ms)" << std::setw(18)
                  << "Accesos" << std::setw(14) << "Encontrados" << '\n';
        printMetrics("Index Scan", comparison.indexScan);
        printMetrics("Full Table Scan", comparison.fullTableScan);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

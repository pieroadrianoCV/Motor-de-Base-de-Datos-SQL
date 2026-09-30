#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace db::benchmark {

template <typename RowId>
struct LookupResult {
    bool found{false};
    RowId rowId{};
    std::size_t pageAccesses{0};
};

struct ScanMetrics {
    std::chrono::nanoseconds elapsed{0};
    std::size_t queries{0};
    std::size_t found{0};
    std::size_t pageAccesses{0};
    std::uint64_t checksum{0};
};

struct ComparisonResult {
    ScanMetrics indexScan;
    ScanMetrics fullTableScan;
};

template <typename Records, typename Keys, typename IndexLookup, typename KeyOf,
          typename RowIdOf, typename PageOf>
ComparisonResult comparePagedScans(const Records& records, const Keys& keys,
                                   IndexLookup indexLookup, KeyOf keyOf,
                                   RowIdOf rowIdOf, PageOf pageOf) {
    ComparisonResult result;
    result.indexScan.queries = keys.size();
    result.fullTableScan.queries = keys.size();

    auto started = std::chrono::steady_clock::now();
    for (const auto& key : keys) {
        const auto lookup = indexLookup(key);
        result.indexScan.pageAccesses += lookup.pageAccesses;
        if (lookup.found) {
            ++result.indexScan.found;
            result.indexScan.checksum += static_cast<std::uint64_t>(lookup.rowId);
        }
    }
    result.indexScan.elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - started);

    started = std::chrono::steady_clock::now();
    for (const auto& key : keys) {
        std::uint64_t previousPage = 0;
        bool hasPreviousPage = false;
        for (const auto& record : records) {
            const auto currentPage = static_cast<std::uint64_t>(pageOf(record));
            if (!hasPreviousPage || currentPage != previousPage) {
                ++result.fullTableScan.pageAccesses;
                previousPage = currentPage;
                hasPreviousPage = true;
            }
            if (keyOf(record) == key) {
                ++result.fullTableScan.found;
                result.fullTableScan.checksum +=
                    static_cast<std::uint64_t>(rowIdOf(record));
                break;
            }
        }
    }
    result.fullTableScan.elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - started);

    if (result.indexScan.found != result.fullTableScan.found ||
        result.indexScan.checksum != result.fullTableScan.checksum) {
        throw std::logic_error(
            "Index Scan y Full Table Scan produjeron resultados distintos");
    }
    return result;
}

// Los callbacks desacoplan el benchmark de las clases BTree y Record concretas:
// indexLookup(key) devuelve RowID y accesos; keyOf/rowIdOf leen cada registro.
template <typename Records, typename Keys, typename IndexLookup, typename KeyOf,
          typename RowIdOf>
ComparisonResult compareScans(const Records& records, const Keys& keys,
                              IndexLookup indexLookup, KeyOf keyOf,
                              RowIdOf rowIdOf) {
    ComparisonResult result;
    result.indexScan.queries = keys.size();
    result.fullTableScan.queries = keys.size();

    auto started = std::chrono::steady_clock::now();
    for (const auto& key : keys) {
        const auto lookup = indexLookup(key);
        result.indexScan.pageAccesses += lookup.pageAccesses;
        if (lookup.found) {
            ++result.indexScan.found;
            result.indexScan.checksum += static_cast<std::uint64_t>(lookup.rowId);
        }
    }
    result.indexScan.elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - started);

    started = std::chrono::steady_clock::now();
    for (const auto& key : keys) {
        for (const auto& record : records) {
            ++result.fullTableScan.pageAccesses;
            if (keyOf(record) == key) {
                ++result.fullTableScan.found;
                result.fullTableScan.checksum +=
                    static_cast<std::uint64_t>(rowIdOf(record));
                break;
            }
        }
    }
    result.fullTableScan.elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - started);

    if (result.indexScan.found != result.fullTableScan.found ||
        result.indexScan.checksum != result.fullTableScan.checksum) {
        throw std::logic_error(
            "Index Scan y Full Table Scan produjeron resultados distintos");
    }
    return result;
}

inline double milliseconds(std::chrono::nanoseconds duration) {
    return std::chrono::duration<double, std::milli>(duration).count();
}

}  // namespace db::benchmark

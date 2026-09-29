#include "fixtures.hpp"
#include "test_framework.hpp"

#include "benchmark/ScanBenchmark.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace {

db::benchmark::LookupResult<std::int64_t> binaryLookup(
    const std::vector<fixtures::Record>& sorted, std::int64_t key) {
    std::size_t first = 0;
    std::size_t last = sorted.size();
    std::size_t accesses = 0;
    while (first < last) {
        ++accesses;
        const auto middle = first + (last - first) / 2;
        if (sorted[middle].key == key) {
            return {true, sorted[middle].rowId, accesses};
        }
        if (sorted[middle].key < key) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    return {false, 0, accesses};
}

}  // namespace

TEST_CASE(index_scan_and_full_scan_return_identical_results) {
    auto sorted = fixtures::sequentialRecords(1'000);
    const auto physical = fixtures::shuffledRecords(1'000);
    const std::vector<std::int64_t> keys{0, 50, 500, 999, 1'500};

    const auto result = db::benchmark::compareScans(
        physical, keys, [&](auto key) { return binaryLookup(sorted, key); },
        [](const auto& record) { return record.key; },
        [](const auto& record) { return record.rowId; });

    EXPECT_EQ(result.indexScan.found, 4U);
    EXPECT_EQ(result.fullTableScan.found, 4U);
    EXPECT_EQ(result.indexScan.checksum, result.fullTableScan.checksum);
    EXPECT_TRUE(result.indexScan.pageAccesses < result.fullTableScan.pageAccesses);
}

TEST_CASE(scan_benchmark_detects_an_incorrect_index_result) {
    const auto records = fixtures::sequentialRecords(10);
    const std::vector<std::int64_t> keys{5};
    bool detected = false;
    try {
        (void)db::benchmark::compareScans(
            records, keys,
            [](auto) {
                return db::benchmark::LookupResult<std::int64_t>{true, 99, 1};
            },
            [](const auto& record) { return record.key; },
            [](const auto& record) { return record.rowId; });
    } catch (const std::logic_error&) {
        detected = true;
    }
    EXPECT_TRUE(detected);
}

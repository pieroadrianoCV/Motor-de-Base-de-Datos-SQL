#include "test_framework.hpp"

#include "benchmark/IndexedBulkLoader.hpp"
#include "benchmark/ScanBenchmark.hpp"
#include "index/BTree.hpp"
#include "index/KeyExtractor.hpp"
#include "storage/RecordManager.hpp"
#include "storage/Tuple.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

std::vector<storage::Tuple> indexedTuples(std::size_t count) {
    std::vector<storage::Tuple> tuples;
    tuples.reserve(count);
    for (std::size_t position = 0; position < count; ++position) {
        tuples.push_back(storage::Tuple{
            static_cast<std::int64_t>(position),
            std::string{"value-"} + std::to_string(position)});
    }
    return tuples;
}

}  // namespace

TEST_CASE(real_btree_scan_and_real_storage_scan_return_identical_results) {
    storage::RecordManager records;
    db::index::BTree index(8);
    const auto tuples = indexedTuples(1'000);
    (void)db::benchmark::loadIndexedTuples(tuples, records, index, 0);
    const auto storedRecords = records.scan();
    const std::vector<std::int64_t> keys{0, 50, 500, 999, 1'500};

    const auto result = db::benchmark::comparePagedScans(
        storedRecords, keys, [&](auto key) { return index.search(key); },
        [](const storage::Record& record) {
            return db::index::keyFromTuple(record.tuple, 0);
        },
        [](const storage::Record& record) { return record.id; },
        [](const storage::Record& record) { return storage::pageID(record.id); });

    EXPECT_EQ(result.indexScan.found, 4U);
    EXPECT_EQ(result.fullTableScan.found, 4U);
    EXPECT_EQ(result.indexScan.checksum, result.fullTableScan.checksum);
    EXPECT_TRUE(result.indexScan.pageAccesses < result.fullTableScan.pageAccesses);
    EXPECT_TRUE(index.validate());
}

TEST_CASE(real_scan_benchmark_reports_absent_keys_consistently) {
    storage::RecordManager records;
    db::index::BTree index(2);
    const std::vector<storage::Tuple> tuples{
        storage::Tuple{std::int64_t{10}, std::string{"ten"}},
        storage::Tuple{std::int64_t{20}, std::string{"twenty"}}};
    (void)db::benchmark::loadIndexedTuples(tuples, records, index, 0);
    const auto storedRecords = records.scan();
    const std::vector<std::int64_t> keys{5, 30};

    const auto result = db::benchmark::comparePagedScans(
        storedRecords, keys, [&](auto key) { return index.search(key); },
        [](const storage::Record& record) {
            return db::index::keyFromTuple(record.tuple, 0);
        },
        [](const storage::Record& record) { return record.id; },
        [](const storage::Record& record) { return storage::pageID(record.id); });

    EXPECT_EQ(result.indexScan.found, 0U);
    EXPECT_EQ(result.fullTableScan.found, 0U);
    EXPECT_EQ(result.indexScan.checksum, 0U);
}

#include "test_framework.hpp"

#include "benchmark/IndexedBulkLoader.hpp"
#include "index/BTree.hpp"
#include "storage/RecordManager.hpp"
#include "storage/Tuple.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace {

storage::Tuple tuple(std::int64_t key, const std::string& value) {
    return storage::Tuple{key, value};
}

std::vector<storage::Tuple> workload(std::size_t count) {
    std::vector<storage::Tuple> tuples;
    tuples.reserve(count);
    for (std::size_t position = 0; position < count; ++position) {
        tuples.push_back(tuple(static_cast<std::int64_t>(position),
                               "record-" + std::to_string(position)));
    }
    std::mt19937 generator(42);
    std::shuffle(tuples.begin(), tuples.end(), generator);
    return tuples;
}

}  // namespace

TEST_CASE(bulk_load_stores_and_indexes_real_tuples) {
    const std::vector<storage::Tuple> tuples{
        tuple(30, "c"), tuple(10, "a"), tuple(20, "b"), tuple(40, "d")};
    storage::RecordManager records;
    db::index::BTree index(3);
    const auto result = db::benchmark::loadIndexedTuples(
        tuples, records, index, 0, db::benchmark::BulkLoadOptions{2});

    EXPECT_EQ(result.attempted, 4U);
    EXPECT_EQ(result.inserted, 4U);
    EXPECT_EQ(result.rejected, 0U);
    EXPECT_EQ(records.size(), 4U);
    EXPECT_EQ(index.size(), 4U);
    EXPECT_TRUE(index.validate());
    for (const auto& source : tuples) {
        const auto key = db::index::keyFromTuple(source, 0);
        const auto found = index.search(key);
        EXPECT_TRUE(found.found);
        const auto stored = records.get(found.rowId);
        EXPECT_TRUE(stored.has_value());
        EXPECT_TRUE(stored->tuple == source);
    }
}

TEST_CASE(bulk_load_rejects_duplicate_keys_without_orphan_records) {
    const std::vector<storage::Tuple> tuples{
        tuple(7, "first"), tuple(7, "duplicate"), tuple(8, "second")};
    storage::RecordManager records;
    db::index::BTree index(2);
    const auto result = db::benchmark::loadIndexedTuples(tuples, records, index, 0);

    EXPECT_EQ(result.inserted, 2U);
    EXPECT_EQ(result.rejected, 1U);
    EXPECT_EQ(records.size(), 2U);
    EXPECT_EQ(index.size(), 2U);
    EXPECT_TRUE(index.validate());
    const auto stored = records.get(index.search(7).rowId);
    EXPECT_TRUE(stored.has_value());
    EXPECT_TRUE(stored->tuple == tuples.front());
}

TEST_CASE(bulk_load_keeps_btree_balanced_after_ten_thousand_insertions) {
    const auto tuples = workload(10'000);
    storage::RecordManager records;
    db::index::BTree index(16);
    const auto result = db::benchmark::loadIndexedTuples(
        tuples, records, index, 0, db::benchmark::BulkLoadOptions{257});

    EXPECT_EQ(result.inserted, 10'000U);
    EXPECT_EQ(records.size(), 10'000U);
    EXPECT_EQ(index.size(), 10'000U);
    EXPECT_TRUE(index.height() > 1U);
    EXPECT_TRUE(index.validate());
    for (std::int64_t key : {0, 1, 127, 999, 5'000, 9'999}) {
        const auto found = index.search(key);
        EXPECT_TRUE(found.found);
        const auto stored = records.get(found.rowId);
        EXPECT_TRUE(stored.has_value());
        EXPECT_EQ(db::index::keyFromTuple(stored->tuple, 0), key);
    }
}

TEST_CASE(bulk_load_validates_an_empty_real_index) {
    const std::vector<storage::Tuple> tuples;
    storage::RecordManager records;
    db::index::BTree index(2);
    const auto result = db::benchmark::loadIndexedTuples(tuples, records, index, 0);

    EXPECT_EQ(result.attempted, 0U);
    EXPECT_EQ(result.validations, 1U);
    EXPECT_TRUE(records.empty());
    EXPECT_TRUE(index.empty());
    EXPECT_TRUE(index.validate());
}

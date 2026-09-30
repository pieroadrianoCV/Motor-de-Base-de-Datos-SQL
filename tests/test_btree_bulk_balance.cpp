#include "test_framework.hpp"

#include "benchmark/IndexedBulkLoader.hpp"
#include "index/BTree.hpp"
#include "index/BTreeNode.hpp"
#include "storage/RecordManager.hpp"
#include "storage/Tuple.hpp"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

namespace {

std::size_t verifyNode(const db::index::BTreeNode* node, std::size_t degree,
                       bool root, std::size_t depth,
                       std::size_t& expectedLeafDepth) {
    EXPECT_TRUE(node != nullptr);
    const auto keyCount = node->keyCount();
    EXPECT_TRUE(keyCount <= 2 * degree - 1);
    if (!root) {
        EXPECT_TRUE(keyCount >= degree - 1);
    }

    for (std::size_t position = 1; position < keyCount; ++position) {
        EXPECT_TRUE(node->entries()[position - 1].key <
                    node->entries()[position].key);
    }

    if (node->isLeaf()) {
        EXPECT_TRUE(node->children().empty());
        if (expectedLeafDepth == 0) {
            expectedLeafDepth = depth;
        }
        EXPECT_EQ(depth, expectedLeafDepth);
        return keyCount;
    }

    EXPECT_EQ(node->children().size(), keyCount + 1);
    std::size_t total = keyCount;
    for (const auto& child : node->children()) {
        total += verifyNode(child.get(), degree, false, depth + 1,
                            expectedLeafDepth);
    }
    return total;
}

std::vector<storage::Tuple> tuplesFromKeys(const std::vector<std::int64_t>& keys) {
    std::vector<storage::Tuple> tuples;
    tuples.reserve(keys.size());
    for (const auto key : keys) {
        tuples.push_back(storage::Tuple{key, std::string{"payload"}});
    }
    return tuples;
}

void verifyWorkload(std::size_t degree, std::vector<std::int64_t> keys) {
    storage::RecordManager records;
    db::index::BTree index(degree);
    const auto tuples = tuplesFromKeys(keys);
    const auto load = db::benchmark::loadIndexedTuples(
        tuples, records, index, 0, db::benchmark::BulkLoadOptions{31});

    EXPECT_EQ(load.inserted, keys.size());
    EXPECT_EQ(index.size(), keys.size());
    EXPECT_EQ(records.size(), keys.size());
    EXPECT_TRUE(index.validate());

    std::size_t leafDepth = 0;
    EXPECT_EQ(verifyNode(index.root(), degree, true, 1, leafDepth), keys.size());
    EXPECT_EQ(leafDepth, index.height());
    for (const auto key : keys) {
        const auto result = index.search(key);
        EXPECT_TRUE(result.found);
        EXPECT_TRUE(records.exists(result.rowId));
    }
}

}  // namespace

TEST_CASE(first_overflow_splits_the_real_root) {
    storage::RecordManager records;
    db::index::BTree index(2);
    const auto tuples = tuplesFromKeys({10, 20, 30, 40});
    (void)db::benchmark::loadIndexedTuples(tuples, records, index, 0);

    EXPECT_EQ(index.height(), 2U);
    EXPECT_TRUE(!index.root()->isLeaf());
    EXPECT_EQ(index.root()->keyCount(), 1U);
    EXPECT_EQ(index.root()->children().size(), 2U);
    EXPECT_TRUE(index.validate());
}

TEST_CASE(bulk_load_preserves_invariants_for_multiple_degrees_and_orders) {
    std::vector<std::int64_t> ascending(2'000);
    std::iota(ascending.begin(), ascending.end(), 1);
    auto descending = ascending;
    std::reverse(descending.begin(), descending.end());

    verifyWorkload(2, ascending);
    verifyWorkload(3, descending);
    verifyWorkload(8, ascending);
    verifyWorkload(32, descending);
}

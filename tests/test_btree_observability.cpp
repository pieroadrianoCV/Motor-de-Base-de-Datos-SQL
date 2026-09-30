#include "test_framework.hpp"

#include "index/BTree.hpp"
#include "storage/RowID.hpp"

#include <cstdint>
#include <vector>

TEST_CASE(split_observer_reports_real_node_divisions) {
    db::index::BTree tree(2);
    std::vector<db::index::SplitEvent> events;
    tree.setSplitObserver(
        [&](const db::index::SplitEvent& event) { events.push_back(event); });

    for (std::int64_t key = 1; key <= 100; ++key) {
        EXPECT_TRUE(tree.insert(key, static_cast<storage::RowID>(key)));
    }

    EXPECT_TRUE(tree.validate());
    EXPECT_TRUE(!events.empty());
    EXPECT_EQ(events.size(), tree.splitCount());
    EXPECT_TRUE(tree.height() > 1U);
    for (const auto& event : events) {
        EXPECT_EQ(event.leftKeyCount, 1U);
        EXPECT_EQ(event.rightKeyCount, 1U);
        EXPECT_TRUE(event.parentKeyCount >= 1U);
        EXPECT_TRUE(event.treeHeight >= 2U);
    }
}

TEST_CASE(split_logging_does_not_change_search_results) {
    db::index::BTree tree(3);
    std::size_t observed = 0;
    tree.setSplitObserver([&](const auto&) { ++observed; });
    for (std::int64_t key = 200; key >= -200; --key) {
        EXPECT_TRUE(tree.insert(key, static_cast<storage::RowID>(key + 201)));
    }

    EXPECT_EQ(observed, tree.splitCount());
    EXPECT_TRUE(tree.validate());
    for (std::int64_t key = -200; key <= 200; ++key) {
        EXPECT_TRUE(tree.search(key).found);
    }
}

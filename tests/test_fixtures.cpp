#include "fixtures.hpp"
#include "test_framework.hpp"

#include <stdexcept>

TEST_CASE(sequential_fixture_assigns_stable_row_ids) {
    const auto records = fixtures::sequentialRecords(100);
    EXPECT_EQ(records.size(), 100U);
    EXPECT_EQ(records.front().key, 0);
    EXPECT_EQ(records.back().rowId, 99);
}

TEST_CASE(shuffled_fixture_is_reproducible) {
    const auto first = fixtures::shuffledRecords(100, 123);
    const auto second = fixtures::shuffledRecords(100, 123);
    for (std::size_t index = 0; index < first.size(); ++index) {
        EXPECT_EQ(first[index].key, second[index].key);
    }
}

TEST_CASE(query_fixture_rejects_an_empty_table) {
    bool rejected = false;
    try {
        (void)fixtures::queryKeys(0, 1);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    EXPECT_TRUE(rejected);
}

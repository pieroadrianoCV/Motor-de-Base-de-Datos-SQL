#include "fixtures.hpp"
#include "test_framework.hpp"

#include "benchmark/BulkLoader.hpp"

#include <cstdint>
#include <set>
#include <vector>

namespace {

class FakeIndex {
public:
    bool insert(const fixtures::Record& record) {
        return keys_.insert(record.key).second;
    }

    bool valid() const { return valid_; }
    void invalidate() { valid_ = false; }
    std::size_t size() const { return keys_.size(); }

private:
    std::set<std::int64_t> keys_;
    bool valid_{true};
};

}  // namespace

TEST_CASE(bulk_load_inserts_ten_thousand_shuffled_records) {
    const auto records = fixtures::shuffledRecords(10'000);
    FakeIndex index;

    const auto result = db::benchmark::bulkLoad(
        records,
        [&](const auto& record) { return index.insert(record); },
        [&] { return index.valid(); }, db::benchmark::BulkLoadOptions{257});

    EXPECT_EQ(result.attempted, 10'000U);
    EXPECT_EQ(result.inserted, 10'000U);
    EXPECT_EQ(result.rejected, 0U);
    EXPECT_EQ(result.validations, 39U);
    EXPECT_EQ(index.size(), 10'000U);
}

TEST_CASE(bulk_load_reports_duplicate_keys) {
    const std::vector<fixtures::Record> records{{7, 1}, {7, 2}, {8, 3}};
    FakeIndex index;

    const auto result = db::benchmark::bulkLoad(
        records,
        [&](const auto& record) { return index.insert(record); },
        [&] { return index.valid(); });

    EXPECT_EQ(result.inserted, 2U);
    EXPECT_EQ(result.rejected, 1U);
    EXPECT_EQ(index.size(), 2U);
}

TEST_CASE(bulk_load_validates_an_empty_index) {
    const std::vector<fixtures::Record> records;
    FakeIndex index;
    const auto result = db::benchmark::bulkLoad(
        records,
        [&](const auto& record) { return index.insert(record); },
        [&] { return index.valid(); });

    EXPECT_EQ(result.attempted, 0U);
    EXPECT_EQ(result.validations, 1U);
}

TEST_CASE(bulk_load_stops_when_balance_validation_fails) {
    const auto records = fixtures::sequentialRecords(20);
    FakeIndex index;
    std::size_t insertions = 0;
    bool detected = false;

    try {
        (void)db::benchmark::bulkLoad(
            records,
            [&](const auto& record) {
                ++insertions;
                if (insertions == 6) {
                    index.invalidate();
                }
                return index.insert(record);
            },
            [&] { return index.valid(); }, db::benchmark::BulkLoadOptions{3});
    } catch (const db::benchmark::BalanceError&) {
        detected = true;
    }

    EXPECT_TRUE(detected);
    EXPECT_EQ(insertions, 6U);
}

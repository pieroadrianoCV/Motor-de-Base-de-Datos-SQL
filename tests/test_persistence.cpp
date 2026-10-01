#include "test_framework.hpp"

#include "storage/DatabaseFile.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

class TemporaryDatabase {
public:
    explicit TemporaryDatabase(std::string name)
        : path(std::filesystem::temp_directory_path() / std::move(name)) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        std::filesystem::remove(path.string() + ".tmp", ignored);
    }

    ~TemporaryDatabase() {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        std::filesystem::remove(path.string() + ".tmp", ignored);
    }

    std::filesystem::path path;
};

}  // namespace

TEST_CASE(database_file_round_trips_degree_and_tuples) {
    TemporaryDatabase database("eda_phase1_roundtrip.bin");
    const std::vector<storage::Tuple> tuples{
        storage::Tuple{std::int64_t{-7}, 2.5, std::string{"first"}},
        storage::Tuple{std::int64_t{99}, -0.125, std::string{"UTF-8: árbol"}}};

    storage::DatabaseFile::save(database.path, 32, tuples);
    const auto restored = storage::DatabaseFile::load(database.path);

    EXPECT_EQ(restored.degree, 32U);
    EXPECT_TRUE(restored.tuples == tuples);
}

TEST_CASE(database_file_can_atomically_replace_an_existing_database) {
    TemporaryDatabase database("eda_phase1_replace.bin");
    storage::DatabaseFile::save(
        database.path, 2,
        std::vector<storage::Tuple>{storage::Tuple{std::int64_t{1}}});
    const std::vector<storage::Tuple> replacement{
        storage::Tuple{std::int64_t{10}, std::string{"ten"}},
        storage::Tuple{std::int64_t{20}, std::string{"twenty"}}};

    storage::DatabaseFile::save(database.path, 8, replacement);
    const auto restored = storage::DatabaseFile::load(database.path);

    EXPECT_EQ(restored.degree, 8U);
    EXPECT_TRUE(restored.tuples == replacement);
    EXPECT_TRUE(!std::filesystem::exists(database.path.string() + ".tmp"));
}

TEST_CASE(database_file_rejects_corruption_and_truncation) {
    TemporaryDatabase corrupted("eda_phase1_corrupted.bin");
    storage::DatabaseFile::save(
        corrupted.path, 4,
        std::vector<storage::Tuple>{storage::Tuple{std::int64_t{42}}});
    {
        std::fstream file(corrupted.path,
                          std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(0);
        file.put('X');
    }
    bool corruptionRejected = false;
    try {
        (void)storage::DatabaseFile::load(corrupted.path);
    } catch (const std::runtime_error&) {
        corruptionRejected = true;
    }
    EXPECT_TRUE(corruptionRejected);

    TemporaryDatabase truncated("eda_phase1_truncated.bin");
    {
        std::ofstream file(truncated.path, std::ios::binary);
        file.write("EDADB", 5);
    }
    bool truncationRejected = false;
    try {
        (void)storage::DatabaseFile::load(truncated.path);
    } catch (const std::runtime_error&) {
        truncationRejected = true;
    }
    EXPECT_TRUE(truncationRejected);
}

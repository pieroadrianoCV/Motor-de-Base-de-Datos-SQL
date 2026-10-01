#include "test_framework.hpp"

#include "demo/DemoEngine.hpp"

#include <filesystem>
#include <string>

TEST_CASE(demo_engine_runs_the_complete_phase_one_flow) {
    db::demo::DemoEngine demo;
    const auto tuples = db::demo::DemoEngine::makeWorkload(2'000);
    const auto load = demo.load(tuples, 8, 100);

    EXPECT_EQ(load.load.inserted, 2'000U);
    EXPECT_TRUE(load.pages > 1U);
    EXPECT_TRUE(load.height > 1U);
    EXPECT_TRUE(load.splits > 0U);
    EXPECT_TRUE(demo.index().validate());

    const auto splits = demo.splitReport(5);
    const auto tree = demo.treeReport(10);
    EXPECT_TRUE(splits.find("Splits registrados") != std::string::npos);
    EXPECT_TRUE(splits.find("promovida=") != std::string::npos);
    EXPECT_TRUE(tree.find("Nivel 0") != std::string::npos);

    const auto comparison = demo.benchmarkScans(100);
    EXPECT_EQ(comparison.indexScan.found, 100U);
    EXPECT_EQ(comparison.fullTableScan.found, 100U);
    EXPECT_EQ(comparison.indexScan.checksum,
              comparison.fullTableScan.checksum);
    EXPECT_TRUE(comparison.indexScan.pageAccesses <
                comparison.fullTableScan.pageAccesses);
}

TEST_CASE(demo_requires_bulk_load_before_reports_or_benchmark) {
    db::demo::DemoEngine demo;
    bool reportRejected = false;
    bool benchmarkRejected = false;
    try {
        (void)demo.treeReport();
    } catch (const std::logic_error&) {
        reportRejected = true;
    }
    try {
        (void)demo.benchmarkScans(10);
    } catch (const std::logic_error&) {
        benchmarkRejected = true;
    }
    EXPECT_TRUE(reportRejected);
    EXPECT_TRUE(benchmarkRejected);
}

TEST_CASE(demo_persists_reopens_and_queries_records) {
    const auto path =
        std::filesystem::temp_directory_path() / "eda_phase1_demo.bin";
    std::error_code ignored;
    std::filesystem::remove(path, ignored);

    {
        db::demo::DemoEngine writer;
        const auto tuples = db::demo::DemoEngine::makeWorkload(500);
        (void)writer.load(tuples, 8);
        writer.saveFile(path);
    }

    db::demo::DemoEngine reader;
    const auto summary = reader.loadFile(path);
    EXPECT_EQ(summary.load.inserted, 500U);
    EXPECT_EQ(reader.index().degree(), 8U);
    const auto found = reader.find(42);
    EXPECT_TRUE(found.found);
    EXPECT_TRUE(found.tuple.has_value());
    EXPECT_EQ(db::index::keyFromTuple(*found.tuple, 0), 42);
    EXPECT_TRUE(found.pageId != storage::INVALID_PAGE_ID);
    EXPECT_TRUE(reader.find(1'000).found == false);

    std::filesystem::remove(path, ignored);
}

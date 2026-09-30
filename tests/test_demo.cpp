#include "test_framework.hpp"

#include "demo/DemoEngine.hpp"

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

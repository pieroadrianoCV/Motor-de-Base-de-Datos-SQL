#include "test_framework.hpp"

#include "../src/index/BTree.hpp"
#include "../src/index/KeyExtractor.hpp"
#include "../src/storage/RowID.hpp"
#include "../src/storage/Tuple.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  using db::index::BTree;
  using db::index::BTreeNode;
  using db::index::Entry;
  using db::index::keyFromTuple;

  storage::Tuple row(std::int64_t id, const std::string& name) {
    return storage::Tuple{id, name};
  }

}

TEST_CASE(key_is_extracted_from_an_int64_column) {
  EXPECT_EQ(keyFromTuple(row(42, "ana"), 0), 42);
}

TEST_CASE(key_extraction_rejects_bad_columns) {
  bool wrongType = false;
  bool outOfRange = false;
  try {
    (void)keyFromTuple(row(1, "ana"), 1);
  } catch (const std::invalid_argument&) {
    wrongType = true;
  }
  try {
    (void)keyFromTuple(row(1, "ana"), 5);
  } catch (const std::out_of_range&) {
    outOfRange = true;
  }
  EXPECT_TRUE(wrongType);
  EXPECT_TRUE(outOfRange);
}

TEST_CASE(btree_returns_the_storage_row_id_of_an_indexed_tuple) {
  const std::vector<storage::Tuple> tuples{row(30, "c"), row(10, "a"),
    row(20, "b")};
  auto leaf = std::make_unique<BTreeNode>(true);
  leaf->entries().push_back(Entry{keyFromTuple(tuples[1], 0), 2});
  leaf->entries().push_back(Entry{keyFromTuple(tuples[2], 0), 3});
  leaf->entries().push_back(Entry{keyFromTuple(tuples[0], 0), 1});
  BTree tree(2);
  tree.setRoot(std::move(leaf));
  tree.setSize(3);

  EXPECT_TRUE(tree.validate());
  EXPECT_EQ(tree.search(10).rowId, static_cast<storage::RowID>(2));
  EXPECT_EQ(tree.search(20).rowId, static_cast<storage::RowID>(3));
  EXPECT_EQ(tree.search(30).rowId, static_cast<storage::RowID>(1));
  const auto missing = tree.search(99);
  EXPECT_TRUE(!missing.found);
  EXPECT_EQ(missing.rowId, storage::INVALID_ROW_ID);
}

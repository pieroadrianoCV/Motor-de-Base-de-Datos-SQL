#include "test_framework.hpp"

#include "../src/index/BTree.hpp"

#include <cstdint>
#include <memory>

namespace {

  using db::index::BTree;
  using db::index::BTreeNode;
  using db::index::Entry;

  storage::RowID rowIdFor(std::int64_t key) {
    return static_cast<storage::RowID>(key) * 10 + 1;
  }

  std::unique_ptr<BTreeNode> makeNode(bool leaf, std::vector<std::int64_t> keys) {
    auto node = std::make_unique<BTreeNode>(leaf);
    for (auto key : keys) {
      node->entries().push_back(Entry{key, rowIdFor(key)});
    }
    return node;
  }

  BTree smallTree() {
    BTree tree(2);
    auto root = makeNode(false, {10, 20});
    root->children().push_back(makeNode(true, {1, 5}));
    root->children().push_back(makeNode(true, {12, 15}));
    root->children().push_back(makeNode(true, {30, 40}));
    tree.setRoot(std::move(root));
    tree.setSize(8);
    return tree;
  }

  std::unique_ptr<BTreeNode> buildFull(std::size_t levels, std::int64_t& next) {
    const bool leaf = levels == 1;
    auto node = std::make_unique<BTreeNode>(leaf);
    if (leaf) {
      for (int i = 0; i < 3; ++i, ++next) node->entries().push_back({next, rowIdFor(next)});
      return node;
    }
    for (int i = 0; i < 4; ++i) {
      node->children().push_back(buildFull(levels - 1, next));
      if (i < 3) {
        node->entries().push_back({next, rowIdFor(next)});
        ++next;
      }
    }
    return node;
  }

}

TEST_CASE(btree_rejects_degree_below_two) {
  bool rejected = false;
  try {
    BTree tree(1);
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  EXPECT_TRUE(rejected);
}

TEST_CASE(btree_starts_as_an_empty_valid_leaf) {
  BTree tree(3);
  EXPECT_TRUE(tree.empty());
  EXPECT_EQ(tree.height(), 1U);
  EXPECT_EQ(tree.maxKeys(), 5U);
  EXPECT_EQ(tree.minKeys(), 2U);
  EXPECT_TRUE(tree.validate());
  const auto result = tree.search(42);
  EXPECT_TRUE(!result.found);
  EXPECT_EQ(result.rowId, storage::INVALID_ROW_ID);
  EXPECT_EQ(result.pageAccesses, 1U);
}

TEST_CASE(node_locate_finds_keys_and_child_positions) {
  const auto node = makeNode(false, {10, 20, 30});
  EXPECT_TRUE(node->locate(20).found);
  EXPECT_EQ(node->locate(20).index, 1U);
  EXPECT_TRUE(!node->locate(5).found);
  EXPECT_EQ(node->locate(5).index, 0U);
  EXPECT_EQ(node->locate(25).index, 2U);
  EXPECT_EQ(node->locate(99).index, 3U);
}

TEST_CASE(node_capacity_follows_minimum_degree) {
  const auto node = makeNode(true, {1, 2, 3});
  EXPECT_TRUE(node->isFull(2));
  EXPECT_TRUE(!node->isFull(3));
  EXPECT_TRUE(node->hasMinimum(3));
  EXPECT_TRUE(!node->hasMinimum(5));
}

TEST_CASE(btree_search_finds_keys_in_leaves_and_internal_nodes) {
  const auto tree = smallTree();
  EXPECT_TRUE(tree.validate());
  EXPECT_EQ(tree.height(), 2U);
  for (std::int64_t key : {1, 5, 10, 12, 15, 20, 30, 40}) {
    const auto result = tree.search(key);
    EXPECT_TRUE(result.found);
    EXPECT_EQ(result.rowId, rowIdFor(key));
  }
  EXPECT_EQ(tree.search(10).pageAccesses, 1U);
  EXPECT_EQ(tree.search(15).pageAccesses, 2U);
}

TEST_CASE(btree_search_reports_missing_keys) {
  const auto tree = smallTree();
  for (std::int64_t key : {0, 7, 11, 25, 99}) {
    EXPECT_TRUE(!tree.search(key).found);
    EXPECT_TRUE(tree.search(key).pageAccesses <= tree.height());
  }
}

TEST_CASE(btree_search_is_logarithmic_in_a_full_tree) {
  std::int64_t next = 0;
  BTree tree(2);
  tree.setRoot(buildFull(5, next));
  tree.setSize(static_cast<std::size_t>(next));
  EXPECT_EQ(tree.size(), 1023U);
  EXPECT_EQ(tree.height(), 5U);
  EXPECT_TRUE(tree.validate());
  for (std::int64_t key = 0; key < 1023; ++key) {
    const auto result = tree.search(key);
    EXPECT_TRUE(result.found);
    EXPECT_EQ(result.rowId, rowIdFor(key));
    EXPECT_TRUE(result.pageAccesses <= 5);
  }
  EXPECT_TRUE(!tree.search(-1).found);
  EXPECT_TRUE(!tree.search(1023).found);
}

TEST_CASE(search_result_exposes_the_fields_used_by_the_benchmark) {
  const auto tree = smallTree();
  const auto lookup = [&](std::int64_t key) { return tree.search(key); };
  const auto hit = lookup(15);
  const bool found = hit.found;
  const storage::RowID rowId = hit.rowId;
  const std::size_t accesses = hit.pageAccesses;
  EXPECT_TRUE(found);
  EXPECT_EQ(rowId, rowIdFor(15));
  EXPECT_EQ(accesses, 2U);
}

TEST_CASE(validate_detects_leaves_at_different_depths) {
  BTree tree(2);
  auto root = makeNode(false, {10, 20});
  auto deep = makeNode(false, {3});
  deep->children().push_back(makeNode(true, {1, 2}));
  deep->children().push_back(makeNode(true, {4, 5}));
  root->children().push_back(std::move(deep));
  root->children().push_back(makeNode(true, {12, 15}));
  root->children().push_back(makeNode(true, {30, 40}));
  tree.setRoot(std::move(root));
  tree.setSize(11);
  EXPECT_TRUE(!tree.validate());
}

TEST_CASE(validate_detects_broken_links_and_ordering) {
  {
    BTree tree(2);
    auto root = makeNode(false, {10, 20});
    root->children().push_back(makeNode(true, {1, 5}));
    root->children().push_back(makeNode(true, {12, 15}));
    tree.setRoot(std::move(root));
    tree.setSize(6);
    EXPECT_TRUE(!tree.validate());
  }
  {
    BTree tree(2);
    auto root = makeNode(false, {10});
    root->children().push_back(makeNode(true, {1, 11}));
    root->children().push_back(makeNode(true, {30, 40}));
    tree.setRoot(std::move(root));
    tree.setSize(5);
    EXPECT_TRUE(!tree.validate());
  }
  {
    BTree tree(2);
    tree.setRoot(makeNode(true, {5, 3}));
    tree.setSize(2);
    EXPECT_TRUE(!tree.validate());
  }
  {
    BTree tree(2);
    auto root = makeNode(false, {10});
    root->children().push_back(makeNode(true, {1, 5}));
    root->children().push_back(nullptr);
    tree.setRoot(std::move(root));
    tree.setSize(3);
    EXPECT_TRUE(!tree.validate());
  }
}

TEST_CASE(validate_detects_capacity_violations_and_size_mismatch) {
  {
    BTree tree(3);
    auto root = makeNode(false, {50});
    root->children().push_back(makeNode(true, {1}));
    root->children().push_back(makeNode(true, {60, 70}));
    tree.setRoot(std::move(root));
    tree.setSize(4);
    EXPECT_TRUE(!tree.validate());
  }
  {
    BTree tree(2);
    tree.setRoot(makeNode(true, {1, 2, 3, 4}));
    tree.setSize(4);
    EXPECT_TRUE(!tree.validate());
  }
  {
    auto tree = smallTree();
    tree.setSize(7);
    EXPECT_TRUE(!tree.validate());
  }
}

#ifndef BTREENODE_H
#define BTREENODE_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../storage/RowID.hpp"

namespace db::index {

  using Key = std::int64_t;
  using RowID = storage::RowID;

  struct Entry {
    Key key{};
    RowID rowId{storage::INVALID_ROW_ID};
  };

  class BTreeNode {
    public:
      struct Position {
        std::size_t index{0};
        bool found{false};
      };

      explicit BTreeNode(bool leaf) : leaf_(leaf) {}

      bool isLeaf() const { return leaf_; }
      void setLeaf(bool leaf) { leaf_ = leaf; }

      std::size_t keyCount() const { return entries_.size(); }

      std::vector<Entry>& entries() { return entries_; }
      const std::vector<Entry>& entries() const { return entries_; }
      std::vector<std::unique_ptr<BTreeNode>>& children() { return children_; }
      const std::vector<std::unique_ptr<BTreeNode>>& children() const {
        return children_;
      }

      bool isFull(std::size_t t) const { return entries_.size() >= 2 * t - 1; }
      bool hasMinimum(std::size_t t) const { return entries_.size() >= t - 1; }

      Position locate(Key key) const {
        std::size_t first = 0;
        std::size_t last = entries_.size();
        while (first < last) {
          const auto middle = first + (last - first) / 2;
          if (entries_[middle].key < key) {
            first = middle + 1;
          } else {
            last = middle;
          }
        }
        return {first, first < entries_.size() && entries_[first].key == key};
      }

    private:
      bool leaf_;
      std::vector<Entry> entries_;
      std::vector<std::unique_ptr<BTreeNode>> children_;
  };
}

#endif //BTREENODE_H

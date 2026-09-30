#ifndef BTREE_H
#define BTREE_H

#include "../index/BTreeNode.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace db::index {

  struct SearchResult {
    bool found{false};
    RowID rowId{storage::INVALID_ROW_ID};
    std::size_t pageAccesses{0};
  };

  class BTree {
    public:
      explicit BTree(std::size_t minimumDegree) : t_(minimumDegree) {
        if (t_ < 2) {
          throw std::invalid_argument("el grado minimo t debe ser >= 2");
        }
        root_ = std::make_unique<BTreeNode>(true);
      }

      std::size_t degree() const { return t_; }
      std::size_t maxKeys() const { return 2 * t_ - 1; }
      std::size_t minKeys() const { return t_ - 1; }
      std::size_t size() const { return size_; }
      bool empty() const { return size_ == 0; }

      std::size_t height() const {
        std::size_t levels = 1;
        for (const BTreeNode* node = root_.get(); !node->isLeaf();
            node = node->children().front().get()) {
          ++levels;
        }
        return levels;
      }

      SearchResult search(Key key) const {
        SearchResult result;
        const BTreeNode* node = root_.get();
        while (node != nullptr) {
          ++result.pageAccesses;
          const auto position = node->locate(key);
          if (position.found) {
            result.found = true;
            result.rowId = node->entries()[position.index].rowId;
            return result;
          }
          if (node->isLeaf()) {
            return result;
          }
          node = node->children()[position.index].get();
        }
        return result;
      }

      bool contains(Key key) const { return search(key).found; }

      bool validate() const {
        std::size_t leafDepth = 0;
        std::size_t count = 0;
        if (!validateNode(root_.get(), std::nullopt, std::nullopt, 1, leafDepth,
              count, true)) {
          return false;
        }
        return count == size_;
      }

      bool insert(Key, RowID) {
        throw std::logic_error(
            "BTree::insert pendiente: lo implementa Integrante 3 "
            "(feature/btree-balance)");
      }

      BTreeNode* root() { return root_.get(); }
      const BTreeNode* root() const { return root_.get(); }

      void setRoot(std::unique_ptr<BTreeNode> newRoot) {
        if (!newRoot) {
          throw std::invalid_argument("la raiz no puede ser nula");
        }
        root_ = std::move(newRoot);
      }
      void setSize(std::size_t newSize) { size_ = newSize; }

    private:
      using Bound = std::optional<Key>;

      bool validateNode(const BTreeNode* node, Bound low, Bound high,
          std::size_t depth, std::size_t& leafDepth,
          std::size_t& count, bool isRoot) const {
        if (node == nullptr) {
          return false;
        }
        const auto& entries = node->entries();
        const auto& children = node->children();

        if (entries.size() > maxKeys()) return false;
        if (!isRoot && entries.size() < minKeys()) return false;
        if (!isRoot && entries.empty()) return false;
        if (!node->isLeaf() && entries.empty()) return false;

        for (std::size_t i = 0; i < entries.size(); ++i) {
          if (i > 0 && !(entries[i - 1].key < entries[i].key)) return false;
          if (low && !(*low < entries[i].key)) return false;
          if (high && !(entries[i].key < *high)) return false;
        }
        count += entries.size();

        if (node->isLeaf()) {
          if (!children.empty()) return false;
          if (leafDepth == 0) {
            leafDepth = depth;
          }
          return leafDepth == depth;
        }

        if (children.size() != entries.size() + 1) return false;
        for (std::size_t i = 0; i < children.size(); ++i) {
          const Bound childLow = i == 0 ? low : Bound{entries[i - 1].key};
          const Bound childHigh =
            i == entries.size() ? high : Bound{entries[i].key};
          if (!validateNode(children[i].get(), childLow, childHigh, depth + 1,
                leafDepth, count, false)) {
            return false;
          }
        }
        return true;
      }

      std::size_t t_;
      std::size_t size_{0};
      std::unique_ptr<BTreeNode> root_;
  };

}

#endif //BTREE_H

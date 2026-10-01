#pragma once

#include "benchmark/IndexedBulkLoader.hpp"
#include "benchmark/ScanBenchmark.hpp"
#include "index/BTree.hpp"
#include "index/KeyExtractor.hpp"
#include "storage/RecordManager.hpp"
#include "storage/DatabaseFile.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace db::demo {

struct LoadSummary {
  benchmark::BulkLoadResult load;
  std::size_t pages{0};
  std::size_t height{0};
  std::size_t splits{0};
};

struct QueryResult {
  bool found{false};
  storage::RowID rowId{storage::INVALID_ROW_ID};
  storage::PageID pageId{storage::INVALID_PAGE_ID};
  storage::SlotID slotId{0};
  std::size_t indexPageAccesses{0};
  std::optional<storage::Tuple> tuple;
};

class DemoEngine {
public:
  static std::vector<storage::Tuple> makeWorkload(std::size_t count)
  {
    std::vector<storage::Tuple> tuples;
    tuples.reserve(count);
    for (std::size_t position = 0; position < count; ++position) {
      tuples.push_back(storage::Tuple{
        static_cast<std::int64_t>(position),
        std::string{"record-"} + std::to_string(position)});
    }
    std::mt19937 generator(42);
    std::shuffle(tuples.begin(), tuples.end(), generator);
    return tuples;
  }

  LoadSummary load(const std::vector<storage::Tuple>& tuples,
                   std::size_t degree,
                   std::size_t validationInterval = 0)
  {
    if (degree < 2) {
      throw std::invalid_argument("el grado minimo debe ser al menos 2");
    }

    records_ = std::make_unique<storage::RecordManager>();
    index_ = std::make_unique<index::BTree>(degree);
    splitEvents_.clear();
    index_->setSplitObserver(
      [this](const index::SplitEvent& event) { splitEvents_.push_back(event); });

    const auto result = benchmark::loadIndexedTuples(
      tuples, *records_, *index_, 0,
      benchmark::BulkLoadOptions{validationInterval});
    if (!index_->validate() || result.inserted != records_->size() ||
        result.inserted != index_->size()) {
      throw std::logic_error("la carga dejo storage e indice inconsistentes");
    }
    return {result, records_->pageCount(), index_->height(), index_->splitCount()};
  }

  [[nodiscard]] bool loaded() const
  {
    return records_ != nullptr && index_ != nullptr;
  }

  LoadSummary loadFile(const std::filesystem::path& path,
                       std::size_t validationInterval = 0)
  {
    const auto image = storage::DatabaseFile::load(path);
    return load(image.tuples, image.degree, validationInterval);
  }

  void saveFile(const std::filesystem::path& path) const
  {
    requireLoaded();
    std::vector<storage::Tuple> tuples;
    const auto records = records_->scan();
    tuples.reserve(records.size());
    for (const auto& record : records) {
      tuples.push_back(record.tuple);
    }
    storage::DatabaseFile::save(path, index_->degree(), tuples);
  }

  [[nodiscard]] QueryResult find(index::Key key) const
  {
    requireLoaded();
    const auto lookup = index_->search(key);
    if (!lookup.found) {
      return {false, storage::INVALID_ROW_ID, storage::INVALID_PAGE_ID, 0,
              lookup.pageAccesses, std::nullopt};
    }
    const auto record = records_->get(lookup.rowId);
    if (!record.has_value()) {
      throw std::logic_error("el indice referencia un RowID inexistente");
    }
    return {true, lookup.rowId, storage::pageID(lookup.rowId),
            storage::slotID(lookup.rowId), lookup.pageAccesses, record->tuple};
  }

  benchmark::ComparisonResult benchmarkScans(std::size_t queryCount) const
  {
    requireLoaded();
    if (queryCount == 0 || index_->empty()) {
      throw std::invalid_argument("se requieren datos y al menos una consulta");
    }

    const auto records = records_->scan();
    std::vector<index::Key> availableKeys;
    availableKeys.reserve(records.size());
    for (const auto& record : records) {
      availableKeys.push_back(index::keyFromTuple(record.tuple, 0));
    }
    std::mt19937 generator(73);
    std::uniform_int_distribution<std::size_t> distribution(
      0, availableKeys.size() - 1);
    std::vector<std::int64_t> keys;
    keys.reserve(queryCount);
    for (std::size_t query = 0; query < queryCount; ++query) {
      keys.push_back(availableKeys[distribution(generator)]);
    }

    return benchmark::comparePagedScans(
      records, keys, [this](auto key) { return index_->search(key); },
      [](const storage::Record& record) {
        return index::keyFromTuple(record.tuple, 0);
      },
      [](const storage::Record& record) { return record.id; },
      [](const storage::Record& record) { return storage::pageID(record.id); });
  }

  [[nodiscard]] std::string splitReport(std::size_t maximumEvents = 20) const
  {
    requireLoaded();
    std::ostringstream output;
    output << "Splits registrados: " << splitEvents_.size() << '\n';
    const auto shown = std::min(maximumEvents, splitEvents_.size());
    for (std::size_t position = 0; position < shown; ++position) {
      const auto& event = splitEvents_[position];
      output << "  #" << position + 1 << " promovida=" << event.promotedKey
             << " izquierda=" << event.leftKeyCount
             << " derecha=" << event.rightKeyCount
             << " altura=" << event.treeHeight << '\n';
    }
    if (shown < splitEvents_.size()) {
      output << "  ... " << splitEvents_.size() - shown
             << " splits adicionales\n";
    }
    return output.str();
  }

  [[nodiscard]] std::string treeReport(std::size_t maximumNodes = 32,
                                       std::size_t maximumKeysPerNode = 12) const
  {
    requireLoaded();
    std::ostringstream output;
    std::queue<std::pair<const index::BTreeNode*, std::size_t>> pending;
    pending.push({index_->root(), 0});
    std::size_t currentLevel = static_cast<std::size_t>(-1);
    std::size_t shown = 0;
    while (!pending.empty() && shown < maximumNodes) {
      const auto [node, level] = pending.front();
      pending.pop();
      if (level != currentLevel) {
        currentLevel = level;
        output << (level == 0 ? "" : "\n") << "Nivel " << level << ": ";
      }
      output << '[';
      const auto shownKeys = std::min(maximumKeysPerNode, node->entries().size());
      for (std::size_t key = 0; key < shownKeys; ++key) {
        if (key != 0) output << ',';
        output << node->entries()[key].key;
      }
      if (shownKeys < node->entries().size()) output << ",...";
      output << "] ";
      ++shown;
      for (const auto& child : node->children()) {
        pending.push({child.get(), level + 1});
      }
    }
    if (!pending.empty()) output << "\n... visualizacion truncada";
    output << '\n';
    return output.str();
  }

  [[nodiscard]] const index::BTree& index() const
  {
    requireLoaded();
    return *index_;
  }

private:
  void requireLoaded() const
  {
    if (!loaded()) {
      throw std::logic_error("primero debe realizar una carga masiva");
    }
  }

  std::unique_ptr<storage::RecordManager> records_;
  std::unique_ptr<index::BTree> index_;
  std::vector<index::SplitEvent> splitEvents_;
};

}  // namespace db::demo

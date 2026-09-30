#pragma once

#include "Record.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace storage {

class RecordManager {
public:
  RecordManager() = default;

  RowID insert(Tuple tuple)
  {
    const RowID id = m_nextRowID++;

    m_records.emplace(
      id,
      Record{id, std::move(tuple)});

    return id;
  }

  [[nodiscard]]
  std::optional<Record> get(RowID id) const
  {
    const auto it = m_records.find(id);

    if (it == m_records.end()) {
      return std::nullopt;
    }

    return it->second;
  }

  [[nodiscard]]
  bool exists(RowID id) const
  {
    return m_records.contains(id);
  }

  bool update(RowID id, Tuple tuple)
  {
    const auto it = m_records.find(id);

    if (it == m_records.end()) {
      return false;
    }

    it->second.tuple = std::move(tuple);

    return true;
  }

  bool erase(RowID id)
  {
    return m_records.erase(id) > 0;
  }

  [[nodiscard]]
  std::size_t size() const
  {
    return m_records.size();
  }

  [[nodiscard]]
  bool empty() const
  {
    return m_records.empty();
  }

  void clear()
  {
    m_records.clear();
    m_nextRowID = 1;
  }

private:
  std::unordered_map<RowID, Record> m_records;
  RowID m_nextRowID{1};
};

}


#pragma once

#include "BulkLoader.hpp"
#include "index/BTree.hpp"
#include "index/KeyExtractor.hpp"
#include "storage/RecordManager.hpp"

#include <cstddef>

namespace db::benchmark {

template <typename TupleRange>
BulkLoadResult loadIndexedTuples(const TupleRange& tuples,
                                 storage::RecordManager& records,
                                 index::BTree& index,
                                 std::size_t indexedColumn,
                                 BulkLoadOptions options = {}) {
    return bulkLoad(
        tuples,
        [&](const storage::Tuple& tuple) {
            const auto key = index::keyFromTuple(tuple, indexedColumn);
            if (index.contains(key)) {
                return false;
            }
            const auto rowId = records.insert(tuple);
            if (!index.insert(key, rowId)) {
                records.erase(rowId);
                return false;
            }
            return true;
        },
        [&] { return index.validate(); }, options);
}

}  // namespace db::benchmark

#pragma once

#include <cstdint>

namespace storage {

using RowID = std::uint64_t;
using PageID = std::uint32_t;
using SlotID = std::uint32_t;

constexpr RowID INVALID_ROW_ID = 0;
constexpr PageID INVALID_PAGE_ID = 0;

constexpr RowID makeRowID(PageID pageId, SlotID slotId)
{
  return (static_cast<RowID>(pageId) << 32U) |
    (static_cast<RowID>(slotId) + 1U);
}

constexpr PageID pageID(RowID rowId)
{
  return static_cast<PageID>(rowId >> 32U);
}

constexpr SlotID slotID(RowID rowId)
{
  return static_cast<SlotID>((rowId & 0xFFFFFFFFULL) - 1U);
}

constexpr bool isValidRowID(RowID rowId)
{
  return rowId != INVALID_ROW_ID && pageID(rowId) != INVALID_PAGE_ID &&
    (rowId & 0xFFFFFFFFULL) != 0;
}

}

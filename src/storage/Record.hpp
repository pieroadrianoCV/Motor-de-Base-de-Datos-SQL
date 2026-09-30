#pragma once

#include "RowID.hpp"
#include "Tuple.hpp"

namespace storage {

struct Record {
  RowID id{INVALID_ROW_ID};
  Tuple tuple;
};

}

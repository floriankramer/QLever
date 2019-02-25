// Copyright 2015, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)
#pragma once

#include <utility>
#include <vector>
#include "IdTable.h"

using std::pair;
using std::vector;

class OBComp {
 public:
  OBComp(const vector<pair<size_t, bool>>& sortIndices)
      : _sortIndices(sortIndices) {}

  template <int I>
  bool operator()(const typename IdTableImpl<I>::Row& a,
                  const typename IdTableImpl<I>::Row& b) const {
    for (auto& entry : _sortIndices) {
      if (a[entry.first] < b[entry.first]) {
        return !entry.second;
      }
      if (a[entry.first] > b[entry.first]) {
        return entry.second;
      }
    }
    return a[0] < b[0];
  }

 private:
  vector<pair<size_t, bool>> _sortIndices;
};

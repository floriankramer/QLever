// Copyright 2015, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#include <sstream>

#include "./Comparators.h"
#include "./OrderBy.h"
#include "./QueryExecutionTree.h"

using std::string;

// _____________________________________________________________________________
size_t OrderBy::getResultWidth() const { return _subtree->getResultWidth(); }

// _____________________________________________________________________________
OrderBy::OrderBy(QueryExecutionContext* qec,
                 std::shared_ptr<QueryExecutionTree> subtree,
                 const vector<pair<size_t, bool>>& sortIndices)
    : Operation(qec), _subtree(subtree), _sortIndices(sortIndices) {}

// _____________________________________________________________________________
string OrderBy::asString(size_t indent) const {
  std::ostringstream os;
  for (size_t i = 0; i < indent; ++i) {
    os << " ";
  }
  os << "ORDER_BY\n" << _subtree->asString(indent);
  os << " order on ";
  for (auto ind : _sortIndices) {
    os << (ind.second ? "desc(" : "asc(") << ind.first << ") ";
  }
  return os.str();
}

// _____________________________________________________________________________
vector<size_t> OrderBy::resultSortedOn() const {
  std::vector<size_t> sortedOn;
  sortedOn.reserve(_sortIndices.size());
  for (const pair<size_t, bool>& p : _sortIndices) {
    if (!p.second) {
      // Only ascending columns count as sorted.
      sortedOn.push_back(p.first);
    }
  }
  return sortedOn;
}

// _____________________________________________________________________________
void OrderBy::computeResult(ResultTable* result) {
  LOG(DEBUG) << "Gettign sub-result for OrderBy result computation..." << endl;
  AD_CHECK(_sortIndices.size() > 0);
  shared_ptr<const ResultTable> subRes = _subtree->getResult();

  std::string orderByVars = "";
  for (auto p : _subtree->getVariableColumnMap()) {
    for (auto oc : _sortIndices) {
      if (oc.first == p.second) {
        if (oc.second) {
          orderByVars += "DESC(" + p.first + ") ";
        } else {
          orderByVars += "ASC(" + p.first + ") ";
        }
      }
    }
  }
  RuntimeInformation& runtimeInfo = getRuntimeInfo();
  runtimeInfo.setDescriptor("OrderBy on " + orderByVars);
  runtimeInfo.addChild(_subtree->getRootOperation()->getRuntimeInfo());
  LOG(DEBUG) << "OrderBy result computation..." << endl;
  result->_nofColumns = subRes->_nofColumns;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.insert(result->_resultTypes.end(),
                              subRes->_resultTypes.begin(),
                              subRes->_resultTypes.end());
  result->_localVocab = subRes->_localVocab;
  result->_data.insert(result->_data.end(), subRes->_data.begin(),
                       subRes->_data.end());
  getEngine().sort(&result->_data, OBComp(_sortIndices));
  result->_sortedBy = resultSortedOn();
  result->finish();
  LOG(DEBUG) << "OrderBy result computation done." << endl;
}

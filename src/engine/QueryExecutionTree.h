// Copyright 2015, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "../util/Conversions.h"
#include "../util/HashSet.h"
#include "./Operation.h"
#include "./QueryExecutionContext.h"

using std::shared_ptr;
using std::string;

// A query execution tree.
// Processed bottom up, this gives an ordering to the operations
// needed to solve a query.
class QueryExecutionTree {
 public:
  explicit QueryExecutionTree(QueryExecutionContext* const qec);

  enum OperationType {
    UNDEFINED = 0,
    SCAN = 1,
    JOIN = 2,
    SORT = 3,
    ORDER_BY = 4,
    FILTER = 5,
    DISTINCT = 6,
    TEXT_FOR_CONTEXTS = 7,
    TEXT_WITHOUT_FILTER = 8,
    TEXT_WITH_FILTER = 9,
    TWO_COL_JOIN = 10,
    OPTIONAL_JOIN = 11,
    COUNT_AVAILABLE_PREDICATES = 12,
    GROUP_BY = 13,
    HAS_RELATION_SCAN = 14,
    UNION = 15,
    MULTICOLUMN_JOIN = 16,
    TRANSITIVE_PATH = 17,
    VALUES = 18
  };

  void setOperation(OperationType type, std::shared_ptr<Operation> op);

  string asString(size_t indent = 0);

  QueryExecutionContext* getQec() const { return _qec; }

  const ad_utility::HashMap<string, size_t>& getVariableColumns() const {
    return _variableColumnMap;
  }

  std::shared_ptr<Operation> getRootOperation() const { return _rootOperation; }

  const OperationType& getType() const { return _type; }

  bool isEmpty() const {
    return _type == OperationType::UNDEFINED || !_rootOperation;
  }

  void setVariableColumn(const string& var, size_t i);

  size_t getVariableColumn(const string& var) const;

  void setVariableColumns(const ad_utility::HashMap<string, size_t>& map);

  void setContextVars(const std::unordered_set<string>& set) {
    _contextVars = set;
  }

  const std::unordered_set<string>& getContextVars() const {
    return _contextVars;
  }

  size_t getResultWidth() const { return _rootOperation->getResultWidth(); }

  shared_ptr<const ResultTable> getResult() const {
    return _rootOperation->getResult(isRoot());
  }

  void writeResultToStream(std::ostream& out, const vector<string>& selectVars,
                           size_t limit = MAX_NOF_ROWS_IN_RESULT,
                           size_t offset = 0, char sep = '\t') const;

  nlohmann::json writeResultAsJson(const vector<string>& selectVars,
                                   size_t limit, size_t offset) const;

  const std::vector<size_t>& resultSortedOn() const {
    return _rootOperation->getResultSortedOn();
  }

  bool isContextvar(const string& var) const {
    return _contextVars.count(var) > 0;
  }

  void addContextVar(const string& var) { _contextVars.insert(var); }

  void setTextLimit(size_t limit) {
    _rootOperation->setTextLimit(limit);
    // Invalidate caches asString representation.
    _asString = "";  // triggers recomputation.
    _sizeEstimate = std::numeric_limits<size_t>::max();
  }

  size_t getCostEstimate();

  size_t getSizeEstimate();

  float getMultiplicity(size_t col) const {
    return _rootOperation->getMultiplicity(col);
  }

  size_t getDistinctEstimate(size_t col) const {
    return static_cast<size_t>(_rootOperation->getSizeEstimate() /
                               _rootOperation->getMultiplicity(col));
  }

  bool varCovered(string var) const;

  bool knownEmptyResult();

  // Try to find the result for this tree in the LRU cache
  // of our qec. If found, we store a shared ptr to pin it
  // and set the size estimate correctly and the cost estimate
  // to zero. Currently multiplicities are not affected
  void readFromCache();

  // recursively get all warnings from descendant operations
  vector<string> collectWarnings() const {
    return _rootOperation->collectWarnings();
  }

  template <typename F>
  void forAllDescendants(F f) {
    static_assert(
        std::is_same_v<void, std::invoke_result_t<F, QueryExecutionTree*>>);
    for (auto ptr : _rootOperation->getChildren()) {
      if (ptr) {
        f(ptr);
        ptr->forAllDescendants(f);
      }
    }
  }

  template <typename F>
  void forAllDescendants(F f) const {
    static_assert(
        std::is_same_v<void,
                       std::invoke_result_t<F, const QueryExecutionTree*>>);
    for (auto ptr : _rootOperation->getChildren()) {
      if (ptr) {
        f(ptr);
        ptr->forAllDescendants(f);
      }
    }
  }

  bool& isRoot() noexcept { return _isRoot; }
  [[nodiscard]] const bool& isRoot() const noexcept { return _isRoot; }

 private:
  QueryExecutionContext* _qec;  // No ownership
  ad_utility::HashMap<string, size_t> _variableColumnMap;
  std::shared_ptr<Operation>
      _rootOperation;  // Owned child. Will be deleted at deconstruction.
  OperationType _type;
  std::unordered_set<string> _contextVars;
  string _asString;
  size_t _indent = 0;  // the indent with which the _asString repr was formatted
  size_t _sizeEstimate;
  bool _isRoot = false;  // used to distinguish the root from child
                         // operations/subtrees when pinning only the result.

  std::shared_ptr<const ResultTable> _cachedResult = nullptr;

  /**
   * @brief Convert an IdTable (typically from a query result) to a json array
   * @param data the IdTable from which we read
   * @param from the first <from> entries of the idTable are skipped
   * @param limit at most <limit> entries are written, starting at <from>
   * @param validIndices each pair of <columnInIdTable, correspondingType> tells
   * us which columns are to be serialized in which order
   * @return a 2D-Json array corresponding to the IdTable given the arguments
   */
  nlohmann::json writeJsonTable(
      const IdTable& data, size_t from, size_t limit,
      const vector<std::optional<pair<size_t, ResultTable::ResultType>>>&
          validIndices) const;

  void writeTable(
      const IdTable& data, char sep, size_t from, size_t upperBound,
      const vector<std::optional<pair<size_t, ResultTable::ResultType>>>&
          validIndices,
      std::ostream& out) const;
};

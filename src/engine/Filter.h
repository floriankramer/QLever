// Copyright 2015, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)
#pragma once

#include <list>
#include <utility>
#include <vector>
#include "../parser/ParsedQuery.h"
#include "./Operation.h"
#include "./QueryExecutionTree.h"

using std::list;
using std::pair;
using std::vector;

class Filter : public Operation {
 public:
  virtual size_t getResultWidth() const;

 public:
  Filter(QueryExecutionContext* qec,
         std::shared_ptr<QueryExecutionTree> subtree,
         SparqlFilter::FilterType type, string lhs, string rhs);

  virtual string asString(size_t indent = 0) const override;

  virtual vector<size_t> resultSortedOn() const override {
    return _subtree->resultSortedOn();
  }

  virtual void setTextLimit(size_t limit) override {
    _subtree->setTextLimit(limit);
  }

  virtual size_t getSizeEstimate() override {
    if (_type == SparqlFilter::FilterType::REGEX) {
      // TODO(jbuerklin): return a better estimate
      return std::numeric_limits<Id>::max();
    }
    // TODO(schnelle): return a better estimate
    if (_rhs[0] == '?') {
      if (_type == SparqlFilter::FilterType::EQ) {
        return _subtree->getSizeEstimate() / 1000;
      }
      if (_type == SparqlFilter::FilterType::NE) {
        return _subtree->getSizeEstimate() / 4;
      } else {
        return _subtree->getSizeEstimate() / 2;
      }
    } else {
      if (_type == SparqlFilter::FilterType::EQ) {
        return _subtree->getSizeEstimate() / 1000;
      }
      if (_type == SparqlFilter::FilterType::NE) {
        return _subtree->getSizeEstimate();
      } else {
        return _subtree->getSizeEstimate() / 50;
      }
    }
  }

  virtual size_t getCostEstimate() override {
    if (_type == SparqlFilter::FilterType::REGEX) {
      return std::numeric_limits<Id>::max();
    }
    return getSizeEstimate() + _subtree->getSizeEstimate() +
           _subtree->getCostEstimate();
  }

  void setRegexIgnoreCase(bool i) { _regexIgnoreCase = i; }

  std::shared_ptr<QueryExecutionTree> getSubtree() const { return _subtree; };

  virtual bool knownEmptyResult() override {
    return _subtree->knownEmptyResult();
  }

  virtual float getMultiplicity(size_t col) override {
    return _subtree->getMultiplicity(col);
  }

 private:
  std::string getDescriptor() const;

  std::shared_ptr<QueryExecutionTree> _subtree;
  SparqlFilter::FilterType _type;
  string _lhs;
  string _rhs;
  bool _regexIgnoreCase;

  /**
   * @brief Uses the result type and the filter type (_type) to apply the filter
   * to subRes and store it in res.
   * @return The pointer res.
   */
  template <ResultTable::ResultType T, int WIDTH>
  void computeFilter(IdTableStatic<WIDTH>* dynResult, size_t lhs, size_t rhs,
                     const IdTableStatic<WIDTH>& dynInput) const;

  template <int WIDTH>
  void computeResultDynamicValue(IdTable* dynResult, size_t lhsInd,
                                 size_t rhsInd, const IdTable& dynInput,
                                 ResultTable::ResultType lhsType);

  /**
   * @brief Uses the result type and the filter type (_type) to apply the filter
   * to subRes and store it in res.
   * @return The pointer res.
   */
  template <ResultTable::ResultType T, int WIDTH>
  void computeFilterFixedValue(IdTableStatic<WIDTH>* res, size_t lhs, Id rhs,
                               const IdTableStatic<WIDTH>& input,
                               shared_ptr<const ResultTable> subRes) const;

  template <int WIDTH>
  void computeResultFixedValue(
      ResultTable* result,
      const std::shared_ptr<const ResultTable> subRes) const;
  virtual void computeResult(ResultTable* result) override;

  /**
   * @brief This struct handles the extraction of the data from an id based upon
   *        the result type of the id's column.
   */
  template <ResultTable::ResultType T>
  struct ValueReader {
    static Id get(size_t in) { return in; }
  };
};

template <>
struct Filter::ValueReader<ResultTable::ResultType::FLOAT> {
  static float get(size_t in) {
    float f;
    std::memcpy(&f, &in, sizeof(float));
    return f;
  }
};

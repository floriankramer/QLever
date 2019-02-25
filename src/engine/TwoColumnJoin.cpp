// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#include "TwoColumnJoin.h"
#include "CallFixedSize.h"

using std::string;

// _____________________________________________________________________________
TwoColumnJoin::TwoColumnJoin(QueryExecutionContext* qec,
                             std::shared_ptr<QueryExecutionTree> t1,
                             std::shared_ptr<QueryExecutionTree> t2,
                             const vector<array<Id, 2>>& jcs)
    : Operation(qec) {
  // Make sure subtrees are ordered so that identical queries can be identified.
  AD_CHECK_EQ(jcs.size(), 2);
  if (t1->asString() < t2->asString()) {
    _left = t1;
    _jc1Left = jcs[0][0];
    _jc2Left = jcs[1][0];
    _right = t2;
    _jc1Right = jcs[0][1];
    _jc2Right = jcs[1][1];
  } else {
    _left = t2;
    _jc1Left = jcs[0][1];
    _jc2Left = jcs[1][1];
    _right = t1;
    _jc1Right = jcs[0][0];
    _jc2Right = jcs[1][0];
  }
  // Now check if one of them is an index scan with width two.
  // If so, make sure that its first jc is 0 and the second 1.
  if (_left->getType() == QueryExecutionTree::SCAN &&
      _left->getResultWidth() == 2) {
    if (_jc1Left > _jc2Left) {
      std::swap(_jc1Left, _jc2Left);
      std::swap(_jc1Right, _jc2Right);
    }
  } else if (_right->getType() == QueryExecutionTree::SCAN &&
             _right->getResultWidth() == 2) {
    if (_jc1Right > _jc2Right) {
      std::swap(_jc1Left, _jc2Left);
      std::swap(_jc1Right, _jc2Right);
    }
  }
}

// _____________________________________________________________________________
string TwoColumnJoin::asString(size_t indent) const {
  std::ostringstream os;
  for (size_t i = 0; i < indent; ++i) {
    os << " ";
  }
  os << "TWO_COLUMN_JOIN\n";
  for (size_t i = 0; i < indent; ++i) {
    os << " ";
  }
  os << _left->asString(indent) << "\n";
  for (size_t i = 0; i < indent; ++i) {
    os << " ";
  }
  os << "join-columns: [" << _jc1Left << " & " << _jc2Left << "]\n";
  for (size_t i = 0; i < indent; ++i) {
    os << " ";
  }
  os << "|X|\n";
  for (size_t i = 0; i < indent; ++i) {
    os << " ";
  }
  os << _right->asString(indent) << "\n";
  for (size_t i = 0; i < indent; ++i) {
    os << " ";
  }
  os << "join-columns: [" << _jc1Right << " & " << _jc2Right << "]";
  return os.str();
}

// _____________________________________________________________________________
void TwoColumnJoin::computeResult(ResultTable* result) {
  AD_CHECK(result);
  LOG(DEBUG) << "TwoColumnJoin result computation..." << endl;

  RuntimeInformation& runtimeInfo = getRuntimeInfo();
  std::string joinVars = "";
  for (auto p : _left->getVariableColumnMap()) {
    if (p.second == _jc1Left || p.second == _jc2Left) {
      joinVars += p.first + " ";
    }
  }
  runtimeInfo.setDescriptor("TwoColumnJoin on " + joinVars);

  // Deal with the case that one of the lists is width two and
  // with join columns 0 1. This means we can use the filter method.
  if ((_left->getResultWidth() == 2 && _jc1Left == 0 && _jc2Left == 1) ||
      (_right->getResultWidth() == 2 && _jc1Right == 0 && _jc2Right == 1)) {
    bool rightFilter =
        (_right->getResultWidth() == 2 && _jc1Right == 0 && _jc2Right == 1);
    const auto& v = rightFilter ? _left : _right;
    const auto leftResult = _left->getResult();
    const auto rightResult = _right->getResult();
    runtimeInfo.addChild(_left->getRootOperation()->getRuntimeInfo());
    runtimeInfo.addChild(_right->getRootOperation()->getRuntimeInfo());
    const IdTable& filter =
        rightFilter ? rightResult->_data : leftResult->_data;
    size_t jc1 = rightFilter ? _jc1Left : _jc1Right;
    size_t jc2 = rightFilter ? _jc2Left : _jc2Right;
    result->_sortedBy = {jc1};
    result->_nofColumns = v->getResultWidth();
    result->_data.setCols(result->_nofColumns);
    result->_resultTypes.reserve(result->_nofColumns);
    result->_resultTypes.insert(result->_resultTypes.end(),
                                leftResult->_resultTypes.begin(),
                                leftResult->_resultTypes.end());
    for (size_t col = 0; col < rightResult->_nofColumns; col++) {
      if (col != _jc1Right && col != _jc2Right) {
        result->_resultTypes.push_back(rightResult->_resultTypes[col]);
      }
    }

    AD_CHECK_GE(result->_nofColumns, 2);

    const auto& toFilter = rightFilter ? leftResult : rightResult;

    int inWidth = toFilter->_data.cols();
    int filterWidth = filter.cols();
    CALL_FIXED_SIZE_2(inWidth, filterWidth, getEngine().filter, toFilter->_data,
                      jc1, jc2, filter, &result->_data);

    result->finish();
    LOG(DEBUG) << "TwoColumnJoin result computation done." << endl;
    return;
  }
  // TOOD: implement the other case later
  AD_THROW(ad_semsearch::Exception::NOT_YET_IMPLEMENTED,
           "For now, prefer cyclic queries to be resolved using a single join.")
}

// _____________________________________________________________________________
ad_utility::HashMap<string, size_t> TwoColumnJoin::getVariableColumns() const {
  ad_utility::HashMap<string, size_t> retVal(_left->getVariableColumnMap());
  size_t leftSize = _left->getResultWidth();
  for (auto it = _right->getVariableColumnMap().begin();
       it != _right->getVariableColumnMap().end(); ++it) {
    if (it->second < _jc1Right) {
      if (it->second < _jc2Right) {
        retVal[it->first] = leftSize + it->second;
      } else if (it->second > _jc2Right) {
        retVal[it->first] = leftSize + it->second - 1;
      }
    }
    if (it->second > _jc1Right) {
      if (it->second < _jc2Right) {
        retVal[it->first] = leftSize + it->second - 1;
      } else if (it->second > _jc2Right) {
        retVal[it->first] = leftSize + it->second - 2;
      }
    }
  }
  return retVal;
}

// _____________________________________________________________________________
size_t TwoColumnJoin::getResultWidth() const {
  size_t res = _left->getResultWidth() + _right->getResultWidth() - 2;
  AD_CHECK(res > 0);
  return res;
}

// _____________________________________________________________________________
vector<size_t> TwoColumnJoin::resultSortedOn() const {
  return {_jc1Left, _jc2Left};
}

// _____________________________________________________________________________
float TwoColumnJoin::getMultiplicity(size_t col) {
  if (_multiplicities.size() == 0) {
    computeMultiplicities();
  }
  AD_CHECK_LT(col, _multiplicities.size());
  return _multiplicities[col];
}

// _____________________________________________________________________________
void TwoColumnJoin::computeMultiplicities() {
  // As currently implemented, one filters the other.
  // Thus, we just take the min multiplicity for each pair join columns.
  // The others are left untouched.
  // The filtering may lower the result size (and the number of distincts)
  // but multiplicities shouldn't be affected.
  for (size_t i = 0; i < _left->getResultWidth(); ++i) {
    if (i == _jc1Left) {
      _multiplicities.push_back(std::min(_left->getMultiplicity(i),
                                         _right->getMultiplicity(_jc1Right)));
    } else if (i == _jc2Left) {
      _multiplicities.push_back(std::min(_left->getMultiplicity(i),
                                         _right->getMultiplicity(_jc2Right)));
    } else {
      _multiplicities.push_back(_left->getMultiplicity(i));
    }
  }

  for (size_t i = 0; i < _right->getResultWidth(); ++i) {
    if (i != _jc1Right && i != _jc2Right) {
      _multiplicities.push_back(_right->getMultiplicity(i));
    }
  }
  assert(_multiplicities.size() == getResultWidth());
}

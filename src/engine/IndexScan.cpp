// Copyright 2015, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#include "./IndexScan.h"
#include <sstream>
#include <string>

using std::string;

// _____________________________________________________________________________
string IndexScan::asString(size_t indent) const {
  std::ostringstream os;
  for (size_t i = 0; i < indent; ++i) {
    os << ' ';
  }
  switch (_type) {
    case PSO_BOUND_S:
      os << "SCAN PSO with P = \"" << _predicate << "\", S = \"" << _subject
         << "\"";
      break;
    case POS_BOUND_O:
      os << "SCAN POS with P = \"" << _predicate << "\", O = \"" << _object
         << "\"";
      break;
    case SOP_BOUND_O:
      os << "SCAN SOP with S = \"" << _subject << "\", O = \"" << _object
         << "\"";
      break;
    case PSO_FREE_S:
      os << "SCAN PSO with P = \"" << _predicate << "\"";
      break;
    case POS_FREE_O:
      os << "SCAN POS with P = \"" << _predicate << "\"";
      break;
    case SPO_FREE_P:
      os << "SCAN SPO with S = \"" << _subject << "\"";
      break;
    case SOP_FREE_O:
      os << "SCAN SOP with S = \"" << _subject << "\"";
      break;
    case OPS_FREE_P:
      os << "SCAN OPS with O = \"" << _object << "\"";
      break;
    case OSP_FREE_S:
      os << "SCAN OSP with O = \"" << _object << "\"";
      break;
    case FULL_INDEX_SCAN_SPO:
      os << "SCAN FOR FULL INDEX SPO (DUMMY OPERATION)";
      break;
    case FULL_INDEX_SCAN_SOP:
      os << "SCAN FOR FULL INDEX SOP (DUMMY OPERATION)";
      break;
    case FULL_INDEX_SCAN_PSO:
      os << "SCAN FOR FULL INDEX PSO (DUMMY OPERATION)";
      break;
    case FULL_INDEX_SCAN_POS:
      os << "SCAN FOR FULL INDEX POS (DUMMY OPERATION)";
      break;
    case FULL_INDEX_SCAN_OSP:
      os << "SCAN FOR FULL INDEX OSP (DUMMY OPERATION)";
      break;
    case FULL_INDEX_SCAN_OPS:
      os << "SCAN FOR FULL INDEX OPS (DUMMY OPERATION)";
      break;
  }
  return os.str();
}

// _____________________________________________________________________________
size_t IndexScan::getResultWidth() const {
  switch (_type) {
    case PSO_BOUND_S:
    case POS_BOUND_O:
    case SOP_BOUND_O:
      return 1;
    case PSO_FREE_S:
    case POS_FREE_O:
    case SPO_FREE_P:
    case SOP_FREE_O:
    case OSP_FREE_S:
    case OPS_FREE_P:
      return 2;
    case FULL_INDEX_SCAN_SPO:
    case FULL_INDEX_SCAN_SOP:
    case FULL_INDEX_SCAN_PSO:
    case FULL_INDEX_SCAN_POS:
    case FULL_INDEX_SCAN_OSP:
    case FULL_INDEX_SCAN_OPS:
      return 3;
    default:
      AD_THROW(ad_semsearch::Exception::CHECK_FAILED, "Should be unreachable.");
  }
}

// _____________________________________________________________________________
vector<size_t> IndexScan::resultSortedOn() const {
  switch (_type) {
    case PSO_BOUND_S:
    case POS_BOUND_O:
    case SOP_BOUND_O:
      return {0};
    case PSO_FREE_S:
    case POS_FREE_O:
    case SPO_FREE_P:
    case SOP_FREE_O:
    case OSP_FREE_S:
    case OPS_FREE_P:
      return {0, 1};
    case FULL_INDEX_SCAN_SPO:
    case FULL_INDEX_SCAN_SOP:
    case FULL_INDEX_SCAN_PSO:
    case FULL_INDEX_SCAN_POS:
    case FULL_INDEX_SCAN_OSP:
    case FULL_INDEX_SCAN_OPS:
      return {0, 1, 2};
    default:
      AD_THROW(ad_semsearch::Exception::CHECK_FAILED, "Should be unreachable.");
  }
}

// _____________________________________________________________________________
void IndexScan::computeResult(ResultTable* result) {
  LOG(DEBUG) << "IndexScan result computation...\n";

  RuntimeInformation& runtimeInfo = getRuntimeInfo();
  runtimeInfo.setDescriptor("IndexScan " + _subject + " " + _predicate + " " +
                            _object);
  switch (_type) {
    case PSO_BOUND_S:
      computePSOboundS(result);
      break;
    case POS_BOUND_O:
      computePOSboundO(result);
      break;
    case PSO_FREE_S:
      computePSOfreeS(result);
      break;
    case POS_FREE_O:
      computePOSfreeO(result);
      break;
    case SOP_BOUND_O:
      computeSOPboundO(result);
      break;
    case SPO_FREE_P:
      computeSPOfreeP(result);
      break;
    case SOP_FREE_O:
      computeSOPfreeO(result);
      break;
    case OSP_FREE_S:
      computeOSPfreeS(result);
      break;
    case OPS_FREE_P:
      computeOPSfreeP(result);
      break;
    case FULL_INDEX_SCAN_SPO:
    case FULL_INDEX_SCAN_SOP:
    case FULL_INDEX_SCAN_PSO:
    case FULL_INDEX_SCAN_POS:
    case FULL_INDEX_SCAN_OSP:
    case FULL_INDEX_SCAN_OPS:
      AD_THROW(ad_semsearch::Exception::CHECK_FAILED,
               "Asked to execute a scan for the full index. "
               "This should never happen.");
  }
  LOG(DEBUG) << "IndexScan result computation done.\n";
}

// _____________________________________________________________________________
void IndexScan::computePSOboundS(ResultTable* result) const {
  result->_nofColumns = 1;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_sortedBy = {0};

  _executionContext->getIndex().scanPSO(_predicate, _subject, &result->_data);
  result->finish();
}

// _____________________________________________________________________________
void IndexScan::computePSOfreeS(ResultTable* result) const {
  result->_nofColumns = 2;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_sortedBy = {0, 1};
  _executionContext->getIndex().scanPSO(_predicate, &result->_data);
  result->finish();
}

// _____________________________________________________________________________
void IndexScan::computePOSboundO(ResultTable* result) const {
  result->_nofColumns = 1;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_sortedBy = {0};
  _executionContext->getIndex().scanPOS(_predicate, _object, &result->_data);
  result->finish();
}

// _____________________________________________________________________________
void IndexScan::computePOSfreeO(ResultTable* result) const {
  result->_nofColumns = 2;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_sortedBy = {0, 1};
  _executionContext->getIndex().scanPOS(_predicate, &result->_data);
  result->finish();
}

// _____________________________________________________________________________
size_t IndexScan::computeSizeEstimate() {
  if (_executionContext) {
    // Should always be in this branch. Else is only for test cases.

    // We have to do a simple scan anyway so might as well do it now
    if (getResultWidth() == 1) {
      RuntimeInformation& runtimeInfo = getRuntimeInfo();
      size_t size = getResult()->size();
      // When a cached result is loaded but the runtimeInfo for that
      // Operation object was already computed the old values are
      // used (instead of the time for the cache access. This breaks
      // the output of the operation time for any operation that has
      // a single column scan as its child, as that operation itself
      // would measure the time of the cache access for the result
      // of that scan, while the scan itself would report the time
      // required for the actual scan that is done in here.
      runtimeInfo.setTime(0);
      return size;
    }
    if (_type == SPO_FREE_P || _type == SOP_FREE_O) {
      return getIndex().sizeEstimate(_subject, "", "");
    } else if (_type == POS_FREE_O || _type == PSO_FREE_S) {
      return getIndex().sizeEstimate("", _predicate, "");
    } else if (_type == OPS_FREE_P || _type == OSP_FREE_S) {
      return getIndex().sizeEstimate("", "", _object);
    }
    return getIndex().sizeEstimate("", "", "");
  } else {
    return 1000 + _subject.size() + _predicate.size() + _object.size();
  }
}

// _____________________________________________________________________________
void IndexScan::computeSPOfreeP(ResultTable* result) const {
  result->_nofColumns = 2;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_sortedBy = {0, 1};
  _executionContext->getIndex().scanSPO(_subject, &result->_data);
  result->finish();
}

// _____________________________________________________________________________
void IndexScan::computeSOPboundO(ResultTable* result) const {
  result->_nofColumns = 1;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_sortedBy = {0};
  _executionContext->getIndex().scanSOP(_subject, _object, &result->_data);
  result->finish();
}

// _____________________________________________________________________________
void IndexScan::computeSOPfreeO(ResultTable* result) const {
  result->_nofColumns = 2;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_sortedBy = {0, 1};
  _executionContext->getIndex().scanSOP(_subject, &result->_data);
  result->finish();
}

// _____________________________________________________________________________
void IndexScan::computeOPSfreeP(ResultTable* result) const {
  result->_nofColumns = 2;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_sortedBy = {0, 1};
  _executionContext->getIndex().scanOPS(_object, &result->_data);
  result->finish();
}

// _____________________________________________________________________________
void IndexScan::computeOSPfreeS(ResultTable* result) const {
  result->_nofColumns = 2;
  result->_data.setCols(result->_nofColumns);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_resultTypes.push_back(ResultTable::ResultType::KB);
  result->_sortedBy = {0, 1};
  _executionContext->getIndex().scanOSP(_object, &result->_data);
  result->finish();
}

// _____________________________________________________________________________
void IndexScan::determineMultiplicities() {
  _multiplicity.clear();
  if (_executionContext) {
    if (getResultWidth() == 1) {
      _multiplicity.emplace_back(1);
    } else {
      switch (_type) {
        case PSO_FREE_S:
          _multiplicity = getIndex().getPSOMultiplicities(_predicate);
          break;
        case POS_FREE_O:
          _multiplicity = getIndex().getPOSMultiplicities(_predicate);
          break;
        case SPO_FREE_P:
          _multiplicity = getIndex().getSPOMultiplicities(_subject);
          break;
        case SOP_FREE_O:
          _multiplicity = getIndex().getSOPMultiplicities(_subject);
          break;
        case OSP_FREE_S:
          _multiplicity = getIndex().getOSPMultiplicities(_object);
          break;
        case OPS_FREE_P:
          _multiplicity = getIndex().getOPSMultiplicities(_object);
          break;
        case FULL_INDEX_SCAN_SPO:
          _multiplicity = getIndex().getSPOMultiplicities();
          break;
        case FULL_INDEX_SCAN_SOP:
          _multiplicity = getIndex().getSOPMultiplicities();
          break;
        case FULL_INDEX_SCAN_PSO:
          _multiplicity = getIndex().getPSOMultiplicities();
          break;
        case FULL_INDEX_SCAN_POS:
          _multiplicity = getIndex().getPOSMultiplicities();
          break;
        case FULL_INDEX_SCAN_OSP:
          _multiplicity = getIndex().getOSPMultiplicities();
          break;
        case FULL_INDEX_SCAN_OPS:
          _multiplicity = getIndex().getOPSMultiplicities();
          break;
        default:
          AD_THROW(ad_semsearch::Exception::ASSERT_FAILED,
                   "Switch reached default block unexpectedly!");
      }
    }
  } else {
    _multiplicity.emplace_back(1);
    _multiplicity.emplace_back(1);
    if (getResultWidth() == 3) {
      _multiplicity.emplace_back(1);
    }
  }
  assert(_multiplicity.size() >= 1 || _multiplicity.size() <= 3);
}

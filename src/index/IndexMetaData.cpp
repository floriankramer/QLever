// Copyright 2015, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#include <stdio.h>
#include <cmath>
#include <algorithm>
#include "./IndexMetaData.h"
#include "../util/ReadableNumberFact.h"
#include "../global/Constants.h"


// _____________________________________________________________________________
void IndexMetaData::add(const FullRelationMetaData& rmd,
                        const BlockBasedRelationMetaData& bRmd) {
  _data[rmd._relId] = rmd;
  off_t afterExpected = rmd.hasBlocks() ? bRmd._offsetAfter :
                        static_cast<off_t>(rmd._startFullIndex +
                                           rmd.getNofBytesForFulltextIndex());
  if (rmd.hasBlocks()) {
    _blockData[rmd._relId] = bRmd;
  }
  if (afterExpected > _offsetAfter) { _offsetAfter = afterExpected; }
}


// _____________________________________________________________________________
off_t IndexMetaData::getOffsetAfter() const {
  return _offsetAfter;
}

// _____________________________________________________________________________
void IndexMetaData::createFromByteBufferWithPreload(unsigned char* buf) {
  size_t nameLength = *reinterpret_cast<size_t*>(buf);
  size_t nofBytesDone = sizeof(size_t);
  _name.assign(reinterpret_cast<char*>(buf + nofBytesDone), nameLength);
  nofBytesDone += nameLength;
  size_t nofRelations = *reinterpret_cast<size_t*>(buf + nofBytesDone);
  nofBytesDone += sizeof(size_t);
  _offsetAfter = *reinterpret_cast<off_t*>(buf + nofBytesDone);
  nofBytesDone += sizeof(off_t);
  _nofTriples = 0;
  for (size_t i = 0; i < nofRelations; ++i) {
    FullRelationMetaData rmd;
    rmd.createFromByteBuffer(buf + nofBytesDone);
    _nofTriples += rmd.getNofElements();
    nofBytesDone += rmd.bytesRequired();
    if (rmd.hasBlocks()) {
      BlockBasedRelationMetaData bRmd;
      bRmd.createFromByteBuffer(buf + nofBytesDone);
      nofBytesDone += bRmd.bytesRequired();
      add(rmd, bRmd);
    } else {
      add(rmd, BlockBasedRelationMetaData());
    }
  }
  _preloaded = true;
}

// _____________________________________________________________________________
void IndexMetaData::createWithoutPreload(ad_utility::File* indexFile,
                                         off_t startMeta,
                                         off_t startRelIdToOffset,
                                         off_t endMeta) {
  size_t bufSize = std::min(MAX_NAME_SIZE + 20 * sizeof(size_t),
                            static_cast<size_t>(endMeta - startMeta));
  auto* buf = new unsigned char[bufSize];
  _indexFile = indexFile;
  _startMeta = startMeta;
  _startRelIdToOffset = startRelIdToOffset;
  _endMeta = endMeta;

  indexFile->read(buf, bufSize, startMeta);
  size_t nameLength = *reinterpret_cast<size_t*>(buf);
  size_t nofBytesDone = sizeof(size_t);
  _name.assign(reinterpret_cast<char*>(buf + nofBytesDone), nameLength);
  nofBytesDone += nameLength;
  size_t nofRelations = *reinterpret_cast<size_t*>(buf + nofBytesDone);
  nofBytesDone += sizeof(size_t);
  _offsetAfter = *reinterpret_cast<off_t*>(buf + nofBytesDone);
  nofBytesDone += sizeof(off_t);

  _nofTriples = 0;

  delete[] buf;
}

// _____________________________________________________________________________
bool IndexMetaData::loadAndAddRelationMetaData(Id relId) {
  std::pair<Id, off_t> p, next;
  binarySearchIndexFile(relId, p, next);
  if (p.first != relId) {
    return false;
  } else {
    off_t excluding = (p.first == next.first ? _startRelIdToOffset
                                             : next.second);
    off_t bufSize = excluding - p.second;
    auto* buf = new unsigned char[bufSize];
    _indexFile->read(buf, bufSize, p.second);
    FullRelationMetaData rmd;
    size_t nofBytesDone = 0;
    rmd.createFromByteBuffer(buf);
    nofBytesDone += rmd.bytesRequired();
    if (rmd.hasBlocks()) {
      BlockBasedRelationMetaData bRmd;
      bRmd.createFromByteBuffer(buf + nofBytesDone);
      nofBytesDone += bRmd.bytesRequired();
      add(rmd, bRmd);
    } else {
      add(rmd, BlockBasedRelationMetaData());
    }
  }
}


// _____________________________________________________________________________
const RelationMetaData IndexMetaData::getRmd(Id relId) const {
  // relationExists always should have been called before.
  auto it = _data.find(relId);
  if (it == _data.end()) {
    AD_THROW(ad_semsearch::Exception::ASSERT_FAILED,
             "Should have called relationExists() before!")
  }
  RelationMetaData ret(it->second);
  if (it->second.hasBlocks()) {
    ret._rmdBlocks = &_blockData.find(it->first)->second;
  }
  return ret;
}

// _____________________________________________________________________________
bool IndexMetaData::relationExists(Id relId) {
  if (_preloaded) {
    return _data.count(relId) > 0;
  } else {
    if (_data.count(relId) > 0) { return true; }
    return loadAndAddRelationMetaData(relId);
  }
}

// _____________________________________________________________________________
ad_utility::File& operator<<(ad_utility::File& f, const IndexMetaData& imd) {
  size_t nameLength = imd._name.size();
  f.write(&nameLength, sizeof(nameLength));
  f.write(imd._name.data(), nameLength);
  size_t nofElements = imd._data.size();
  f.write(&nofElements, sizeof(nofElements));
  f.write(&imd._offsetAfter, sizeof(imd._offsetAfter));
  for (auto it = imd._data.begin(); it != imd._data.end(); ++it) {
    f << it->second;
    if (it->second.hasBlocks()) {
      auto itt = imd._blockData.find(it->second._relId);
      AD_CHECK(itt != imd._blockData.end());
      f << itt->second;
    }
  }
  return f;
}

// _____________________________________________________________________________
string IndexMetaData::statistics() const {
  std::ostringstream os;
  std::locale loc;
  ad_utility::ReadableNumberFacet facet(1);
  std::locale locWithNumberGrouping(loc, &facet);
  os.imbue(locWithNumberGrouping);
  os << '\n';
  os
      << "-------------------------------------------------------------------\n";
  os << "----------------------------------\n";
  os << "Index Statistics:\n";
  os << "----------------------------------\n\n";
  os << "# Relations: " << _data.size() << '\n';
  size_t totalElements = 0;
  size_t totalBytes = 0;
  size_t totalBlocks = 0;
  for (auto it = _data.begin(); it != _data.end(); ++it) {
    totalElements += it->second.getNofElements();
    totalBytes += getTotalBytesForRelation(it->second);
    totalBlocks += getNofBlocksForRelation(it->first);
  }
  size_t totalPairIndexBytes = totalElements * 2 * sizeof(Id);
  os << "# Elements:  " << totalElements << '\n';
  os << "# Blocks:    " << totalBlocks << "\n\n";
  os << "Theoretical size of Id triples: "
     << totalElements * 3 * sizeof(Id) << " bytes \n";
  os << "Size of pair index:             "
     << totalPairIndexBytes << " bytes \n";
  os << "Total Size:                     " << totalBytes << " bytes \n";
  os
      << "-------------------------------------------------------------------\n";
  return os.str();
}


// _____________________________________________________________________________
size_t IndexMetaData::getNofBlocksForRelation(const Id id) const {
  auto it = _blockData.find(id);
  if (it != _blockData.end()) {
    return it->second._blocks.size();
  } else {
    return 0;
  }
}

// _____________________________________________________________________________
size_t IndexMetaData::getTotalBytesForRelation(
    const FullRelationMetaData& frmd) const {
  auto it = _blockData.find(frmd._relId);
  if (it != _blockData.end()) {
    return static_cast<size_t>(it->second._offsetAfter -
                               frmd._startFullIndex);
  } else {
    return frmd.getNofBytesForFulltextIndex();
  }
}

// _____________________________________________________________________________
size_t IndexMetaData::getNofDistinctC1() const {
  return _data.size();
}

// _____________________________________________________________________________
void IndexMetaData::binarySearchIndexFile(const Id relId, pair<Id, off_t>& res,
                                          pair<Id, off_t>& follower) {
  size_t elemSize = sizeof(Id) + sizeof(off_t);
  auto* buf = new unsigned char[elemSize];
  auto beg = static_cast<int>(_startRelIdToOffset);
  auto end = static_cast<int>(_endMeta);

  while (beg <= end) {
    auto middle = static_cast<int>((end - beg) * elemSize);
    _indexFile->seek(middle, beg);
    _indexFile->read(buf, elemSize);
    auto id = *static_cast<Id*>(buf);
    if (id == relId) {
      res.first = id;
      auto off = *static_cast<off_t*>(buf + sizeof(size_t));
      res.second = off;
      if (middle + 2 * elemSize > static_cast<int>(_endMeta)) {
        follower = res;
      } else {
        _indexFile->seek(middle + elemSize, middle);
        _indexFile->read(buf, elemSize);
        follower.first = id;
        off = *static_cast<off_t*>(buf + sizeof(size_t));
        follower.second = off;
      }
      delete[] buf;
      return;
    } else if (relId > id) {
      beg = static_cast<int>(middle + elemSize);
    } else {
      end = static_cast<int>(middle - elemSize);
    }
  }
  // Case: not found.
  res.first = std::numeric_limits<Id>::max();
  delete[] buf;
}


// _____________________________________________________________________________
FullRelationMetaData::FullRelationMetaData() :
    _relId(0), _startFullIndex(0), _typeMultAndNofElements(0) {
}

// _____________________________________________________________________________
FullRelationMetaData::FullRelationMetaData(Id
                                           relId, off_t
                                           startFullIndex,
                                           size_t
                                           nofElements,
                                           double
                                           col1Mult, double
                                           col2Mult,
                                           bool
                                           isFunctional, bool
                                           hasBlocks) :
    _relId(relId), _startFullIndex(startFullIndex),
    _typeMultAndNofElements(nofElements) {
  assert(col1Mult >= 1);
  assert(col2Mult >= 1);
  double c1ml = log2(col1Mult);
  double c2ml = log2(col2Mult);
  uint8_t c1 = c1ml > 255 ? uint8_t(255) : uint8_t(c1ml);
  uint8_t c2 = c2ml > 255 ? uint8_t(255) : uint8_t(c2ml);
  setIsFunctional(isFunctional);
  setHasBlocks(hasBlocks);
  setCol1LogMultiplicity(c1);
  setCol2LogMultiplicity(c2);
}

// _____________________________________________________________________________
size_t FullRelationMetaData::getNofBytesForFulltextIndex() const {
  return getNofElements() * 2 * sizeof(Id);
}

// _____________________________________________________________________________
bool FullRelationMetaData::isFunctional() const {
  return (_typeMultAndNofElements & IS_FUNCTIONAL_MASK) != 0;
}

// _____________________________________________________________________________
bool FullRelationMetaData::hasBlocks() const {
  return (_typeMultAndNofElements & HAS_BLOCKS_MASK) != 0;
}

// _____________________________________________________________________________
size_t FullRelationMetaData::getNofElements() const {
  return size_t(_typeMultAndNofElements & NOF_ELEMENTS_MASK);
}


// _____________________________________________________________________________
void FullRelationMetaData::setIsFunctional(bool isFunctional) {
  if (isFunctional) {
    _typeMultAndNofElements |= IS_FUNCTIONAL_MASK;
  } else {
    _typeMultAndNofElements &= ~IS_FUNCTIONAL_MASK;
  }
}

// _____________________________________________________________________________
void FullRelationMetaData::setHasBlocks(bool hasBlocks) {
  if (hasBlocks) {
    _typeMultAndNofElements |= HAS_BLOCKS_MASK;
  } else {
    _typeMultAndNofElements &= ~HAS_BLOCKS_MASK;
  }
}

// _____________________________________________________________________________
void FullRelationMetaData::setCol1LogMultiplicity(uint8_t mult) {
  // Reset a current value
  _typeMultAndNofElements &= 0xFF00FFFFFFFFFFFF;
  // Set the new one
  _typeMultAndNofElements |= (uint64_t(mult) << 48);
}

// _____________________________________________________________________________
void FullRelationMetaData::setCol2LogMultiplicity(uint8_t mult) {
  // Reset a current value
  _typeMultAndNofElements &= 0xFFFF00FFFFFFFFFF;
  // Set the new one
  _typeMultAndNofElements |= (uint64_t(mult) << 40);
}

// _____________________________________________________________________________
uint8_t FullRelationMetaData::getCol1LogMultiplicity() const {
  return uint8_t((_typeMultAndNofElements & 0x00FF000000000000) >> 48);
}

// _____________________________________________________________________________
uint8_t FullRelationMetaData::getCol2LogMultiplicity() const {
  return uint8_t((_typeMultAndNofElements & 0x0000FF0000000000) >> 40);
}


// _____________________________________________________________________________
pair<off_t, size_t>
BlockBasedRelationMetaData::getBlockStartAndNofBytesForLhs(
    Id lhs) const {

  auto it = std::lower_bound(_blocks.begin(), _blocks.end(), lhs,
                             [](const BlockMetaData& a, Id lhs) {
                               return a._firstLhs < lhs;
                             });

  // Go back one block unless perfect lhs match.
  if (it == _blocks.end() || it->_firstLhs > lhs) {
    AD_CHECK(it != _blocks.begin());
    it--;
  }

  off_t after;
  if ((it + 1) != _blocks.end()) {
    after = (it + 1)->_startOffset;
  } else {
    after = _startRhs;
  }

  return pair<off_t, size_t>(it->_startOffset, after - it->_startOffset);
}

// _____________________________________________________________________________
pair<off_t, size_t> BlockBasedRelationMetaData::getFollowBlockForLhs(
    Id lhs) const {

  auto it = std::lower_bound(_blocks.begin(), _blocks.end(), lhs,
                             [](const BlockMetaData& a, Id lhs) {
                               return a._firstLhs < lhs;
                             });

  // Go back one block unless perfect lhs match.
  if (it == _blocks.end() || it->_firstLhs > lhs) {
    AD_CHECK(it != _blocks.begin());
    it--;
  }

  // Advance one block again is possible
  if ((it + 1) != _blocks.end()) {
    ++it;
  }

  off_t after;
  if ((it + 1) != _blocks.end()) {
    after = (it + 1)->_startOffset;
  } else {
    // In this case after is the beginning of the rhs list,
    after = _startRhs;
  }

  return pair<off_t, size_t>(it->_startOffset, after - it->_startOffset);
}

// _____________________________________________________________________________
FullRelationMetaData& FullRelationMetaData::createFromByteBuffer(
    unsigned char* buffer) {

  _relId = *reinterpret_cast<Id*>(buffer);
  _startFullIndex = *reinterpret_cast<off_t*>(buffer + sizeof(_relId));
  _typeMultAndNofElements = *reinterpret_cast<uint64_t*>(
      buffer + sizeof(Id) + sizeof(off_t));
  return *this;
}

// _____________________________________________________________________________
BlockBasedRelationMetaData& BlockBasedRelationMetaData::createFromByteBuffer(
    unsigned char* buffer) {
  _startRhs = *reinterpret_cast<off_t*>(buffer);
  _offsetAfter = *reinterpret_cast<off_t*>(buffer + sizeof(_startRhs));
  size_t nofBlocks = *reinterpret_cast<size_t*>(buffer + sizeof(_startRhs) +
                                                sizeof(_offsetAfter));
  _blocks.resize(nofBlocks);
  memcpy(_blocks.data(),
         buffer + sizeof(_startRhs) + sizeof(_offsetAfter) +
         sizeof(nofBlocks),
         nofBlocks * sizeof(BlockMetaData));

  return *this;
}


// _____________________________________________________________________________
size_t FullRelationMetaData::bytesRequired() const {
  return sizeof(_relId)
         + sizeof(_startFullIndex)
         + sizeof(_typeMultAndNofElements);
}

// _____________________________________________________________________________
off_t FullRelationMetaData::getStartOfLhs() const {
  AD_CHECK(hasBlocks());
  return _startFullIndex + 2 * sizeof(Id) * getNofElements();
}

// _____________________________________________________________________________
size_t BlockBasedRelationMetaData::bytesRequired() const {
  return sizeof(_startRhs)
         + sizeof(_offsetAfter)
         + sizeof(size_t)
         + _blocks.size() * sizeof(BlockMetaData);
}

// _____________________________________________________________________________
BlockBasedRelationMetaData::BlockBasedRelationMetaData() :
    _startRhs(0), _offsetAfter(0), _blocks() {}

// _____________________________________________________________________________
BlockBasedRelationMetaData::BlockBasedRelationMetaData(
    off_t
    startRhs,
    off_t
    offsetAfter,
    const vector<BlockMetaData>& blocks) :
    _startRhs(startRhs), _offsetAfter(offsetAfter), _blocks(blocks) {}



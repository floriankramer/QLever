// Copyright 2019, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Florian Kramer (florian.kramer@mail.uni-freiburg.de)

#pragma once

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <ostream>
#include "../global/Id.h"
#include "../util/Log.h"

/**
 * @brief This struct is used to store all the template specialized data and
 * methods of the IdTable class. This way less code of the IdTable has to
 * be duplicated.
 **/
template <int COLS>
class IdTableImpl {
 public:
  IdTableImpl()
      : _data(nullptr), _size(0), _capacity(0), _manage_storage(true) {}
  /**
   * This is simply an Id* which has some additional methods.
   **/
  class ConstRow {
   public:
    virtual ~ConstRow() {}
    bool operator==(ConstRow& other) const {
      bool matches = true;
      for (size_t i = 0; matches && i < COLS; i++) {
        matches &= reinterpret_cast<const Id*>(this)[i] ==
                   reinterpret_cast<const Id*>(&other)[i];
      }
      return matches;
    }
    const Id& operator[](size_t i) const {
      return *(reinterpret_cast<const Id*>(this) + i);
    }
    const Id* data() const { return reinterpret_cast<const Id*>(this); }
    size_t size() const { return COLS; }
  };

  /**
   * This provides access to a single row of a Table. The class can optionally
   * manage its own data, allowing for it to be swappable (otherwise swapping
   * two rows during e.g. a std::sort would lead to bad behaviour).
   **/
  class Row {
   public:
    Row(size_t cols) : _data(new Id[cols]), _allocated(true) {}
    Row(Id* data, size_t cols) : _data(data), _allocated(false) { (void)cols; }
    virtual ~Row() {
      if (_allocated) {
        delete[] _data;
      }
    }
    Row(const Row& other) : _data(new Id[COLS]), _allocated(true) {
      std::memcpy(_data, other._data, sizeof(Id) * COLS);
    }
    Row(Row&& other)
        : _data(other._allocated ? other._data : new Id[COLS]),
          _allocated(true) {
      if (other._allocated) {
        other._data = nullptr;
      } else {
        std::memcpy(_data, other._data, sizeof(Id) * COLS);
      }
    }

    Row& operator=(const Row& other) {
      if (_allocated) {
        // If we manage our own storage recreate that to fit the other row
        delete[] _data;
        _data = new Id[COLS];
      }
      // Copy over the data from the other row to this row
      if (COLS == COLS && _data != nullptr && other._data != nullptr) {
        std::memcpy(_data, other._data, sizeof(Id) * COLS);
      }
      return *this;
    }

    Row& operator=(Row&& other) {
      // This class cannot use move semantics if at least one of the two
      // rows invovled in an assigment does not manage it's data, but rather
      // functions as a view into an IdTable
      if (_allocated) {
        // If we manage our own storage recreate that to fit the other row
        delete[] _data;
        if (other._allocated) {
          // Both rows manage their own storage so we can take advantage
          // of move semantics.
          _data = other._data;
          return *this;
        } else {
          _data = new Id[COLS];
        }
      }
      // Copy over the data from the other row to this row
      if (_data != nullptr && other._data != nullptr) {
        std::memcpy(_data, other._data, sizeof(Id) * COLS);
      }
      return *this;
    }

    bool operator==(const Row& other) const {
      bool matches = true;
      for (size_t i = 0; matches && i < COLS; i++) {
        matches &= _data[i] == other._data[i];
      }
      return matches;
    }

    Id& operator[](size_t i) { return *(_data + i); }
    const Id& operator[](size_t i) const { return *(_data + i); }
    Id* data() { return _data; }
    const Id* data() const { return _data; }

    size_t size() const { return COLS; }
    size_t cols() const { return COLS; }

    Id* _data;
    bool _allocated;
  };

  class iterator {
   public:
    // iterator traits types
    using difference_type = ssize_t;
    using value_type = Row;
    using pointer = Row*;
    using reference = Row&;
    using iterator_category = std::forward_iterator_tag;

    iterator() : _data(nullptr), _row(0), _rowView(nullptr, 0) {}
    iterator(Id* data, size_t row, size_t cols)
        : _data(data), _row(row), _rowView(data + (cols * row), cols) {}
    virtual ~iterator() {}

    // Copy and move constructors and assignment operators
    iterator(const iterator& other)
        : _data(other._data),
          _row(other._row),
          _rowView(_data + (COLS * _row), COLS) {}

    iterator(iterator&& other)
        : _data(other._data),
          _row(other._row),
          _rowView(_data + (COLS * _row), COLS) {}

    iterator& operator=(const iterator& other) {
      new (this) iterator(other);
      return *this;
    }
    iterator& operator=(iterator&& other) {
      new (this) iterator(other);
      return *this;
    }

    // prefix increment
    iterator& operator++() {
      ++_row;
      _rowView._data = _data + (_row * COLS);
      return *this;
    }

    // postfix increment
    iterator operator++(int) {
      iterator tmp(*this);
      ++_row;
      _rowView._data = _data + (_row * COLS);
      return tmp;
    }

    // prefix increment
    iterator& operator--() {
      --_row;
      _rowView._data = _data + (_row * COLS);
      return *this;
    }

    // postfix increment
    iterator operator--(int) {
      iterator tmp(*this);
      --_row;
      _rowView._data = _data + (_row * COLS);
      return tmp;
    }

    iterator operator+(size_t i) const {
      return iterator(_data, _row + i, COLS);
    }
    iterator operator-(size_t i) const {
      return iterator(_data, _row - i, COLS);
    }
    ssize_t operator-(const iterator& other) const { return _row - other._row; }

    bool operator==(iterator const& other) const {
      return _data == other._data && _row == other._row;
    }

    bool operator!=(iterator const& other) const {
      return _data != other._data || _row != other._row;
    }

    bool operator<(const iterator& other) const { return _row < other._row; }
    bool operator>(const iterator& other) const { return _row > other._row; }
    bool operator<=(const iterator& other) const { return _row <= other._row; }
    bool operator>=(const iterator& other) const { return _row >= other._row; }

    Row& operator*() { return _rowView; }
    const Row& operator*() const { return _rowView; }

    Id& operator[](size_t i) { return *(_data + _row * COLS + i); }
    const Id& operator[](size_t i) const { return *(_data + _row * COLS + i); }

    size_t row() const { return _row; }
    size_t cols() const { return COLS; }
    size_t size() const { return COLS; }

   private:
    Id* _data;
    size_t _row;
    Row _rowView;
  };

  const ConstRow& getConstRow(size_t row) const {
    return *reinterpret_cast<ConstRow*>(_data + (row * COLS));
  }

  constexpr size_t cols() const { return COLS; }

  Id* _data;
  size_t _size;
  size_t _capacity;
  // This ensures the name _cols is always defined within the IdTableImpl
  // and allows for uniform usage.
  static constexpr int _cols = COLS;
  bool _manage_storage;

  void setCols(size_t cols) { (void)cols; };

  using const_row_type = const ConstRow&;
};

template <>
class IdTableImpl<0> {
 public:
  IdTableImpl()
      : _data(nullptr),
        _size(0),
        _capacity(0),
        _cols(0),
        _manage_storage(true) {}

  class ConstRow {
   public:
    ConstRow(const Id* data, size_t cols) : _data(data), _cols(cols) {}
    virtual ~ConstRow() {}
    ConstRow(const ConstRow& other) : _data(other._data), _cols(other._cols) {}
    ConstRow(ConstRow&& other) : _data(other._data), _cols(other._cols) {}
    ConstRow& operator=(const ConstRow& other) = delete;
    ConstRow& operator=(ConstRow&& other) = delete;
    bool operator==(ConstRow& other) const {
      bool matches = _cols == other._cols;
      for (size_t i = 0; matches && i < _cols; i++) {
        matches &= _data[i] == other._data[i];
      }
      return matches;
    }
    const Id& operator[](size_t i) const { return *(_data + i); }
    const Id* data() const { return _data; }
    size_t size() const { return _cols; }

    const Id* _data;
    size_t _cols;
  };

  /**
   * This provides access to a single row of a Table. The class can optionally
   * manage its own data, allowing for it to be swappable (otherwise swapping
   * two rows during e.g. a std::sort would lead to bad behaviour).
   **/
  class Row {
   public:
    Row(size_t cols) : _data(new Id[cols]), _cols(cols), _allocated(true) {}
    Row(Id* data, size_t cols) : _data(data), _cols(cols), _allocated(false) {}
    virtual ~Row() {
      if (_allocated) {
        delete[] _data;
      }
    }
    Row(const Row& other)
        : _data(new Id[other._cols]), _cols(other._cols), _allocated(true) {
      std::memcpy(_data, other._data, sizeof(Id) * _cols);
    }
    Row(Row&& other)
        : _data(other._allocated ? other._data : new Id[other._cols]),
          _cols(other._cols),
          _allocated(true) {
      if (other._allocated) {
        other._data = nullptr;
      } else {
        std::memcpy(_data, other._data, sizeof(Id) * _cols);
      }
    }

    Row& operator=(const Row& other) {
      if (_allocated) {
        // If we manage our own storage recreate that to fit the other row
        delete[] _data;
        _data = new Id[other._cols];
        _cols = other._cols;
      }
      // Copy over the data from the other row to this row
      if (_cols == other._cols && _data != nullptr && other._data != nullptr) {
        std::memcpy(_data, other._data, sizeof(Id) * _cols);
      }
      return *this;
    }

    Row& operator=(Row&& other) {
      // This class cannot use move semantics if at least one of the two
      // rows invovled in an assigment does not manage it's data, but rather
      // functions as a view into an IdTable
      if (_allocated) {
        // If we manage our own storage recreate that to fit the other row
        delete[] _data;
        if (other._allocated) {
          // Both rows manage their own storage so we can take advantage
          // of move semantics.
          _data = other._data;
          _cols = other._cols;
          return *this;
        } else {
          _data = new Id[other._cols];
          _cols = other._cols;
        }
      }
      // Copy over the data from the other row to this row
      if (_cols == other._cols && _data != nullptr && other._data != nullptr) {
        std::memcpy(_data, other._data, sizeof(Id) * _cols);
      }
      return *this;
    }

    bool operator==(const Row& other) const {
      bool matches = _cols == other._cols;
      for (size_t i = 0; matches && i < _cols; i++) {
        matches &= _data[i] == other._data[i];
      }
      return matches;
    }

    Id& operator[](size_t i) { return *(_data + i); }
    const Id& operator[](size_t i) const { return *(_data + i); }
    Id* data() { return _data; }
    const Id* data() const { return _data; }

    size_t size() const { return _cols; }
    size_t cols() const { return _cols; }

    Id* _data;
    size_t _cols;
    bool _allocated;
  };

  class iterator {
   public:
    using difference_type = ssize_t;
    using value_type = Row;
    using pointer = Row*;
    using reference = Row&;
    using iterator_category = std::forward_iterator_tag;

    iterator() : _data(nullptr), _row(0), _cols(0), _rowView(nullptr, 0) {}
    iterator(Id* data, size_t row, size_t cols)
        : _data(data),
          _row(row),
          _cols(cols),
          _rowView(data + (cols * row), cols) {}
    virtual ~iterator() {}

    // Copy and move constructors and assignment operators
    iterator(const iterator& other)
        : _data(other._data),
          _row(other._row),
          _cols(other._cols),
          _rowView(_data + (_cols * _row), _cols) {}

    iterator(iterator&& other)
        : _data(other._data),
          _row(other._row),
          _cols(other._cols),
          _rowView(_data + (_cols * _row), _cols) {}

    iterator& operator=(const iterator& other) {
      new (this) iterator(other);
      return *this;
    }
    iterator& operator=(iterator&& other) {
      new (this) iterator(other);
      return *this;
    }

    // prefix increment
    iterator& operator++() {
      ++_row;
      _rowView._data = _data + (_row * _cols);
      return *this;
    }

    // postfix increment
    iterator operator++(int) {
      iterator tmp(*this);
      ++_row;
      _rowView._data = _data + (_row * _cols);
      return tmp;
    }

    // prefix increment
    iterator& operator--() {
      --_row;
      _rowView._data = _data + (_row * _cols);
      return *this;
    }

    // postfix increment
    iterator operator--(int) {
      iterator tmp(*this);
      --_row;
      _rowView._data = _data + (_row * _cols);
      return tmp;
    }

    iterator operator+(size_t i) const {
      return iterator(_data, _row + i, _cols);
    }
    iterator operator-(size_t i) const {
      return iterator(_data, _row - i, _cols);
    }
    ssize_t operator-(const iterator& other) const { return _row - other._row; }

    bool operator==(iterator const& other) const {
      return _data == other._data && _row == other._row && _cols == other._cols;
    }

    bool operator!=(iterator const& other) const {
      return _data != other._data || _row != other._row || _cols != other._cols;
    }

    bool operator<(const iterator& other) const { return _row < other._row; }
    bool operator>(const iterator& other) const { return _row > other._row; }
    bool operator<=(const iterator& other) const { return _row <= other._row; }
    bool operator>=(const iterator& other) const { return _row >= other._row; }

    Row& operator*() { return _rowView; }
    const Row& operator*() const { return _rowView; }

    Id& operator[](size_t i) { return *(_data + _row * _cols + i); }
    const Id& operator[](size_t i) const { return *(_data + _row * _cols + i); }

    size_t row() const { return _row; }
    size_t cols() const { return _cols; }
    size_t size() const { return _cols; }

   private:
    Id* _data;
    size_t _row;
    size_t _cols;
    Row _rowView;
  };

  ConstRow getConstRow(size_t row) const {
    return ConstRow(_data + (row * _cols), _cols);
  }

  size_t cols() const { return _cols; }

  void setCols(size_t cols) { _cols = cols; }

  Id* _data;
  size_t _size;
  size_t _capacity;
  size_t _cols;
  bool _manage_storage;

  using const_row_type = ConstRow;
};

template <int COLS = 0>
class IdTableStatic;
using IdTable = IdTableStatic<>;

template <int COLS>
class IdTableStatic : private IdTableImpl<COLS> {
  // Make all other instantiations of this template friends of this.
  template <int>
  friend class IdTableStatic;
  template <int I>
  friend void swap(IdTableStatic<I>& left, IdTableStatic<I>& right);

 private:
  static constexpr float GROWTH_FACTOR = 1.5;

 public:
  using Row = typename IdTableImpl<COLS>::Row;
  using ConstRow = typename IdTableImpl<COLS>::const_row_type;
  using iterator = typename IdTableImpl<COLS>::iterator;
  using const_iterator = const typename IdTableImpl<COLS>::iterator;

  IdTableStatic() {
    IdTableImpl<COLS>::_data = nullptr;
    IdTableImpl<COLS>::_size = 0;
    IdTableImpl<COLS>::_capacity = 0;
    IdTableImpl<COLS>::setCols(0);
    IdTableImpl<COLS>::_manage_storage = true;
  }

  IdTableStatic(size_t cols) {
    IdTableImpl<COLS>::_data = nullptr;
    IdTableImpl<COLS>::_size = 0;
    IdTableImpl<COLS>::_capacity = 0;
    IdTableImpl<COLS>::setCols(cols);
    IdTableImpl<COLS>::_manage_storage = true;
  }

  virtual ~IdTableStatic() {
    if (IdTableImpl<COLS>::_manage_storage) {
      free(IdTableImpl<COLS>::_data);
    }
  }

  // Copy constructor
  IdTableStatic<COLS>(const IdTableStatic<COLS>& other) {
    IdTableImpl<COLS>::_data = other.IdTableImpl<COLS>::_data;
    IdTableImpl<COLS>::_size = other.IdTableImpl<COLS>::_size;
    IdTableImpl<COLS>::setCols(other.IdTableImpl<COLS>::_cols);
    IdTableImpl<COLS>::_capacity = other.IdTableImpl<COLS>::_capacity;
    if (other.IdTableImpl<COLS>::_data != nullptr) {
      IdTableImpl<COLS>::_data =
          reinterpret_cast<Id*>(malloc(IdTableImpl<COLS>::_capacity *
                                       IdTableImpl<COLS>::_cols * sizeof(Id)));
      std::memcpy(
          IdTableImpl<COLS>::_data, other.IdTableImpl<COLS>::_data,
          IdTableImpl<COLS>::_size * sizeof(Id) * IdTableImpl<COLS>::_cols);
    }
    IdTableImpl<COLS>::_manage_storage = true;
  }

  // Move constructor
  IdTableStatic<COLS>(IdTableStatic<COLS>&& other) {
    swap(*this, other);
    other.IdTableImpl<COLS>::_data = nullptr;
    other.IdTableImpl<COLS>::_size = 0;
    other.IdTableImpl<COLS>::_capacity = 0;
  }

  // copy assignment
  IdTableStatic<COLS>& operator=(IdTableStatic<COLS> other) {
    swap(*this, other);
    return *this;
  }

  // Element access
  Id& operator()(size_t row, size_t col) {
    return IdTableImpl<COLS>::_data[row * IdTableImpl<COLS>::_cols + col];
  }
  const Id& operator()(size_t row, size_t col) const {
    return IdTableImpl<COLS>::_data[row * IdTableImpl<COLS>::_cols + col];
  }

  // Row access
  Row operator[](size_t row) {
    return Row(IdTableImpl<COLS>::_data + row * IdTableImpl<COLS>::_cols,
               IdTableImpl<COLS>::_cols);
  }

  ConstRow operator[](size_t row) const {
    // Moving this method to impl allows for efficient ConstRow types when
    // using non dynamic IdTables.
    return IdTableImpl<COLS>::getConstRow(row);
  }

  // Begin iterator
  iterator begin() {
    return iterator(IdTableImpl<COLS>::_data, 0, IdTableImpl<COLS>::_cols);
  }
  const_iterator begin() const {
    return iterator(IdTableImpl<COLS>::_data, 0, IdTableImpl<COLS>::_cols);
  }
  const_iterator cbegin() const {
    return iterator(IdTableImpl<COLS>::_data, 0, IdTableImpl<COLS>::_cols);
  }

  // End iterator
  iterator end() {
    return iterator(IdTableImpl<COLS>::_data, IdTableImpl<COLS>::_size,
                    IdTableImpl<COLS>::_cols);
  }
  const_iterator end() const {
    return iterator(IdTableImpl<COLS>::_data, IdTableImpl<COLS>::_size,
                    IdTableImpl<COLS>::_cols);
  }
  const_iterator cend() const {
    return iterator(IdTableImpl<COLS>::_data, IdTableImpl<COLS>::_size,
                    IdTableImpl<COLS>::_cols);
  }

  Row back() {
    return Row(IdTableImpl<COLS>::_data +
                   (IdTableImpl<COLS>::_size - 1) * IdTableImpl<COLS>::_cols,
               IdTableImpl<COLS>::_cols);
  }

  ConstRow back() const {
    return ConstRow(IdTableImpl<COLS>::_data + (IdTableImpl<COLS>::_size - 1) *
                                                   IdTableImpl<COLS>::_cols,
                    IdTableImpl<COLS>::_cols);
  }

  Id* data() { return IdTableImpl<COLS>::_data; }
  const Id* data() const { return IdTableImpl<COLS>::_data; }

  // Size access
  size_t rows() const { return IdTableImpl<COLS>::_size; }

  /**
   * The template parameter here is used to allow for creating
   * a version of the function that is constexpr for COLS != 0
   * and one that is not constexpt for COLS == 0
   */
  using IdTableImpl<COLS>::cols;

  size_t size() const { return IdTableImpl<COLS>::_size; }

  /**
   * @brief Sets the number of columns. Should only be called while data is
   *        still nullptr.
   **/
  void setCols(size_t cols) {
    assert(IdTableImpl<COLS>::_data == nullptr);
    IdTableImpl<COLS>::setCols(cols);
  }

  void emplace_back() { push_back(); }

  void push_back() {
    if (IdTableImpl<COLS>::_size + 1 >= IdTableImpl<COLS>::_capacity) {
      grow();
    }
    IdTableImpl<COLS>::_size++;
  }

  void push_back(const std::initializer_list<Id>& init) {
    assert(init.size() == IdTableImpl<COLS>::_cols);
    if (IdTableImpl<COLS>::_size + 1 >= IdTableImpl<COLS>::_capacity) {
      grow();
    }
    std::memcpy(IdTableImpl<COLS>::_data +
                    IdTableImpl<COLS>::_size * IdTableImpl<COLS>::_cols,
                init.begin(), sizeof(Id) * IdTableImpl<COLS>::_cols);
    IdTableImpl<COLS>::_size++;
  }

  /**
   * @brief Read cols() elements from init and stores them in a new row
   **/
  void push_back(const Id* init) {
    if (IdTableImpl<COLS>::_size + 1 >= IdTableImpl<COLS>::_capacity) {
      grow();
    }
    std::memcpy(IdTableImpl<COLS>::_data +
                    IdTableImpl<COLS>::_size * IdTableImpl<COLS>::_cols,
                init, sizeof(Id) * IdTableImpl<COLS>::_cols);
    IdTableImpl<COLS>::_size++;
  }

  /**
   * @brief Read cols() elements from init and stores them in a new row
   **/
  void push_back(const Row& init) {
    assert(init.cols() == cols());
    if (IdTableImpl<COLS>::_size + 1 >= IdTableImpl<COLS>::_capacity) {
      grow();
    }
    std::memcpy(IdTableImpl<COLS>::_data +
                    IdTableImpl<COLS>::_size * IdTableImpl<COLS>::_cols,
                init.data(), sizeof(Id) * IdTableImpl<COLS>::_cols);
    IdTableImpl<COLS>::_size++;
  }

  /**
   * @brief Read cols() elements from init and stores them in a new row
   **/
  void push_back(ConstRow init) {
    if (IdTableImpl<COLS>::_size + 1 >= IdTableImpl<COLS>::_capacity) {
      grow();
    }
    std::memcpy(IdTableImpl<COLS>::_data +
                    IdTableImpl<COLS>::_size * IdTableImpl<COLS>::_cols,
                init.data(), sizeof(Id) * IdTableImpl<COLS>::_cols);
    IdTableImpl<COLS>::_size++;
  }

  void push_back(const IdTableStatic<COLS>& init, size_t row) {
    assert(init.IdTableImpl<COLS>::_cols == IdTableImpl<COLS>::_cols);
    if (IdTableImpl<COLS>::_size + 1 >= IdTableImpl<COLS>::_capacity) {
      grow();
    }
    std::memcpy(IdTableImpl<COLS>::_data +
                    IdTableImpl<COLS>::_size * IdTableImpl<COLS>::_cols,
                init.IdTableImpl<COLS>::_data + row * IdTableImpl<COLS>::_cols,
                sizeof(Id) * IdTableImpl<COLS>::_cols);
    IdTableImpl<COLS>::_size++;
  }

  void pop_back() {
    if (IdTableImpl<COLS>::_size > 0) {
      IdTableImpl<COLS>::_size--;
    }
  }

  /**
   * @brief Inserts the elements in the range [begin;end) before pos.
   **/
  void insert(const iterator& pos, const iterator& begin, const iterator& end) {
    assert(begin.cols() == cols());
    if (begin.row() >= end.row()) {
      return;
    }
    size_t target = std::min(pos.row(), this->end().row());
    size_t numNewRows = end.row() - begin.row();
    if (IdTableImpl<COLS>::_capacity < IdTableImpl<COLS>::_size + numNewRows) {
      size_t numMissing =
          IdTableImpl<COLS>::_size + numNewRows - IdTableImpl<COLS>::_capacity;
      grow(numMissing);
    }
    // Move the data currently in the way back. As the src and target
    // of the copy might overlap the rows are copied individually starting
    // at the back. The loop still counts up to avoid underflows.
    for (size_t i = target;
         i < target + numNewRows && i < IdTableImpl<COLS>::_size; i++) {
      size_t row = target + numNewRows - 1 - i;
      std::memcpy(IdTableImpl<COLS>::_data + row * IdTableImpl<COLS>::_cols,
                  IdTableImpl<COLS>::_data +
                      (row + numNewRows) * IdTableImpl<COLS>::_cols,
                  sizeof(Id) * IdTableImpl<COLS>::_cols);
    }
    IdTableImpl<COLS>::_size += numNewRows;
    // Copy the new data
    std::memcpy(IdTableImpl<COLS>::_data + target * IdTableImpl<COLS>::_cols,
                (*begin).data(),
                sizeof(Id) * IdTableImpl<COLS>::_cols * numNewRows);
  }

  /**
   * @brief Erases all rows in the range [begin;end)
   **/
  void erase(const_iterator& begin,
             const_iterator& end = iterator(nullptr, 0, 0)) {
    iterator actualEnd = end;
    if (actualEnd == iterator(nullptr, 0, 0)) {
      actualEnd = iterator(IdTableImpl<COLS>::_data, begin.row() + 1,
                           IdTableImpl<COLS>::_cols);
    }
    if (actualEnd.row() > IdTableStatic<COLS>::end().row()) {
      actualEnd = IdTableStatic<COLS>::end();
    }
    if (actualEnd.row() <= begin.row()) {
      return;
    }
    size_t numErased = actualEnd.row() - begin.row();
    // Move the elements from actualEnd() to end() forward
    for (size_t row = begin.row(); row + numErased < IdTableImpl<COLS>::_size;
         row++) {
      // copy a single row
      std::memcpy(IdTableImpl<COLS>::_data + row * IdTableImpl<COLS>::_cols,
                  IdTableImpl<COLS>::_data +
                      (row + numErased) * IdTableImpl<COLS>::_cols,
                  sizeof(Id) * IdTableImpl<COLS>::_cols);
    }
    IdTableImpl<COLS>::_size -= numErased;
  }

  void clear() { IdTableImpl<COLS>::_size = 0; }

  /**
   * @brief Ensures this table has enough space allocated to store rows many
   *        rows of data
   **/
  void reserve(size_t rows) {
    if (IdTableImpl<COLS>::_capacity < rows) {
      // Add rows - IdTableImpl<COLS>::_capacity many new rows
      grow(rows - IdTableImpl<COLS>::_capacity);
    }
  }

  /**
   * @brief Resizes this IdTableStatic to have at least row many rows.
   **/
  void resize(size_t rows) {
    if (rows > IdTableImpl<COLS>::_size) {
      reserve(rows);
      IdTableImpl<COLS>::_size = rows;
    } else if (rows < IdTableImpl<COLS>::_size) {
      IdTableImpl<COLS>::_size = rows;
    }
  }

  /**
   * @brief Creates an IdTableStatic<NEW_COLS> that now owns this
   * id tables data. This is effectively a move operation that also
   * changes the type to an equibalent one.
   **/
  template <int NEW_COLS>
  IdTableStatic<NEW_COLS> moveToStatic() {
    IdTableStatic<NEW_COLS> tmp;
    tmp.setCols(cols());
    tmp._data = IdTableImpl<COLS>::_data;
    tmp._size = IdTableImpl<COLS>::_size;
    tmp._capacity = IdTableImpl<COLS>::_capacity;
    IdTableImpl<COLS>::_data = nullptr;
    IdTableImpl<COLS>::_size = 0;
    IdTableImpl<COLS>::_capacity = 0;
    return tmp;
  };

  template <int NEW_COLS>
  const IdTableStatic<NEW_COLS> asStaticView() const {
    IdTableStatic<NEW_COLS> tmp;
    tmp.setCols(cols());
    tmp._data = IdTableImpl<COLS>::_data;
    tmp._size = IdTableImpl<COLS>::_size;
    tmp._capacity = IdTableImpl<COLS>::_capacity;
    // This prevents the view from freeing the memory
    tmp._manage_storage = false;
    return tmp;
  };

  /**
   * @brief Creates an IdTable that now owns this
   * id tables data. This is effectively a move operation that also
   * changes the type to an equibalent one.
   **/
  IdTable moveToDynamic() {
    IdTable tmp(cols());
    tmp._data = IdTableImpl<COLS>::_data;
    tmp._size = IdTableImpl<COLS>::_size;
    tmp._capacity = IdTableImpl<COLS>::_capacity;
    IdTableImpl<COLS>::_data = nullptr;
    IdTableImpl<COLS>::_size = 0;
    IdTableImpl<COLS>::_capacity = 0;
    return tmp;
  };

 private:
  /**
   * @brief Grows the storage of this IdTableStatic
   * @param newRows If newRows is 0 the storage is grown by GROWTH_FACTOR. If
   *        newRows is any other number the vector is grown by newRows many
   *        rows.
   **/
  void grow(size_t newRows = 0) {
    // If IdTableImpl<COLS>::_data is nullptr so is IdTableImpl<COLS>::_size
    // which makes this safe
    size_t new_capacity;
    if (newRows == 0) {
      new_capacity = IdTableImpl<COLS>::_capacity * GROWTH_FACTOR + 1;
    } else {
      new_capacity = IdTableImpl<COLS>::_capacity + newRows;
    }
    Id* larger = reinterpret_cast<Id*>(
        realloc(IdTableImpl<COLS>::_data,
                new_capacity * IdTableImpl<COLS>::_cols * sizeof(Id)));
    if (larger == nullptr) {
      // We were unable to acquire the memory
      free(IdTableImpl<COLS>::_data);
      IdTableImpl<COLS>::_data = nullptr;
      IdTableImpl<COLS>::_capacity = 0;
      IdTableImpl<COLS>::_size = 0;
      LOG(ERROR) << "Unable to grow the IDTable at "
                 << (reinterpret_cast<void*>(this)) << std::endl;
      throw std::bad_alloc();
    }
    IdTableImpl<COLS>::_data = larger;
    IdTableImpl<COLS>::_capacity = new_capacity;
  }
};

// Support for ostreams
template <int COLS>
inline std::ostream& operator<<(std::ostream& out,
                                const IdTableStatic<COLS>& table) {
  out << "IdTable(" << ((void*)table.data()) << ") with " << table.size()
      << " rows and " << table.cols() << " columns" << std::endl;
  for (size_t row = 0; row < table.size(); row++) {
    for (size_t col = 0; col < table.cols(); col++) {
      out << table(row, col) << ", ";
    }
    out << std::endl;
  }
  return out;
}

template <int COLS>
inline std::ostream& operator<<(std::ostream& out,
                                const typename IdTableStatic<COLS>::Row& row) {
  for (size_t col = 0; col < row.size(); col++) {
    out << row[col] << ", ";
  }
  out << std::endl;
  return out;
}

template <int COLS>
void swap(IdTableStatic<COLS>& left, IdTableStatic<COLS>& right) {
  using std::swap;
  swap(left._data, right._data);
  swap(left._size, right._size);
  swap(left._capacity, right._capacity);
  // As the cols member only exists for COLS = 0 we use the setCols
  // method here. setCols is guaranteed to always be defined even
  // if COLS != 0 (It might not have any effect though).
  size_t rcols = right.cols();
  right.IdTableImpl<COLS>::setCols(left.cols());
  left.IdTableImpl<COLS>::setCols(rcols);
  swap(left._manage_storage, right._manage_storage);
}

/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef ITERATOR_INCL
#define ITERATOR_INCL

#include <assert.h>
#include <stdarg.h>
#include "AllocationCategoryClasses.hpp"

namespace OMR {
namespace JB2 {

class Allocator;

// Establish the public interface for any Iterator class
// This is an abstract class with non-virtual functions so it cannot be used directly
// Subclass iterators are allowed to add more public API but must at least implement these functions
// Iterators in jb2 assume that the T type has a valid zero value
//
// All Iterators are expected to be manipulated as values and should not be dynamically allocated;
//
template <typename T>
class Iterator {
public:
    Iterator(Allocator *a)
        : _mem(a) {

    }

    // Rule of 3:
    Iterator(const Iterator & other)
        : _mem(other._mem) {

    }

    virtual ~Iterator() { }
    Iterator &operator=(const Iterator & other) {
        _mem = other._mem;
        return *this;
    }

    // Rule of 5:
    Iterator(Iterator && other)
        : _mem(other._mem) {
    }
    Iterator &operator=(Iterator && other) {
        _mem = other._mem;
        return *this;
    }

    void reset();
    bool hasItem();
    void operator++(int32_t);
    T item();

protected:
    T *allocate(size_t num) {
        return reinterpret_cast<T *>(_mem->allocate(num * sizeof(T), NoAllocationCategory));
    }
    Allocator *allocator() const { return _mem; }

    Allocator *_mem;
};

template <class T>
class ForwardSimpleIterator : public Iterator<T> {
public:
    // Rule of 3
    ForwardSimpleIterator(const ForwardSimpleIterator & other)
        : Iterator<T>(other)
        , _index(other._index)
        , _items(copyItems(other._items, other._length)) {
    }
    virtual ~ForwardSimpleIterator() {
        if (_ownItems)
            this->_mem->deallocate(_items);
    }
    ForwardSimpleIterator & operator=(const ForwardSimpleIterator & other) {
        _index = other._index;
        _items = copyItems(other._items, other._length);
    }

    // Rule of 5
    ForwardSimpleIterator(ForwardSimpleIterator && other)
        : Iterator<T>(other)
        , _index(other._index)
        , _length(other._length)
        , _ownItems(other._ownItems)
        , _items(other._items) {
        other._ownItems = false;
    }
    ForwardSimpleIterator & operator=(ForwardSimpleIterator && other) {
        _index = other._index;
        _length = other._length;
        _ownItems = other._ownItems;
        _items = other._items;
        other._ownItems = false;
    }

    ForwardSimpleIterator()
        : Iterator<T>(NULL)
        , _index(0)
        , _length(0)
        , _ownItems(false)
        , _items(NULL) {
    }
    ForwardSimpleIterator(Allocator *a, T one)
        : Iterator<T>(a)
        , _index(0)
        , _items(copyItems(1, one)) {
    }

    ForwardSimpleIterator(Allocator *a, T one, T two)
        : Iterator<T>(a)
        , _index(0)
        , _items(copyItems(2, one, two)) {
    }

    ForwardSimpleIterator(Allocator *a, T one, T two, T three)
        : Iterator<T>(a)
        , _index(0)
        , _items(copyItems(3, one, two, three)) {
    }

    ForwardSimpleIterator(Allocator *a, size_t numArgs, ...)
        : Iterator<T>(a)
        , _index(0) {

        va_list(args);
        va_start(args, numArgs);
        _items = copyItems(args, numArgs);
    }

    ForwardSimpleIterator(Allocator *a, T *array, size_t arraySize)
        : Iterator<T>(a)
        , _index(0)
        , _length(arraySize)
        , _ownItems(false)
        , _items(array) {
    }

    void reset()               { _index = 0; }
    bool hasItem() const       { return _index < _length; }
    void operator++(int32_t)   { assert(hasItem()); _index++; }
    T item() const             { assert(hasItem()); return _items[_index]; }

protected:
    T *copyItems(size_t numArgs, ...) { 
        va_list(args);
        va_start(args, numArgs);
        return copyItems(args, numArgs);
    }

    T *copyItems(va_list args, size_t numArgs) {
        T *array = NULL;
        if (numArgs > 0) {
            array = this->allocate(numArgs);
            for (size_t a=0;a < numArgs;a++)
                array[a] = va_arg(args, T);
            va_end(args);
        }
        _items = array;
        _ownItems = (_items != NULL);
        _length = numArgs;
        return array;
    }
    T *copyItems(T *oldArray, size_t arraySize) {
        T *newArray = this->allocate(arraySize);
        for (size_t a=0;a < arraySize;a++)
            newArray[a] = oldArray[a]; _items = newArray;
        _ownItems = (_items != NULL);
        _length = arraySize;
        return newArray;
    }

    size_t _index;
    size_t _length;
    bool    _ownItems;
    T     * _items;
};

template <class T>
class BackwardArrayIterator : public ForwardSimpleIterator<T> {
public:
    // so long as no BackwardArrayIterator<T> state, rule of 3 and
    //   rule of 5 satisfied already by ForwardArrayIterator<T>

    BackwardArrayIterator<T>()
        : ForwardSimpleIterator<T>() {
    }
    BackwardArrayIterator<T>(Allocator*a, T one)
        : ForwardSimpleIterator<T>(a, one) {
    }

    BackwardArrayIterator<T>(Allocator *a, T one, T two)
        : ForwardSimpleIterator<T>(a, two, one) {
    }

    BackwardArrayIterator<T>(Allocator *a, T one, T two, T three)
        : ForwardSimpleIterator<T>(a, three, two, one) {
    }

    BackwardArrayIterator<T>(Allocator *a, size_t numArgs, ...)
        : ForwardSimpleIterator<T>(a) {
        va_list(args);
        va_start(args, numArgs);
        ForwardSimpleIterator<T>::copyItems(numArgs, args);
        // now reverse _items
        for (size_t a=0;a < numArgs/2;a++) {
            T swap = this->_items[a];
            this->_items[numArgs-1 - a] = this->_items[a];
            this->_items[a] = swap;
        }
        va_end(args);
    }

    BackwardArrayIterator<T>(Allocator *a, T *array, size_t arraySize)
        : ForwardSimpleIterator<T>(a, array, arraySize) {
        
        if (!this->_ownItems) // base class constructor doesn't copy
            this->_items = copyItems(arraySize, array);

        // reverse _items now we own _items
        for (size_t a=0;a < arraySize/2;a++) {
            T swap = this->_items[a];
            this->_items[arraySize-1 - a] = this->_items[a];
            this->_items[a] = swap;
        }
    }

    // since constructors reverse the items, ForwardArrayIterator's implementation of
    //   public Iterator API will now return values in "reverse order"

};

} // namespace JB2
} // namespace OMR

#endif // defined(ITERATOR_INCL)

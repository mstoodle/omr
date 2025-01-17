/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
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

#ifndef JB2_ARRAY_INCL
#define JB2_ARRAY_INCL

#include <cassert>
#include <cstring>
#include "Allocatable.hpp"
#include "Iterator.hpp"

namespace OMR {
namespace JB2 {

class Allocator;

template <typename T>
class Array : public Allocatable {
    JBALLOC_NO_DESTRUCTOR(Array<T>, NoAllocationCategory)

    typedef uint64_t ChangeID;

public:

    class ForwardIterator : public OMR::JB2::ForwardSimpleIterator<T> {
    public:
        ForwardIterator() // empty iterator
            : ForwardSimpleIterator<T>()
            , _array(NULL)
            , _changeAtCreation(0) {

            }

        ForwardIterator(const Array<T> * array, bool detectChanges)
            : ForwardSimpleIterator<T>(array->allocator(), array->_items, array->_length)
            , _array(array)
            , _changeAtCreation(array->_changeID) {

            }

        // rule of 3
        ForwardIterator(const ForwardIterator & other)
            : ForwardSimpleIterator<T>(other.allocator(), other._items, other._length)
            , _array(other._array)
            , _changeAtCreation(other._changeAtCreation) {
        }
        virtual ~ForwardIterator() { }
        ForwardIterator operator=(const ForwardIterator & other) {
            _array = other._array;
            _changeAtCreation = other._changeAtCreation;
            return *this;
        }

        // rule of 5
        ForwardIterator(ForwardIterator && other)
            : ForwardSimpleIterator<T>(other.allocator(), other._items, other._length)
            , _array(other._array)
            , _changeAtCreation(other._changeAtCreation) {
        }
        ForwardIterator & operator=(const ForwardIterator && other) {
            _array = other._array;
            _changeAtCreation = other._changeAtCreation;
            return *this;
        }

        void reset() {
            if (!_array) return;
            assert(_changeAtCreation == _array->_changeID);
            ForwardSimpleIterator<T>::reset();
        }
        bool hasItem() {
            if (!_array) return false;
            assert(_changeAtCreation == _array->_changeID);
            return ForwardSimpleIterator<T>::hasItem();
        }
        void operator++(int i) {
            assert(_array != NULL && _changeAtCreation == _array->_changeID);
            return ForwardSimpleIterator<T>::operator++(i);
        }
        T item() const {
            assert(_array != NULL && _changeAtCreation == _array->_changeID);
            return ForwardSimpleIterator<T>::item();
        }

    protected:
        const Array<T> *_array;
        ChangeID _changeAtCreation;
    };

public:
    Array(Allocator *a)
        : Allocatable(a)
        , _arrayAllocator(a)
        , _changeID(0)
        , _ownItems(false)
        , _items(NULL)
        , _length(0) {
    }
    Array(Allocator *a, Allocator *arrayAllocator)
        : Allocatable(a)
        , _arrayAllocator(arrayAllocator)
        , _changeID(0)
        , _ownItems(false)
        , _items(NULL)
        , _length(0) {
    }

    Array(Allocator *a, T one)
        : Allocatable(a)
        , _arrayAllocator(a)
        , _changeID(0)
        , _items(initialize(1, one))
        , _length(0) {
    }
    Array(Allocator *a, Allocator *arrayAllocator, T one)
        : Allocatable(a)
        , _arrayAllocator(arrayAllocator)
        ,  _changeID(0)
        , _items(initialize(1, one))
        , _length(0) {
    }

    Array(Allocator *a, T one, T two)
        : Allocatable(a)
        , _arrayAllocator(a)
        , _changeID(0)
        , _items(initialize(2, one, two)) {
    }
    Array(Allocator *a, Allocator *arrayAllocator, T one, T two)
        : Allocatable(a)
        , _arrayAllocator(arrayAllocator)
        , _changeID(0)
        , _items(initialize(2, one, two)) {
    }

    Array(Allocator *a, T one, T two, T three)
        : Allocatable(a)
        , _arrayAllocator(a)
        , _changeID(0)
        , _items(initialize(3, one, two, three)) {
    }
    Array(Allocator *a, Allocator *arrayAllocator, T one, T two, T three)
        : Allocatable(a)
        , _arrayAllocator(arrayAllocator)
        , _changeID(0)
        , _items(initialize(3, one, two, three)) {
    }

    Array(Allocator *a, T *array, size_t arraySize)
        : Allocatable(a)
        , _arrayAllocator(a)
        , _changeID(0)
        , _ownItems(false)
        , _length(arraySize)
        , _items(array, arraySize) {
    }
    Array(Allocator *a, Allocator *arrayAllocator, T *array, size_t arraySize)
        : Allocatable(a)
        , _arrayAllocator(arrayAllocator)
        , _changeID(0)
        , _ownItems(false)
        , _length(arraySize)
        , _items(array, arraySize) {
    }

    virtual ~Array() {
        if (_ownItems && _items != NULL)
            deallocate(_items);
    }

    size_t length() const { return _length; }
    bool empty() const { return _length == 0; }
    bool exists(size_t index) const { return (index >= 0 && index < _length); }
    const T operator[] (size_t index) const {
        assert(exists(index));
        return _items[index];
    }
    T operator[](size_t index) {
        assert(exists(index));
        return _items[index];
    }
    void assign(size_t index, T v) {
        assert(index >= 0);
        if (index >= _length)
            grow(index);
        _changeID++; // assume A[i] = x but imperfect accounting unless we wrap the T
        _items[index] = v;
    }

    void erase() {
        _length = 0;
        if (_ownItems && _items != NULL)
            deallocate(_items);
        _ownItems = false;
        _changeID++;
    }

    ForwardIterator iterator(bool detectChanges=true) {
        return ForwardIterator(this, detectChanges);
    }
    ForwardIterator constIterator(bool detectChanges=true) const {
        return ForwardIterator(this, detectChanges);
    }
    ForwardIterator fwdIterator(bool detectChanges=true) {
        return ForwardIterator(this, detectChanges);
    }
    #if 0
    Array<T>::BackwardIterator revIterator(bool detectChanges=true) const {
        return List<T>::BackwardIterator(this, detectChanges);
    }
    #endif

protected:
    void grow(size_t indexNeeded) {
        if (indexNeeded < _length)
            return;

        size_t newLength = indexNeeded + 1;
        bool needDeallocate = _ownItems && _items != NULL;

        T *newItems = allocate(newLength);
        assert(newItems != NULL);

        T *zeroStart = newItems;
        size_t oldItemsBytes = _length * sizeof(T);
        if (oldItemsBytes > 0) {
            memcpy(newItems, _items, oldItemsBytes);
            zeroStart += _length;
        }
        memset(zeroStart, 0, (newLength - _length) * sizeof(T));
        if (needDeallocate)
            deallocate(_items);

        _items = newItems;
        _length = newLength;
        _ownItems = true;
    }

    T *initialize(size_t numArgs, ...) { 
        va_list(args);
        va_start(args, numArgs);
        return initialize(args, numArgs);
    }

    T *initialize(va_list args, size_t numArgs) {
        T *array = allocate(numArgs);
        for (size_t a=0;a < numArgs;a++)
            array[a] = va_arg(args, T);
        va_end(args);
        _items = array;
        _ownItems = true;
        _length = numArgs;
        return array;
    }
    T *initialize(T *oldArray, size_t arraySize) {
        T *newArray = allocate(arraySize);
        for (size_t a=0;a < arraySize;a++)
            newArray[a] = oldArray[a];
        _items = newArray;
        _ownItems = true;
        _length = arraySize;
        return newArray;
    }
    T *allocate(size_t arraySize) {
        return reinterpret_cast<T *>(_arrayAllocator->allocate<T>(arraySize * sizeof(T), NoAllocationCategory));
    }
    void deallocate(T *array) {
        _arrayAllocator->deallocate(array);
    }

    Allocator *_arrayAllocator;
    ChangeID _changeID;
    size_t _length;
    bool _ownItems;
    T *_items;
};

} // namespace JB2
} // namespace OMR

#endif // defined(JB2_ARRAY_INCL)

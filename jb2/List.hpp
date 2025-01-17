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

#ifndef JB2_LIST_INCL
#define JB2_LIST_INCL

#include <assert.h>
#include "Allocatable.hpp"
#include "Iterator.hpp"

namespace OMR {
namespace JB2 {

class Compiler;
class TextLogger;

template <typename T>
class List : public Allocatable {

    JBALLOC_NO_DESTRUCTOR(List<T>, NoAllocationCategory)

    typedef uint64_t ChangeID;

protected:
    class Item : public Allocatable {
        JBALLOC_NO_DESTRUCTOR(List<T>::Item, NoAllocationCategory)

        friend class List<T>;
    public:
        Item(Allocator *a, T item, List<T>::Item *prev=NULL, List<T>::Item *next=NULL)
            : Allocatable(a)
            , _item(item)
            , _prev(prev)
            , _next(next) {
        }

        virtual ~Item() { }

        void insertAfter(List<T>::Item *item) {
            item->_next = _next;
            item->_prev = this;
            if (_next)
                _next->_prev = item;
            _next = item;
        }
        void insertBefore(List<T>::Item *item) {
            item->_next = this;
            item->_prev = _prev;
            if (_prev)
                _prev->_next = item;
            _prev = item;
        }
        List<T>::Item *remove() {
            if (_prev)
                _prev->_next = _next;
            if (_next)
                _next->_prev = _prev;
            _prev = _next = NULL;
            return this;
        }
        List<T>::Item *removeTwo() {
            assert(_next);
            if (_prev)
                _prev->_next = _next->_next;
            if (_next->_next)
                _next->_next->_prev = _prev;
            _next->_next = NULL;
            _prev = NULL;
            return this;
        }
    protected:
        T _item;
        List<T>::Item *_prev;
        List<T>::Item *_next;
    };

public:
    class Iterator : public OMR::JB2::Iterator<T> {
        friend class List<T>;

    public:
        // Rule of 3:
        Iterator()
            : OMR::JB2::Iterator<T>(NULL)
            , _list(NULL)
            , _cursor(NULL)
            , _listSnapshot(0)
            , _detectChanges(false) {
        }

        Iterator(const Iterator & other)
            : OMR::JB2::Iterator<T>(other)
            , _list(other._list)
            , _cursor(other._cursor)
            , _listSnapshot(other._listSnapshot)
            , _detectChanges(other._detectChanges) {
        }
        virtual ~Iterator() { }
        Iterator &operator=(const Iterator & other) {
            assert(_list == other._list);
            //_list = other._list;
            _cursor = other._cursor;
            _listSnapshot = other._listSnapshot;
            _detectChanges = other._detectChanges;
            return *this;
        }

        // Rule of 5:
        Iterator(Iterator && other)
            : OMR::JB2::Iterator<T>(other)
            , _list(other._list)
            , _cursor(other._cursor)
            , _listSnapshot(other._listSnapshot)
            , _detectChanges(other._detectChanges) {
        }
        Iterator &operator=(Iterator && other)
            {
            assert(_list == other._list);
            //_list = other._list;
            _cursor = other._cursor;
            _listSnapshot = other._listSnapshot;
            _detectChanges = other._detectChanges;
            return *this;
        }

        // forward iteration
        void reset()               { _cursor = _list->_head; }
        bool hasItem()             { if (detectChange()) return false; return (_cursor != NULL); }
        bool hasTwoItems()         { if (detectChange()) return false; return (_cursor != NULL && _cursor->_next != NULL); }
        void operator++(int32_t) {
            assert(_cursor != NULL && !detectChange());
            //while (i > 0 && _cursor != NULL) {
            _cursor = _cursor->_next;
            //    i--;
            //}
            //assert(i == 0);
        }
        T item()                   { assert(_cursor != NULL && !detectChange()); return _cursor->_item; }
        T secondItem()             { assert(_cursor != NULL && _cursor->_next != NULL && !detectChange()); return _cursor->_next->_item; }

        // backward iteration
        void resetEnd()            { _cursor = _list->_tail; }
        void operator--(int32_t i) {
            assert(_cursor != NULL && !detectChange());
            //while (i > 0 && _cursor != NULL) {
            _cursor = _cursor->_prev;
            //    i--;
            //}
            //assert(i == 0);
        }
    protected:
        Iterator(Allocator *a, const List<T> * const originalList, bool startForward=true, bool detectChanges=true, bool makeCopy=false)
            : OMR::JB2::Iterator<T>(a)
            , _list(originalList->copy(makeCopy))
            , _cursor(NULL)
            , _listSnapshot(originalList->_changeID)
            , _detectChanges(detectChanges) {

            if (startForward)
                reset();
            else
                resetEnd();
        }
        #if 0
        Iterator() // used by ShortIterator
            : _list(NULL)
            , _cursor(NULL)
            , _listSnapshot(0)
            , _detectChanges(false) {
        }
        #endif
        virtual bool detectChange()  { return (_list != NULL && _list->_changeID != _listSnapshot); }

        const List<T> * const _list;
        List<T>::Item *_cursor;
        List<T>::ChangeID _listSnapshot;
        bool _detectChanges;
    };

public:
    List(Allocator *a)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(a)
        , _changeID(0)
        , _length(0) {
    }
    List(Allocator *a, Allocator *itemAllocator)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(itemAllocator)
        , _changeID(0)
        , _length(0) {
    }

    List(Allocator *a, T one)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(a)
        , _changeID(0)
        , _length(0) {

        push_back(one);
    }
    List(Allocator *a, Allocator *itemAllocator, T one)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(itemAllocator)
        , _changeID(0)
        , _length(0) {

        push_back(one);
    }

    List(Allocator *a, T one, T two)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(a)
        , _changeID(0)
        , _length(0) {

        push_back(one);
        push_back(two);
    }
    List(Allocator *a, Allocator *itemAllocator, T one, T two)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(itemAllocator)
        , _changeID(0)
        , _length(0) {

        push_back(one);
        push_back(two);
    }

    List(Allocator *a, T one, T two, T three)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(a)
        , _changeID(0)
        , _length(0) {

        push_back(one);
        push_back(two);
        push_back(three);
    }
    List(Allocator *a, Allocator *itemAllocator, T one, T two, T three)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(itemAllocator)
        , _changeID(0)
        , _length(0) {

        push_back(one);
        push_back(two);
        push_back(three);
    }

    List(Allocator *a, size_t numArgs, ...)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(a)
        , _changeID(0)
        , _length(0) {

        va_list(args);
        va_start(args, numArgs);
        for (size_t a=0;a < numArgs;a++)
            push_back(va_arg(args, T));
        va_end(args);
    }
    List(Allocator *a, Allocator *itemAllocator, size_t numArgs, ...)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(itemAllocator)
        , _changeID(0)
        , _length(0) {

        va_list(args);
        va_start(args, numArgs);
        for (size_t a=0;a < numArgs;a++)
            push_back(va_arg(args, T));
        va_end(args);
    }

    List(Allocator *a, T *array, size_t arraySize)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(a)
        , _changeID(0)
        , _length(0) {

        for (size_t a=0;a < arraySize;a++)
            push_back(va_arg(array, T));
    }
    List(Allocator *a, Allocator *itemAllocator, T *array, size_t arraySize)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(itemAllocator)
        , _changeID(0)
        , _length(0) {

        for (size_t a=0;a < arraySize;a++)
            push_back(va_arg(array, T));
    }

    List(const List<T> * source)
        : Allocatable(source->allocator())
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(source->_itemAllocator)
        , _changeID(0)
        , _length(0) {
        for (auto it = source->fwdIterator(); it.hasItem(); it++) {
            push_back(it.item());
        }
    }
    List(Allocator *a, const List<T> * source)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(a)
        , _changeID(0)
        , _length(0) {
        for (auto it = source->fwdIterator(); it.hasItem(); it++) {
            push_back(it.item());
        }
    }
    List(Allocator *a, Allocator *itemAllocator, const List<T> * source)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _itemAllocator(itemAllocator)
        , _changeID(0)
        , _length(0) {
        for (auto it = source->fwdIterator(); it.hasItem(); it++) {
            push_back(it.item());
        }
    }
    virtual ~List() {
        List<T>::Item *p=_head;
        while (p) {
            List<T>::Item *next = p->_next;
            delete p;
            p = next;
        }
        change(-_length);
    }

    size_t length() const { return _length; }
    bool empty() const { return _length == 0; }

    T back() const {
        assert(_tail != NULL);
        return _tail->_item;
    }
    T front() const {
        assert(_head != NULL);
        return _head->_item;
    }
    void push_front(T v) {
        List<T>::Item *newItem = new (_itemAllocator) List<T>::Item(_itemAllocator, v, NULL, _head);
        if (_head)
            _head->_prev = newItem;
        _head = newItem;
        if (_tail == NULL)
            _tail = newItem;
        change(1);
    }
    T pop_front() {
        assert (_head != NULL);
        List<T>::Item *item = _head;
        T v = item->_item;
        _head = item->_next;
        if (_head)
            _head->_prev = NULL;
        else
            _tail = NULL; // list must be empty now
        change(-1);
        delete item;
        return v;
    }
    void push_back(T v) {
        List<T>::Item *newItem = new (_itemAllocator) List<T>::Item(_itemAllocator, v, _tail, NULL);
        if (_tail)
            _tail->_next = newItem;
        _tail = newItem;
        if (_head == NULL)
            _head = newItem;
        change(1);
    }
    T pop_back() {
        assert(_tail != NULL);
        List<T>::Item *item = _tail;
        T v = item->_item;
        _tail = item->_prev;
        if (_tail)
            _tail->_next = NULL;
        else
            _head = NULL; // list must be empty now
        change(-1);
        delete item;
        return v;
    }
    void insertAfter(T v, List<T>::Iterator &cursor) {
        List<T>::Item *newItem = new (_itemAllocator) List<T>::Item(_itemAllocator, v);
        cursor._cursor->insertAfter(newItem);
        if (cursor._cursor == _tail)
            _tail = cursor._cursor->_next;
        change(1);
    }
    void insertBefore(T v, List<T>::Iterator &cursor) { 
        List<T>::Item *newItem = new (_itemAllocator) List<T>::Item(_itemAllocator, v);
        cursor._cursor->insertBefore(newItem);
        if (cursor._cursor == _head)
            _head = cursor._cursor->_prev;
        change(1);
    }

    List<T>::Iterator find(T v) {
        for (auto it = iterator(); it.hasItem(); it++ ) {
            if (v == it.item())
                return it;
        }
        return List<T>::Iterator();
    }

    // this doesn't work...need to return a new iterator!
    void remove(List<T>::Iterator &cursor) {
        List<T>::Item *current = cursor._cursor;
        if (current == _head)
            _head = current->_next;
        if (current == _tail)
            _tail = current->_prev;
        delete current->remove();
        change(-1);
    }
    void removeTwo(List<T>::Iterator &cursor) {
        List<T>::Item *current = cursor._cursor;
        assert(current != NULL && current->_next != NULL);
        List<T>::Item *after = current->_next->_next;
        if (current == _head)
            _head = after;
        if (after == NULL)
            _tail = current->_prev;
        delete current->_next->remove();
        delete current->remove();
        change(-1);
    }
    void log(TextLogger & lgr) {
        for (auto it=iterator();it.hasItem();it++) {
            T *x = it.item();
            x->log(lgr);
        }
    }
    void erase() {
        int32_t len=_length;
        for (auto it = iterator(); it.hasItem();) {
            List<T>::Item *item = it._cursor;
            it++; // must precede removal or else next pointer is lost
            delete item->remove();
        }
        change(-len);
        _head = _tail = NULL;
    }

    const List<T> *copy(bool makeCopy=true) const {
        return makeCopy ? (new (_itemAllocator) List<T>(_itemAllocator, this)) : this;
    }

    List<T>::Iterator iterator(bool forward=true, bool detectChanges=true, bool makeCopy=false) const {
        return List<T>::Iterator(_itemAllocator, this, forward, detectChanges, makeCopy);
    }
    List<T>::Iterator fwdIterator(bool detectChanges=true, bool makeCopy=false) const {
        return List<T>::Iterator(_itemAllocator, this, true, detectChanges, makeCopy);
    }
    List<T>::Iterator revIterator(bool detectChanges=true, bool makeCopy=false) const {
        return List<T>::Iterator(_itemAllocator, this, false, detectChanges, makeCopy);
    }

protected:
    void change(size_t i) { _changeID++; assert(_length + i >= 0); _length += i; }

    List<T>::Item *_head;
    List<T>::Item *_tail;
    Allocator * _itemAllocator;
    List<T>::ChangeID _changeID;
    size_t _length;
};

} // namespace JB2
} // namespace OMR

#endif // defined(JB2_LIST_INCL)

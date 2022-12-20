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

#ifndef LIST_INCL
#define LIST_INCL

#include <assert.h>
#include "Iterator.hpp"

namespace OMR {
namespace JitBuilder {

template <typename T>
class List {

    typedef uint64_t ChangeID;

protected:
    class Item {
        friend class List<T>;
    public:
        Item(T item, List<T>::Item *prev=NULL, List<T>::Item *next=NULL)
            : _item(item)
            , _prev(prev)
            , _next(next) {
        }

        void insertAfter(List<T>::Item *item) {
            item->_next = _next;
            item->_prev = this;
            _next = item;
        }
        void insertBefore(List<T>::Item *item) {
            item->_next = this;
            item->_prev = _prev;
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
    protected:
        T _item;
        List<T>::Item *_prev;
        List<T>::Item *_next;
    };

public:
    class Iterator : public OMR::JitBuilder::Iterator<T> {
        friend class List<T>;

    public:
        // Rule of 3:
        Iterator(const Iterator & other)
            : OMR::JitBuilder::Iterator<T>(other)
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
            : OMR::JitBuilder::Iterator<T>(other)
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
        void operator++(int32_t) {
            assert(_cursor != NULL && !detectChange());
            //while (i > 0 && _cursor != NULL) {
            _cursor = _cursor->_next;
            //    i--;
            //}
            //assert(i == 0);
        }
        T item()                   { assert(_cursor != NULL && !detectChange()); return _cursor->_item; };

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
        Iterator(const List<T> * const originalList, bool startForward=true, bool detectChanges=true, bool makeCopy=false)
            : _list(originalList->copy(makeCopy))
            , _cursor(NULL)
            , _listSnapshot(originalList->_changeID)
            , _detectChanges(detectChanges) {

            if (startForward)
                reset();
            else
                resetEnd();
        }
        Iterator() // used by ShortIterator
            : _list(NULL)
            , _cursor(NULL)
            , _listSnapshot(0)
            , _detectChanges(false) {
        }
        virtual bool detectChange()  { return (_list->_changeID != _listSnapshot); }

        const List<T> * const _list;
        List<T>::Item *_cursor;
        List<T>::ChangeID _listSnapshot;
        bool _detectChanges;
    };

public:
    List()
        : _head(NULL)
        , _tail(NULL)
        , _changeID(0)
        , _length(0) {
    }

    List(T one)
        : _head(NULL)
        , _tail(NULL)
        , _changeID(0)
        , _length(0) {

        push_back(one);
    }

    List(T one, T two)
        : _head(NULL)
        , _tail(NULL)
        , _changeID(0)
        , _length(0) {

        push_back(one);
        push_back(two);
    }

    List(T one, T two, T three)
        : _head(NULL)
        , _tail(NULL)
        , _changeID(0)
        , _length(0) {

        push_back(one);
        push_back(two);
        push_back(three);
    }

    List(int numArgs, ...)
        : _head(NULL)
        , _tail(NULL)
        , _changeID(0)
        , _length(0) {

        va_list(args);
        va_start(args, numArgs);
        for (int a=0;a < numArgs;a++)
            push_back(va_arg(args, T));
        va_end(args);
    }

    List(T *array, int arraySize)
        : _head(NULL)
        , _tail(NULL)
        , _changeID(0)
        , _length(0) {

        for (int a=0;a < arraySize;a++)
            push_back(va_arg(array, T));
    }

    List(const List<T> * source) {
        for (auto it = source->fwdIterator(); it.hasItem(); it++) {
            push_back(it.item());
        }
        change(source->length());
    }
    virtual ~List() {
        List<T>::Item *p=_head;
        while (p) {
            List<T>::Item *next = p->_next;
            JB2::deallocate<List<T>::Item>(p, 1);
            p = next;
        }
        change(-_length);
    }

    uint32_t length() const { return _length; }
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
        List<T>::Item *newItem = new (JB2::allocate<List<T>::Item>(1)) List<T>::Item(v, NULL, _head);
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
        List<T>::Item *newItem = new (JB2::allocate<List<T>::Item>(1)) List<T>::Item(v, _tail, NULL);
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
    void insertAfter(T v, List<T>::Iterator cursor) {
        List<T>::Item *newItem = new (JB2::allocate<List<T>::Item>(1)) List<T>::Item(v);
        cursor.current()->insertAfter(newItem);
        change(1);
    }
    void insertBefore(T v, List<T>::Iterator cursor) { 
        List<T>::Item *newItem = new (JB2::allocate<List<T>::Item>(1)) List<T>::Item(v);
        cursor.current()->insertBefore(newItem);
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
    void remove(List<T>::Iterator cursor) {
        List<T>::Item *current = cursor._cursor;
        if (current == _head)
            _head = current->_next;
        if (current == _tail)
            _tail = current->_prev;
        delete current->remove();
        change(-1);
    }
    void erase() {
        int32_t len=_length;
        for (auto it = iterator(); it.hasItem();) {
            List<T>::Item *item = it._cursor;
            it++; // must precede removal or else next pointer is lost
            item->remove();
        }
        change(-len);
    }

    const List<T> *copy(bool makeCopy=true) const {
        return makeCopy ? (new (JB2::allocate<List<T>>(1)) List<T>(this)) : this;
    }

    List<T>::Iterator iterator(bool forward=true, bool detectChanges=true, bool makeCopy=false) const {
        return List<T>::Iterator(this, forward, detectChanges, makeCopy);
    }
    List<T>::Iterator fwdIterator(bool detectChanges=true, bool makeCopy=false) const {
        return List<T>::Iterator(this, true, detectChanges, makeCopy);
    }
    List<T>::Iterator revIterator(bool detectChanges=true, bool makeCopy=false) const {
        return List<T>::Iterator(this, false, detectChanges, makeCopy);
    }

protected:
    void change(int32_t i) { _changeID++; assert(_length + i >= 0); _length += i; }

    List<T>::Item *_head;
    List<T>::Item *_tail;
    List<T>::ChangeID _changeID;
    int32_t _length;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(LIST_INCL)

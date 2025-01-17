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

#ifndef MAPPER_INCL
#define MAPPER_INCL

#include "common.hpp"
#include "String.hpp"


namespace OMR {
namespace JB2 {

template <typename T>
class Mapper : public Allocatable {
    //JBALLOC_NO_DESTRUCTOR_(Mapper)

private:
    struct Element : public Allocatable {
        Element(Allocator *a, T *t, String name="", size_t offset=0)
            : Allocatable(a)
            , _item(t)
            , _name(name)
            , _offset(offset)
            , _next(NULL) {
	    }

        T *_item;
        String _name;
        size_t _offset;
        Element *_next;
    };

public:
    Mapper(Allocator *a)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _current(NULL)
        , _size(0) {

    }

    Mapper(Allocator *a, T *t, String name="", size_t offset=0)
        : Allocatable(a)
        , _head(NULL)
        , _tail(NULL)
        , _current(NULL)
        , _size(0) {

        add(t, name, offset);
    }
    virtual ~Mapper() {
        Allocator *mem = allocator();
        Element *p = _head;
        while (p) {
            Element *n = p->_next;
            mem->deallocate(p);
            p = n;
        }
    }

    size_t size()  { return _size; }

    // add() adds a new element to the mapper
    // next() returns the next value from _current
    // start() should be called to ensure next() starts from the first element
    // clear() can be used to empty out the mapper

    // The list always wraps around so next() will always return an element once something
    // has been added to the Mapper.
    // Expected common scenarios:
    //   1) _items has several item; TypeReplacer calls next() as many times as size()
    //   2) _items has one item and TypeReplacer will call next() many times to reuse that item
    //          with different items returned by another Mapper (like "scalar" expansion)

    void start() { _current = _head; }
    void add(T *t, String name="", size_t offset=0) {
        Allocator *mem = allocator();
        Element *elem = new (mem) Element(mem, t, name, offset);
        if (_head == NULL) {
            _head = elem;
            _current = _head;
        }
        if (_tail)
            _tail->_next = elem;
        _tail = elem;
        elem->_next = _head;
        _size++;
    }

    void clear() {
        if (_head == NULL) return;
        while (_head != _tail) {
            Element *next = _head->_next;
            delete _head;
            _head = next;
        }
        delete _head;
        _head = _tail = _current = NULL;
        _size = 0;
    }

    T *next() {
        if (!_current) return NULL;
        T *item = _current->_item;
        _current = _current->_next;
        return item;
    }
    T *current() {
        return _current->_item;
    }
    String name() {
        // must be called *before* next() to get correct name
        if (!_current)
            return String(allocator(), "");
        return _current->_name;
    }
    size_t offset() {
        // must be called *before* next() to geet correct offset
        if (!_current)
            return 0;
        return _current->_offset;
    }

private:
    Element *_head;
    Element *_tail;
    Element *_current;
    size_t   _size;
};


class Builder;
typedef Mapper<Builder> BuilderMapper;
//INIT_JBALLOC_TEMPLATE(Mapper, Builder, TypeReplacement::allocCat())

class Literal;
typedef Mapper<Literal> LiteralMapper;
//INIT_JBALLOC_TEMPLATE(Mapper, Literal, TypeReplacement::allocCat())

class Symbol;
typedef Mapper<Symbol> SymbolMapper;
//INIT_JBALLOC_TEMPLATE(Mapper, Symbol, TypeReplacement::allocCat())

class Type;
typedef Mapper<const Type> TypeMapper;
//INIT_JBALLOC_TEMPLATE(Mapper, Type, TypeReplacement::allocCat())

class Value;
typedef Mapper<Value> ValueMapper;
//INIT_JBALLOC_TEMPLATE(Mapper, Value, TypeReplacement::allocCat())


} // namespace JB2
} // namespace OMR

#endif // defined(MAPPER_INCL)


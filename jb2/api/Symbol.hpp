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

#ifndef SYMBOL_INCL
#define SYMBOL_INCL

#include <cassert>
#include <string>
#include <vector>
#include "IDs.hpp"
#include "KindService.hpp"
#include "util/String.hpp"

namespace OMR {
namespace JitBuilder {

class SymbolDictionary;
class TextWriter;
class Type;

typedef KindService::Kind SymbolKind;

class Symbol {
    friend class SymbolDictionary;

public:
    static Symbol *create(String name, const Type * type) { return new Symbol(name, type); }

    String name() const { return _name; }
    const Type * type() const { return _type; }
    SymbolID id() const { return _id; }
    virtual SymbolKind kind() const { return _kind; }

    template<typename T>
    bool isExactKind() const {
        return (kindService.isExactMatch(_kind, T::getSymbolClassKind()));
    }

    template<typename T>
    bool isKind() const {
        return (kindService.isMatch(_kind, T::getSymbolClassKind()));
    }

    template<typename T>
    T *refine() {
        assert(isKind<T>());
        return static_cast<T *>(this);
    }

    virtual void write(TextWriter & w) const;

    static const SymbolKind getSymbolClassKind();

protected:
    Symbol(String name, const Type * type)
        : _id(NoSymbol)
        , _kind(getSymbolClassKind())
        , _name(name)
        , _type(type) {

    }
    Symbol(SymbolKind kind, String name, const Type * type)
        : _id(NoSymbol)
        , _kind(kind)
        , _name(name)
        , _type(type) {

    }

    void assignID(SymbolID id) {
        // TODO convert to CompilationException
        assert(_id == NoSymbol);
       	assert(id != NoSymbol);
       	_id = id;
    }
    SymbolKind assignSymbolKind(SymbolKind baseKind, String name);

    SymbolID _id;
    SymbolKind _kind;
    String _name;
    const Type * _type;

    static KindService kindService;
    static SymbolKind SYMBOLKIND;
    static bool kindRegistered;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(SYMBOL_INCL)


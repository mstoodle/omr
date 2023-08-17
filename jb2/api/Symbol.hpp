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
#include "IDs.hpp"
#include "KindService.hpp"
#include "String.hpp"

namespace OMR {
namespace JitBuilder {

class Extension;
class IRCloner;
class SymbolDictionary;
class TextLogger;
class Type;
class TypeDictionary;

KINDSERVICE_CATEGORY(Symbol);

class Symbol : public Allocatable {
    JBALLOC_(Symbol)

    friend class IRCloner;
    friend class SymbolDictionary;

public:
    String name() const { return _name; }
    const Type * type() const { return _type; }
    SymbolID id() const { return _id; }
    void log(TextLogger & lgr) const;
    virtual void logDetails(TextLogger & lgr) const { }

protected:
    Symbol(Allocator *mem, Extension *ext, String name, const Type * type)
        : Allocatable(mem)
        , _ext(ext)
        , _id(NoSymbol)
        , _name(name)
        , _type(type)
        , BASECLASS_KINDINIT(getSymbolClassKind()) {

    }
    Symbol(Allocator *mem, SymbolKind kind, Extension *ext, String name, const Type * type)
        : Allocatable(mem)
        , _ext(ext)
        , _id(NoSymbol)
        , _name(name)
        , _type(type)
        , BASECLASS_KINDINIT(kind) {

    }
    Symbol(Allocator *mem, const Symbol *source, IRCloner *cloner);

    virtual Symbol *clone(Allocator *a, IRCloner *cloner);

    void assignID(SymbolID id) {
        // TODO convert to CompilationException
        assert(_id == NoSymbol);
       	assert(id != NoSymbol);
       	_id = id;
    }

    Extension *_ext;
    SymbolID _id;
    String _name;
    const Type * _type;

    BASECLASS_KINDSERVICE_DECL(Symbol);
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(SYMBOL_INCL)


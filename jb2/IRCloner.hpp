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

#ifndef IRCLONER_INCL
#define IRCLONER_INCL

#include <stddef.h>
#include <stdint.h>
#include "common.hpp"
#include "ExtensibleIR.hpp"

namespace OMR {
namespace JB2 {

class Builder;
class Context;
class EntryPoint;
class Extension;
class IR;
class Literal;
class LiteralDictionary;
class Location;
class Operation;
class Scope;
class Symbol;
class SymbolDictionary;
class Type;
class TypeDictionary;
class Value;

class IRCloner : public Extensible {
    friend class IR;

public:
    IRCloner(Allocator *mem, Extension *ext);
    IRCloner(IR *ir);
    virtual ~IRCloner();

    void setClonedIR(IR *clonedIR);

    Allocator *mem() const { return _mem; }
    IR *clonedIR() { return _clonedIR; }

    Builder *clonedBuilder(Builder *b);
    Context *clonedContext(Context *c);
    EntryPoint *clonedEntryPoint(EntryPoint *e);
    Literal *clonedLiteral(Literal *lv);
    LiteralDictionary *clonedLiteralDictionary(LiteralDictionary *d);
    Location *clonedLocation(Location *loc);
    Operation *clonedOperation(Operation *op);
    Scope *clonedScope(Scope *s);
    Symbol *clonedSymbol(Symbol *s);
    SymbolDictionary *clonedSymbolDictionary(SymbolDictionary *d);
    const Type *clonedType(const Type *t);
    const Type **clonedTypeArray(int32_t numTypes, const Type **typeArray);
    TypeDictionary *clonedTypeDictionary(TypeDictionary *d);
    Value *clonedValue(Value *v);
    ExtensibleIR *clone(const ExtensibleIR *item);

protected:
    Allocator                 * _mem;
    IR                        * _clonedIR;
    Array<Builder *>            _clonedBuilders;
    Array<Context *>            _clonedContexts;
    Array<EntryPoint *>         _clonedEntryPoints;
    Array<Literal *>            _clonedLiterals;
    Array<LiteralDictionary *>  _clonedLiteralDictionaries;
    Array<Location *>           _clonedLocations;
    Array<Operation *>          _clonedOperations;
    Array<Scope *>              _clonedScopes;
    Array<Symbol *>             _clonedSymbols;
    Array<SymbolDictionary *>   _clonedSymbolDictionaries;
    Array<const Type *>         _clonedTypes;
    Array<TypeDictionary *>     _clonedTypeDictionaries;
    Array<Value *>              _clonedValues;
};

} // namespace JB2
} // namespace OMR

#endif // defined(IRCLONER_INCL)

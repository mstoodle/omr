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

#include "Builder.hpp"
//#include "Context.hpp"
#include "EntryPoint.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "Location.hpp"
#include "IR.hpp"
#include "IRCloner.hpp"
#include "Operation.hpp"
#include "Scope.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {

IRCloner::IRCloner(Allocator *mem, Extension *ext)
    : Extensible(mem, ext, CLASSKIND(IR, Extensible))
    , _mem(mem)
    , _clonedIR(NULL)
    , _clonedBuilders(NULL, _mem)
    , _clonedContexts(NULL, _mem)
    , _clonedEntryPoints(NULL, _mem)
    , _clonedLiterals(NULL, _mem)
    , _clonedLiteralDictionaries(NULL, _mem)
    , _clonedLocations(NULL, _mem)
    , _clonedOperations(NULL, _mem)
    , _clonedScopes(NULL, _mem)
    , _clonedSymbols(NULL, _mem)
    , _clonedSymbolDictionaries(NULL, _mem)
    , _clonedTypes(NULL, _mem)
    , _clonedTypeDictionaries(NULL, _mem)
    , _clonedValues(NULL, _mem) {

}

IRCloner::IRCloner(IR *ir)
    : Extensible(ir->mem(), ir->ext(), CLASSKIND(IR, Extensible))
    , _mem(ir->mem())
    , _clonedIR(ir)
    , _clonedBuilders(NULL, _mem)
    , _clonedContexts(NULL, _mem)
    , _clonedEntryPoints(NULL, _mem)
    , _clonedLiterals(NULL, _mem)
    , _clonedLiteralDictionaries(NULL, _mem)
    , _clonedLocations(NULL, _mem)
    , _clonedOperations(NULL, _mem)
    , _clonedScopes(NULL, _mem)
    , _clonedSymbols(NULL, _mem)
    , _clonedSymbolDictionaries(NULL, _mem)
    , _clonedTypes(NULL, _mem)
    , _clonedTypeDictionaries(NULL, _mem)
    , _clonedValues(NULL, _mem) {

}

IRCloner::~IRCloner() {
    #if 0
    for (auto it=_clonedBuilders.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedContexts.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedEntryPoints.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedLiterals.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedLiteralDictionaries.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedLocations.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedOperations.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedScopes.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedSymbolDictionaries.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedTypes.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedTypeDictionaries.iterator();it.hasItem();it++)
        delete (it.item());

    for (auto it=_clonedValues.iterator();it.hasItem();it++)
        delete (it.item());
    #endif
}

void
IRCloner::setClonedIR(IR *clonedIR) {
    _clonedIR = clonedIR;
}

Builder *
IRCloner::clonedBuilder(Builder *b) {
    BuilderID id=b->id();
    Builder *clonedBuilder = NULL;
    if (id < _clonedBuilders.length())
        clonedBuilder = _clonedBuilders[id];
    
    if (clonedBuilder == NULL) {
        clonedBuilder = b->cloneBuilder(_mem, this);
        _clonedBuilders.assign(id, clonedBuilder);
    }

    return clonedBuilder;
}

Context *
IRCloner::clonedContext(Context *ctx) {
    if (ctx == NULL)
        return NULL;

    ContextID id=ctx->id();
    Context *clonedContext = NULL;
    if (id < _clonedContexts.length())
        clonedContext = _clonedContexts[id];
    
    if (clonedContext == NULL) {
        clonedContext = ctx->cloneContext(_mem, this);
        _clonedContexts.assign(id, clonedContext);
    }

    return clonedContext;
}

// Note that not all EntryPoints are copied (e.g. NativeEntry and DebugEntry)
// If there is no equivalent cloned EntryPoint this function returns NULL
EntryPoint *
IRCloner::clonedEntryPoint(EntryPoint *e) {
    EntryPointID id=e->id();
    EntryPoint *clonedEntry = NULL;
    if (id < _clonedEntryPoints.length())
        clonedEntry = _clonedEntryPoints[id];
    
    if (clonedEntry == NULL) {
        clonedEntry = e->cloneEntryPoint(_mem, this);
        if (clonedEntry != NULL)
            _clonedEntryPoints.assign(id, clonedEntry);
    }

    return clonedEntry;
}

Literal *
IRCloner::clonedLiteral(Literal *lv) {
    LiteralID id=lv->id();
    Literal *clonedLiteral = NULL;
    if (id < _clonedLiterals.length())
        clonedLiteral = _clonedLiterals[id];
    
    if (clonedLiteral == NULL) {
        clonedLiteral = lv->cloneLiteral(_mem, this);
        _clonedLiterals.assign(id, clonedLiteral);
    }

    return clonedLiteral;
}

LiteralDictionary *
IRCloner::clonedLiteralDictionary(LiteralDictionary *d) {
    if (d == NULL)
        return NULL;

    LiteralDictionaryID id=d->id();
    LiteralDictionary *clonedDict = NULL;
    if (id < _clonedLiteralDictionaries.length())
        clonedDict = _clonedLiteralDictionaries[id];
    
    if (clonedDict == NULL) {
        clonedDict = d->cloneDictionary(_mem, this);
        _clonedLiteralDictionaries.assign(id, clonedDict);
        clonedDict->cloneFrom(d, this);
    }

    return clonedDict;
}

Location *
IRCloner::clonedLocation(Location *loc) {
    LocationID id=loc->id();
    Location *clonedLocation = NULL;
    if (id < _clonedLocations.length())
        clonedLocation = _clonedLocations[id];
    
    if (clonedLocation == NULL) {
        clonedLocation = loc->cloneLocation(_mem, this);
        _clonedLocations.assign(id, clonedLocation);
    }

    return clonedLocation;
}

Operation *
IRCloner::clonedOperation(Operation *op) {
    OperationID id=op->id();
    Operation *clonedOperation = NULL;
    if (id < _clonedOperations.length())
        clonedOperation = _clonedOperations[id];
    
    if (clonedOperation == NULL) {
        clonedOperation = op->cloneOperation(_mem, this);
        _clonedOperations.assign(id, clonedOperation);
    }

    return clonedOperation;
}

Scope *
IRCloner::clonedScope(Scope *s) {
    if (s == NULL)
        return NULL;

    ScopeID id=s->id();
    Scope *clonedScope = NULL;
    if (id < _clonedScopes.length())
        clonedScope = _clonedScopes[id];
    
    if (clonedScope == NULL) {
        clonedScope = s->cloneScope(_mem, this);
        _clonedScopes.assign(id, clonedScope);
    }

    return clonedScope;
}

Symbol *
IRCloner::clonedSymbol(Symbol *s) {
    SymbolID id=s->id();
    Symbol *clonedSymbol = NULL;
    if (id < _clonedSymbols.length())
        clonedSymbol = _clonedSymbols[id];
    
    if (clonedSymbol == NULL) {
        clonedSymbol = s->cloneSymbol(_mem, this);
        _clonedSymbols.assign(id, clonedSymbol);
    }

    return clonedSymbol;
}

SymbolDictionary *
IRCloner::clonedSymbolDictionary(SymbolDictionary *d) {
    if (d == NULL)
        return NULL;

    SymbolDictionaryID id=d->id();
    SymbolDictionary *clonedDict = NULL;
    if (id < _clonedSymbolDictionaries.length())
        clonedDict = _clonedSymbolDictionaries[id];
    
    if (clonedDict == NULL) {
        clonedDict = d->cloneDictionary(_mem, this);
        _clonedSymbolDictionaries.assign(id, clonedDict);
        clonedDict->cloneFrom(d, this);
    }

    return clonedDict;
}

const Type *
IRCloner::clonedType(const Type *type) {
    TypeID id=type->id();
    const Type *clonedType = NULL;
    if (id < _clonedTypes.length())
        clonedType = _clonedTypes[id];
    
    if (clonedType == NULL) {
        clonedType = type->cloneType(_mem, this);
        _clonedTypes.assign(id, clonedType);
    }

    return clonedType;
}

const Type **
IRCloner::clonedTypeArray(int32_t numTypes, const Type ** typeArray) {
    const Type ** clonedTypeArray = _mem->allocate<const Type *>(numTypes);
    for (int32_t i=0;i < numTypes;i++) {
        const Type *type = typeArray[i];
        clonedTypeArray[i] = clonedType(type);
    }
    return clonedTypeArray;
}

TypeDictionary *
IRCloner::clonedTypeDictionary(TypeDictionary *d) {
    if (d == NULL)
        return NULL;

    TypeDictionaryID id=d->id();
    TypeDictionary *clonedDict = NULL;
    if (id < _clonedTypeDictionaries.length())
        clonedDict = _clonedTypeDictionaries[id];
    
    if (clonedDict == NULL) {
        clonedDict = d->cloneDictionary(_mem, this);
        _clonedTypeDictionaries.assign(id, clonedDict);
        clonedDict->cloneFrom(d, this);
    }

    return clonedDict;
}

Value *
IRCloner::clonedValue(Value *v) {
    ValueID id=v->id();
    Value *clonedValue = NULL;
    if (id < _clonedValues.length())
        clonedValue = _clonedValues[id];
    
    if (clonedValue == NULL) {
        clonedValue = v->cloneValue(_mem, this);
        _clonedValues.assign(id, clonedValue);
    }

    return clonedValue;
}

ExtensibleIR *
IRCloner::clone(const ExtensibleIR *item) {
    return item->clone(_mem, this);
}

} // namespace JB2
} // namespace OMR

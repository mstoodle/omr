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

#include <assert.h>
#include "Debug/DebugCompilation.hpp"
#include "Debug/DebugContext.hpp"
#include "Debug/DebugExtension.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

INIT_JBALLOC_REUSECAT(DebugContext, Context);
SUBCLASS_KINDSERVICE_IMPL(DebugContext, "DebugContext", Context, Context);

DebugContext::DebugContext(LOCATION, DebugExtension *dbx, Func::FunctionCompilation *comp, Compilation *compToDebug, String name)
    : Func::FunctionContext(PASSLOC, KIND(Context), comp, name)
    , _dbx(dbx)
    , _bx(dbx->bx())
    , _fx(dbx->fx())
    , _compToDebug(compToDebug) {

}

DebugContext::DebugContext(LOCATION, DebugExtension *dbx, DebugContext *caller, String name)
    : Func::FunctionContext(PASSLOC, KIND(Context), caller, name)
    , _dbx(dbx)
    , _bx(dbx->bx())
    , _fx(dbx->fx())
    , _compToDebug(caller->compToDebug()) {

}

DebugContext::~DebugContext() {
}

bool
DebugContext::dissolveAtEntry(Operation *op, Builder *b) {
    if (op->numSymbols() == 0)
        return false;

    for (auto sIt = op->symbols(); sIt.hasItem(); sIt++) {
        Symbol *sym = sIt.item();
        _fx->Store(LOC, b, sym, loadValue(LOC, b, sym));
    }
    return true;
}

bool
DebugContext::dissolveAtExit(Operation *op, Builder *b) {
    if (op->numSymbols() == 0)
        return false;

    for (auto sIt = op->symbols(); sIt.hasItem(); sIt++) {
        Symbol *sym = sIt.item();
        storeValue(LOC, b, sym, _fx->Load(LOC, b, sym));
    }

    for (auto vIt = op->results(); vIt.hasItem(); vIt++) {
        Value *value = vIt.item();
        storeValue(LOC, b, value, value);
    }
    return true;
}

void
DebugContext::storeValue(LOCATION, Builder *b, Symbol *local, Value *value) {
    Func::LocalSymbol *localsSym = LookupLocal("locals");
    storeToDebugValue(PASSLOC, b, bx()->IndexAt(PASSLOC, b, fx()->Load(PASSLOC, b, localsSym), bx()->ConstInt64(PASSLOC, b, index(local))), value);
}

void
DebugContext::storeValue(LOCATION, Builder *b, Value *destValue, Value *value) {
    Func::LocalSymbol *valuesSym = LookupLocal("values");
    storeToDebugValue(PASSLOC, b, bx()->IndexAt(PASSLOC, b, fx()->Load(PASSLOC, b, valuesSym), bx()->ConstInt64(PASSLOC, b, index(destValue))), value);
}

void
DebugContext::storeReturnValue(LOCATION, Builder *b, int32_t resultIdx, Value *value) {
    Func::LocalSymbol *frameSym = LookupLocal("frame");
    storeToDebugValue(PASSLOC, b, bx()->IndexAt(PASSLOC, b, bx()->LoadFieldAt(PASSLOC, b, comp()->refine<DebugCompilation>()->_DebugFrame_returnValues, fx()->Load(PASSLOC, b, frameSym)), bx()->ConstInt64(PASSLOC, b, resultIdx)), value);
}

Value *
DebugContext::loadValue(LOCATION, Builder *b, Symbol *local) {
    Func::LocalSymbol *localsSym = LookupLocal("locals");
    return loadFromDebugValue(PASSLOC, b, bx()->IndexAt(PASSLOC, b, fx()->Load(PASSLOC, b, localsSym), bx()->ConstInt64(PASSLOC, b, index(local))), local->type());
}

Value *
DebugContext::loadValue(LOCATION, Builder *b, Value *value) {
    Func::LocalSymbol *valuesSym = LookupLocal("values");
    return loadFromDebugValue(PASSLOC, b, bx()->IndexAt(PASSLOC, b, fx()->Load(PASSLOC, b, valuesSym), bx()->ConstInt64(PASSLOC, b, index(value))), value->type());
}

void
DebugContext::storeToDebugValue(LOCATION, Builder *b, Value *debugValue, Value *value) {
    const Type *type = value->type();
    bx()->StoreFieldAt(PASSLOC, b, comp()->refine<DebugCompilation>()->_DebugValue_type, debugValue, bx()->ConstInt64(PASSLOC, b, (int64_t)type));
    bx()->StoreFieldAt(PASSLOC, b, comp()->refine<DebugCompilation>()->lookupTypeField(type), debugValue, value);
}

Value *
DebugContext::loadFromDebugValue(LOCATION, Builder *b, Value *debugValueBase, const Type *type) {
    assert(debugValueBase->type() == comp()->refine<DebugCompilation>()->_pDebugValue);
    return bx()->LoadFieldAt(PASSLOC, b, comp()->refine<DebugCompilation>()->lookupTypeField(type), debugValueBase);
}

uint64_t
DebugContext::index(const Symbol *symbol) const {
   return symbol->id();
}

uint64_t
DebugContext::index(const Value *value) const {
   return value->id();
}

} // namespace Debug
} // namespace JB2
} // namespace OMR

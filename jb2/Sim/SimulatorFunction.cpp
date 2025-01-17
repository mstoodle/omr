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

#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "Debug/DebuggerFunction.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

DebuggerFunction::DebuggerFunction(LOCATION, Debugger *dbgr, Base::FunctionCompilation *compToDebug)
    : Base::Function(PASSLOC, compToDebug->compiler())
    , _debugger(dbgr)
    , _base(compToDebug->compiler()->lookupExtension<Base::BaseExtension>())
    , _debugDictionary(new DebugDictionary(compToDebug, dbgr->getDictionary(compToDebug->func())))
    , _comp(compToDebug) {
}

DebuggerFunction::DebuggerFunction(LOCATION, Debugger *dbgr, Base::FunctionCompilation *compToDebug, DebugDictionary *dict)
    : Base::Function(PASSLOC, compToDebug->compiler())
    , _base(compToDebug->compiler()->lookupExtension<Base::BaseExtension>())
    , _debugger(dbgr)
    , _debugDictionary(dict) {
}

bool
DebuggerFunction::buildContext(LOCATION, Base::FunctionCompilation *comp, Base::FunctionContext *fc) {
    DebugDictionary *dict = dbgDict();

    _DebugValue = dict->_DebugValue;
    _pDebugValue = dict->_pDebugValue;
    _DebugValue_type = dict->_DebugValue_type;
    _DebugValue_fields = &dict->_DebugValue_fields;

    _DebugFrame = dict->_DebugFrame;
    _pDebugFrame = dict->_pDebugFrame;
    _DebugFrame_info = dict->_DebugFrame_info;
    _DebugFrame_debugger = dict->_DebugFrame_debugger;
    _DebugFrame_locals = dict->_DebugFrame_locals;
    _DebugFrame_values = dict->_DebugFrame_values;
    _DebugFrame_returnValues = dict->_DebugFrame_returnValues;
    _DebugFrame_fromBuilder = dict->_DebugFrame_fromBuilder;
    _DebugFrame_returning = dict->_DebugFrame_returning;
    _DebugFrame_builderToDebug = dict->_DebugFrame_builderToDebug;

    return true;
}

void
DebuggerFunction::storeValue(LOCATION, Base::FunctionContext *fc, Builder *b, Symbol *local, Value *value) {
    LocalSymbol *localsSym = fc->LookupLocal("locals");
    storeToDebugValue(PASSLOC, b, _base->IndexAt(PASSLOC, b, _base->Load(PASSLOC, b, localsSym), _base->ConstInt64(PASSLOC, b, _debugger->index(local))), value);
}

void
DebuggerFunction::storeValue(LOCATION, Base::FunctionContext *fc, Builder *b, Value *destValue, Value *value) {
    LocalSymbol *valuesSym = fc->LookupLocal("values");
    storeToDebugValue(PASSLOC, b, _base->IndexAt(PASSLOC, b, _base->Load(PASSLOC, b, valuesSym), _base->ConstInt64(PASSLOC, b, _debugger->index(destValue))), value);
}

void
DebuggerFunction::storeReturnValue(LOCATION, Base::FunctionContext *fc, Builder *b, int32_t resultIdx, Value *value) {
    LocalSymbol *frameSym = fc->LookupLocal("frame");
    storeToDebugValue(PASSLOC, b, _base->IndexAt(PASSLOC, b, _base->LoadFieldAt(PASSLOC, b, _DebugFrame_returnValues, _base->Load(PASSLOC, b, frameSym)), _base->ConstInt64(PASSLOC, b, resultIdx)), value);
}

Value *
DebuggerFunction::loadValue(LOCATION, Base::FunctionContext *fc, Builder *b, Symbol *local) {
    LocalSymbol *localsSym = fc->LookupLocal("locals");
    return loadFromDebugValue(PASSLOC, b, _base->IndexAt(PASSLOC, b, _base->Load(PASSLOC, b, localsSym), _base->ConstInt64(PASSLOC, b, _debugger->index(local))), local->type());
}

Value *
DebuggerFunction::loadValue(LOCATION, Base::FunctionContext *fc, Builder *b, Value *value) {
    LocalSymbol *valuesSym = fc->LookupLocal("values");
    return loadFromDebugValue(PASSLOC, b, _base->IndexAt(PASSLOC, b, _base->Load(PASSLOC, b, valuesSym), _base->ConstInt64(PASSLOC, b, _debugger->index(value))), value->type());
}

void
DebuggerFunction::storeToDebugValue(LOCATION, Builder *b, Value *debugValue, Value *value) {
    const Type *type = value->type();
    _base->StoreFieldAt(PASSLOC, b, _DebugValue_type, debugValue, _base->ConstInt64(PASSLOC, b, (int64_t)type));
    _base->StoreFieldAt(PASSLOC, b, lookupTypeField(type), debugValue, value);
}

Value *
DebuggerFunction::loadFromDebugValue(LOCATION, Builder *b, Value *debugValueBase, const Type *type) {
    assert(debugValueBase->type() == _pDebugValue);
    return _base->LoadFieldAt(PASSLOC, b, lookupTypeField(type), debugValueBase);
}

const FieldType *
DebuggerFunction::lookupTypeField(const Type *type) {
    auto it = _DebugValue_fields->find(type);
    assert(it != _DebugValue_fields->end());
    const Base::FieldType *typeField = it->second;
    return typeField;
}

} // namespace Debug
} // namespace JB2
} // namespace OMR

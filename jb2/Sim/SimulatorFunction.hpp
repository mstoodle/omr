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

#ifndef SIMULATORFUNCTION_INCL
#define SIMULATORFUNCTION_INCL

#include <map>
#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"

namespace OMR {
namespace JB2 {
namespace Sim {

class DebugDictionary
class Debugger;

class SimulatorFunction : public Base::Function {
public:

    SimulatorFunction(LOCATION, Debugger *dbgr, Base::FunctionCompilation *compToDebug);
    SimulatorFunction(LOCATION, Debugger *dbgr, Base::FunctionCompilation *compToDebug, DebugDictionary *types);

protected:
    DebugDictionary *dbgDict() { return _debugDictionary; }

    void initialize(DebugDictionary *types);

    virtual bool buildContext(LOCATION, Base::FunctionCompilation *comp, Base::FunctionContext *fc);

    void storeValue(LOCATION, Base::FunctionContext *fc, Builder *b, Symbol *local, Value *value);
    void storeValue(LOCATION, Base::FunctionContext *fc, Builder *b, Value *debugvalue, Value *value);
    void storeReturnValue(LOCATION, Base::FunctionContext *fc, Builder *b, int32_t resultIdx, Value *value);
    Value *loadValue(LOCATION, Base::FunctionContext *fc, Builder *b, Symbol *local);
    Value *loadValue(LOCATION, Base::FunctionContext *fc, Builder *b, Value *value);
    void storeToDebugValue(LOCATION, Builder *b, Value *debugValue, Value *value);
    Value *loadFromDebugValue(LOCATION, Builder *b, Value *debugValue, const Type *type);

    const Base::FieldType *lookupTypeField(const Type *type);

    void transferToBoundBuilder(Builder *b, Builder *bound);
    void transferToUnboundBuilder(Builder *b, Builder *target);

    Debugger *_debugger;
    Base::BaseExtension *_base;
    DebugDictionary *_debugDictionary;
    Base::FunctionCompilation *_comp;
    const Base::StructType *_DebugValue;
    const Base::PointerType *_pDebugValue;
    const Base::FieldType *_DebugValue_type;
    std::map<const Type *, const Base::FieldType *> *_DebugValue_fields;

    const Base::StructType *_DebugFrame;
    const Base::PointerType *_pDebugFrame;
    const Base::FieldType *_DebugFrame_info;
    const Base::FieldType *_DebugFrame_debugger;
    const Base::FieldType *_DebugFrame_locals;
    const Base::FieldType *_DebugFrame_values;
    const Base::FieldType *_DebugFrame_returnValues;
    const Base::FieldType *_DebugFrame_fromBuilder;
    const Base::FieldType *_DebugFrame_returning;
    const Base::FieldType *_DebugFrame_builderToDebug;
};

} // namespace Sim
} // namespace JB2
} // namespace OMR

#endif // defined(SIMULATORFUNCTION_INCL)

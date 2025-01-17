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
#include "Debug/DebugCompilation.hpp"
#include "Debug/DebugExtension.hpp"
#include "Debug/DebugValue.hpp"
#include "Debug/DebuggerFrame.hpp"

namespace OMR {
namespace JB2 {
namespace Debug{


INIT_JBALLOC_REUSECAT(DebugCompilation, Compilation)
SUBCLASS_KINDSERVICE_IMPL(DebugCompilation,"DebugCompilation",Func::FunctionCompilation,Extensible)

class DebugValueStructBuilder : public Base::StructTypeBuilder {
public:
    DebugValueStructBuilder(Base::BaseExtension *bx, Compilation *comp, Compilation *compToDebug)
        : Base::StructTypeBuilder(bx, comp)
        , _bx(bx)
        , _typedict(compToDebug->ir()->typedict()) {

    }

    virtual void innerCreate(const Base::StructType *sType) {
        addField("_type", _bx->Int64, 8*offsetof(DebugValue, _type));
        for (auto typeIt = _typedict->typesIterator(); typeIt.hasItem(); typeIt++) {
            const Type *type = typeIt.item();
            if (type->size() > 0) { // && !type->isKind<Base::FieldType>()) {
                // this makes no sense to me :(
                //const Base::FieldType *myType = _typedict->LookupType(type->id())->refine<const Base::FieldType>();
                // special typeString Literal will be handled correctly by TypeReplacer
                // otherwise user defined types may not be handled properly
                size_t offset = 8*offsetof(DebugValue, _firstValueData);
                if (type->isKind<Base::FieldType>()) // right?
                    offset += type->refine<Base::FieldType>()->offset();
                addField(type->name(), type, offset);
                //Literal *typeName = Literal::create(this, myType);
                //_DebugValue_fields.insert({type, DefineField(_DebugValue, typeName, myType, 8*offsetof(DebugValue, _firstValueData))});
            }
        }
    }

protected:
    Base::BaseExtension *_bx;
    TypeDictionary *_typedict;
};

DebugCompilation::DebugCompilation(Allocator *a, DebugExtension *dbx, KINDTYPE(Extensible) kind, Func::Function *func, Compilation *compToDebug, StrategyID strat, Config *localConfig)
    : Func::FunctionCompilation(a, dbx, kind, func, strat, localConfig)
    , _compToDebug(compToDebug)
    , _bx(dbx->bx()) {

    TypeDictionary *tdToDebug = compToDebug->ir()->typedict();

    // first, determine largest Type in the dictionary
    size_t debugValueSizeInBytes = 0;
    for (auto typeIt = tdToDebug->typesIterator(); typeIt.hasItem(); typeIt++) {
        const Type *type = typeIt.item();
        size_t size = type->size();
        if (debugValueSizeInBytes < size)
            debugValueSizeInBytes = size;
    }
    debugValueSizeInBytes = sizeof(DebugValue) - sizeof(uintptr_t) + debugValueSizeInBytes / 8;

    DebugValueStructBuilder debugValueBuilder(bx(), this, compToDebug);
    _DebugValue = (&debugValueBuilder)->setName("DebugValue")
                                      ->setSize(8*debugValueSizeInBytes)
                                      ->create(LOC);
    _pDebugValue = bx()->PointerTo(LOC, compToDebug, _DebugValue);
    for (auto typeIt = tdToDebug->typesIterator(); typeIt.hasItem(); typeIt++) {
        const Type *type = typeIt.item();
        if (type->size() > 0) { // && !type->isKind<Base::FieldType>()) {
            const Type *myType = ir()->typedict()->LookupType(type->id());
            const Base::FieldType *ft = _DebugValue->LookupField(myType->name());
            _DebugValue_fields.insert({type, ft});
        }
    }

    Base::StructTypeBuilder frameBuilder(bx(), compToDebug);
    _DebugFrame = (&frameBuilder)->setName("DebugFrame")
                                 ->setSize(8*sizeof(struct DebuggerFrame))
                                 ->addField("_info", bx()->Address, 8*offsetof(struct DebuggerFrame, _info))
                                 ->addField("_debugger", bx()->Address, 8*offsetof(struct DebuggerFrame, _debugger))
                                 ->addField("_locals", _pDebugValue, 8*offsetof(struct DebuggerFrame, _locals))
                                 ->addField("_values", _pDebugValue, 8*offsetof(struct DebuggerFrame, _values))
                                 ->addField("_returnValues", _pDebugValue, 8*offsetof(struct DebuggerFrame, _returnValues))
                                 ->addField("_fromBuilder", bx()->Address, 8*offsetof(struct DebuggerFrame, _fromBuilder))
                                 ->addField("_returning", bx()->Address, 8*offsetof(struct DebuggerFrame, _returning))
                                 ->addField("_builderToDebug", bx()->Address, 8*offsetof(struct DebuggerFrame, _builderToDebug))
                                 ->create(LOC);
    _pDebugFrame = bx()->PointerTo(LOC, compToDebug, _DebugFrame);

    _DebugFrame_info = _DebugFrame->LookupField("_info");
    _DebugFrame_debugger = _DebugFrame->LookupField("_debugger");
    _DebugFrame_locals = _DebugFrame->LookupField("_locals");
    _DebugFrame_values = _DebugFrame->LookupField("_values");
    _DebugFrame_returnValues = _DebugFrame->LookupField("_returnValues");
    _DebugFrame_fromBuilder = _DebugFrame->LookupField("_fromBuilder");
    _DebugFrame_returning = _DebugFrame->LookupField("_returning");
    _DebugFrame_builderToDebug = _DebugFrame->LookupField("_builderToDebug");
}

} // namespace Debug
} // namespace JB2
} // namespace OMR

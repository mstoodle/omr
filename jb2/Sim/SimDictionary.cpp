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
#include "Debug/DebugDictionary.hpp"
#include "Debug/DebuggerFrame.hpp"
#include "Debug/DebugValue.hpp"

namespace OMR {
namespace JB2 {
namespace Debug{


DebugDictionary::DebugDictionary(Base::FunctionCompilation *compToDebug)
    : TypeDictionary(compToDebug->compiler(), compToDebug->typedict()->name() + "_DBG", compToDebug->typedict())
    , _base(_compiler->lookupExtension<Base::BaseExtension>()) {

    createTypes(compToDebug);
}

DebugDictionary::DebugDictionary(Base::FunctionCompilation *compToDebug, DebugDictionary *baseDict)
    : TypeDictionary(compToDebug->compiler(), compToDebug->typedict()->name() + "_DBG", baseDict)
    , _base(_compiler->lookupExtension<Base::BaseExtension>()) {

    initTypes(baseDict);
}

class DebugValueStructBuilder : public Base::StructTypeBuilder {
public:
    DebugValueStructBuilder(DebugDictionary *debugDict, Base::BaseExtension *base, Base::FunctionCompilation *comp)
        : Base::StructTypeBuilder(base, comp)
        , _debugDict(debugDict) {

    }

    virtual void innerCreate(const Base::StructType *sType) {
        addField("_type", _ext->Int64, 8*offsetof(DebugValue, _type));
        for (auto typeIt = _debugDict->TypesBegin(); typeIt != _debugDict->TypesEnd(); typeIt++) {
            const Type *type = *typeIt;
            if (type->size() > 0 && !type->isKind<Base::FieldType>()) {
                const Base::FieldType *myType = _debugDict->LookupType(type->id())->refine<const Base::FieldType>();
                // special typeString Literal will be handled correctly by TypeReplacer
                // otherwise user defined types may not be handled properly
                addField(myType->name(), myType->type(), 8*offsetof(DebugValue, _firstValueData));
                //Literal *typeName = Literal::create(this, myType);
                //_DebugValue_fields.insert({type, DefineField(_DebugValue, typeName, myType, 8*offsetof(DebugValue, _firstValueData))});
            }
        }
    }

protected:
    DebugDictionary *_debugDict;
};

void
DebugDictionary::createTypes(Base::FunctionCompilation *compToDebug) {
    TypeDictionary *tdToDebug = compToDebug->typedict();
    size_t sizeDebugValue = 0;
    for (auto typeIt = tdToDebug->TypesBegin(); typeIt != tdToDebug->TypesEnd(); typeIt++) {
        const Type *type = *typeIt;
        size_t size = type->size();
        if (sizeDebugValue < size)
            sizeDebugValue = size;
    }
    sizeDebugValue = sizeof(DebugValue) - sizeof(uintptr_t) + sizeDebugValue / 8;

    DebugValueStructBuilder valueBuilder(this, _base, compToDebug);
    _DebugValue = (&valueBuilder)->setName("DebugValue")
                                 ->create(LOC);
    _pDebugValue = _base->PointerTo(LOC, compToDebug, _DebugValue);
    for (auto typeIt = TypesBegin(); typeIt != TypesEnd(); typeIt++) {
        const Type *type = *typeIt;
        if (type->size() > 0 && !type->isKind<Base::FieldType>()) {
            const Type *myType = LookupType(type->id());
            const Base::FieldType *ft = _DebugValue->LookupField(myType->name());
            _DebugValue_fields.insert({type, ft});
        }
    }

    Base::StructTypeBuilder frameBuilder(_base, compToDebug);
    _DebugFrame = (&frameBuilder)->setName("DebugFrame")
                                 ->setSize(8*sizeof(struct DebuggerFrame))
                                 ->addField("_info", _base->Address, 8*offsetof(struct DebuggerFrame, _info))
                                 ->addField("_debugger", _base->Address, 8*offsetof(struct DebuggerFrame, _debugger))
                                 ->addField("_locals", _pDebugValue, 8*offsetof(struct DebuggerFrame, _locals))
                                 ->addField("_values", _pDebugValue, 8*offsetof(struct DebuggerFrame, _values))
                                 ->addField("_returnValues", _pDebugValue, 8*offsetof(struct DebuggerFrame, _returnValues))
                                 ->addField("_fromBuilder", _base->Address, 8*offsetof(struct DebuggerFrame, _fromBuilder))
                                 ->addField("_returning", _base->Address, 8*offsetof(struct DebuggerFrame, _returning))
                                 ->addField("_builderToDebug", _base->Address, 8*offsetof(struct DebuggerFrame, _builderToDebug))
                                 ->create(LOC);
    _pDebugFrame = _base->PointerTo(LOC, compToDebug, _DebugFrame);

    _DebugFrame_info = _DebugFrame->LookupField("_info");
    _DebugFrame_debugger = _DebugFrame->LookupField("_debugger");
    _DebugFrame_locals = _DebugFrame->LookupField("_locals");
    _DebugFrame_values = _DebugFrame->LookupField("_values");
    _DebugFrame_returnValues = _DebugFrame->LookupField("_returnValues");
    _DebugFrame_fromBuilder = _DebugFrame->LookupField("_fromBuilder");
    _DebugFrame_returning = _DebugFrame->LookupField("_returning");
    _DebugFrame_builderToDebug = _DebugFrame->LookupField("_builderToDebug");
}

void
DebugDictionary::initTypes(DebugDictionary *baseDict) {
    _DebugValue =                baseDict->_DebugValue;
    _DebugValue_type =           baseDict->_DebugValue_type;
    _DebugValue_fields =         baseDict->_DebugValue_fields;
    _pDebugValue =               baseDict->_pDebugValue;
    _DebugFrame =                baseDict->_DebugFrame;
    _DebugFrame_info =           baseDict->_DebugFrame_info;
    _DebugFrame_debugger =       baseDict->_DebugFrame_debugger;
    _DebugFrame_locals =         baseDict->_DebugFrame_locals;
    _DebugFrame_values =         baseDict->_DebugFrame_values;
    _DebugFrame_returnValues =   baseDict->_DebugFrame_returnValues;
    _DebugFrame_fromBuilder =    baseDict->_DebugFrame_fromBuilder;
    _DebugFrame_returning =      baseDict->_DebugFrame_returning;
    _DebugFrame_builderToDebug = baseDict->_DebugFrame_builderToDebug;
    _pDebugFrame =               baseDict->_pDebugFrame;
}

} // namespace Debug
} // namespace JB2
} // namespace OMR

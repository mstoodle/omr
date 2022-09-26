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
#include "Compilation.hpp"
#include "Config.hpp"
#include "JB1MethodBuilder.hpp"
#include "Literal.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "Symbol.hpp"
#include "TextWriter.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

#include "ilgen/BytecodeBuilder.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/IlType.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "ilgen/VirtualMachineState.hpp"

namespace OMR {
namespace JitBuilder {

JB1MethodBuilder::JB1MethodBuilder(Compilation *comp)
    : Loggable()
    , _comp(comp)
    , _mb(NULL)
    , _entryPoint(NULL)
    , _compileReturnCode(0) {
    setTraceEnabled(comp->config()->traceCodeGenerator());
}

JB1MethodBuilder::~JB1MethodBuilder() {
}

//
// Public functions
//

void
JB1MethodBuilder::registerTypes(TypeDictionary *dict) {
    TypeID numTypes = dict ->numTypes();
    std::vector<bool> mappedTypes(numTypes);
    while (numTypes > 0) {
        TypeID startNumTypes = numTypes;
        for (auto it = dict->TypesBegin(); it != dict->TypesEnd(); it++) {
            const Type *type = *it;
            if (mappedTypes[type->id()] != true) {
                bool mapped = type->registerJB1Type(this);
                if (mapped) {
                    numTypes--;
                    mappedTypes[type->id()] = true;
                }
            }
        }
        assert(numTypes < startNumTypes);
    }
}

bool
JB1MethodBuilder::typeRegistered(const Type *t) {
    return (_types.find(t->id()) != _types.end());
}

void
JB1MethodBuilder::registerNoType(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->NoType;
}

void
JB1MethodBuilder::registerInt8(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Int8;
}

void
JB1MethodBuilder::registerInt16(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Int16;
}

void
JB1MethodBuilder::registerInt32(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Int32;
}

void
JB1MethodBuilder::registerInt64(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Int64;
}

void
JB1MethodBuilder::registerFloat(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Float;
}

void
JB1MethodBuilder::registerDouble(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Double;
}

void
JB1MethodBuilder::registerAddress(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Address;
}

void
JB1MethodBuilder::registerPointer(const Type * pointerType, const Type *baseType) {
    auto found = _types.find(pointerType->id());
    if (found != _types.end()) {
        assert(found->second->baseType() == map(baseType));
        return;
    }

    assert(_types.find(baseType->id()) != _types.end());
    TR::IlType *baseIlType = _types[baseType->id()];

    TR::TypeDictionary *dict = _mb->typeDictionary();
    TR::IlType *ptrIlType = dict->PointerTo(baseIlType);

    _types[pointerType->id()] = ptrIlType;
}

void
JB1MethodBuilder::registerStruct(const Type * type) {
    auto found = _types.find(type->id());
    assert(found == _types.end());

    TR::TypeDictionary *dict = _mb->typeDictionary();
    TR::IlType *structIlType = dict->DefineStruct(findOrCreateString(type->name()));

    _types[type->id()] = structIlType;
}

void
JB1MethodBuilder::registerField(std::string structName, std::string fieldName, const Type *type, size_t offset) {
    TR::TypeDictionary *dict = _mb->typeDictionary();
    dict->DefineField(findOrCreateString(structName), findOrCreateString(fieldName), map(type), offset/8); // JB1 uses byte offsets
}

void
JB1MethodBuilder::closeStruct(std::string structName) {
    TR::TypeDictionary *dict = _mb->typeDictionary();
    dict->CloseStruct(findOrCreateString(structName));
}

void
JB1MethodBuilder::registerBuilder(const Builder * b, TR::IlBuilder *omr_b) {
    if (_builders.find(b->id()) != _builders.end())
        return;

    if (omr_b != NULL)
        _builders[b->id()] = omr_b;
    else
        b->jbgen(this);
}

void
JB1MethodBuilder::registerBytecodeBuilder(const Builder * bcb, TR::BytecodeBuilder *omr_bcb) {
    if (_bytecodeBuilders.find(bcb->id()) != _bytecodeBuilders.end())
        return;

    if (omr_bcb != NULL) {
        _bytecodeBuilders[bcb->id()] = omr_bcb;
        _builders[bcb->id()] = static_cast<TR::IlBuilder *>(omr_bcb);
    }
    else
        bcb->jbgen(this);
}

void
JB1MethodBuilder::createBuilder(const Builder * b) {
    if (_builders.find(b->id()) != _builders.end())
        return;

    TR::IlBuilder *omr_bcb = _mb->OrphanBuilder();
    _builders[b->id()] = omr_bcb;
}

void
JB1MethodBuilder::createBytecodeBuilder(const Builder * bcb, int32_t bcIndex, std::string name) {
    if (_bytecodeBuilders.find(bcb->id()) != _bytecodeBuilders.end())
        return;

    TR::BytecodeBuilder *omr_bcb = _mb->OrphanBytecodeBuilder(bcIndex, findOrCreateString(name));
    TR::VirtualMachineState *vmState = new TR::VirtualMachineState(); // just need an empty state so OMR can propagate it
    omr_bcb->setVMState(vmState);
    _bytecodeBuilders[bcb->id()] = omr_bcb;
    _builders[bcb->id()] = static_cast<TR::IlBuilder *>(omr_bcb);
}

void
JB1MethodBuilder::addFallThroughBuilder(const Builder *bcb, const Builder * ftbcb) {
    TR::BytecodeBuilder *omr_bcb = mapBytecodeBuilder(bcb);
    assert(omr_bcb);

    TR::BytecodeBuilder *omr_ftbcb = mapBytecodeBuilder(ftbcb);
    assert(omr_ftbcb);

    omr_bcb->AddFallThroughBuilder(omr_ftbcb);
}

void
JB1MethodBuilder::addSuccessorBuilder(const Builder *bcb, const Builder * sbcb) {
    TR::BytecodeBuilder *omr_bcb = mapBytecodeBuilder(bcb);
    assert(omr_bcb);

    TR::BytecodeBuilder *omr_sbcb = mapBytecodeBuilder(sbcb);
    assert(omr_sbcb);

    omr_bcb->AddSuccessorBuilder(&omr_sbcb);
    _bytecodeBuilders[sbcb->id()] = omr_sbcb; // AddSuccesorBuilder may have changed it
    _builders[sbcb->id()] = static_cast<TR::IlBuilder *>(omr_sbcb);
}

void
JB1MethodBuilder::FunctionName(std::string name) {
    _mb->DefineName(findOrCreateString(name));
}

void
JB1MethodBuilder::FunctionFile(std::string file) {
    _mb->DefineFile(findOrCreateString(file));
}

void
JB1MethodBuilder::FunctionLine(std::string line) {
    _mb->DefineLine(findOrCreateString(line));
}

void
JB1MethodBuilder::FunctionReturnType(const Type *type) {
    _mb->DefineReturnType(map(type));
}

void
JB1MethodBuilder::Parameter(std::string name, const Type * type) {
    _mb->DefineParameter(findOrCreateString(name), map(type));
}

void
JB1MethodBuilder::Local(std::string name, const Type * type) {
    _mb->DefineLocal(findOrCreateString(name), map(type));
}

void
JB1MethodBuilder::DefineFunction(std::string name,
                             std::string fileName,
                             std::string lineNumber,
                             void *entryPoint,
                             const Type * returnType,
                             int32_t numParms,
                             const Type **parmTypes) {

    TR::IlType **omr_parmTypes = new TR::IlType *[numParms];
    for (int p=0;p < numParms;p++)
        omr_parmTypes[p] = map(parmTypes[p]);                                    
    _mb->DefineFunction(findOrCreateString(name),
                        findOrCreateString(fileName),
                        findOrCreateString(lineNumber),
                        entryPoint,
                        map(returnType),
                        numParms, omr_parmTypes);
}

void
JB1MethodBuilder::ConstInt8(Location *loc, Builder *b, Value *result, int8_t v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstInt8(v));
}

void
JB1MethodBuilder::ConstInt16(Location *loc, Builder *b, Value *result, int16_t v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstInt16(v));
}

void
JB1MethodBuilder::ConstInt32(Location *loc, Builder *b, Value *result, int32_t v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstInt32(v));
}

void
JB1MethodBuilder::ConstInt64(Location *loc, Builder *b, Value *result, int64_t v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstInt64(v));
}

void
JB1MethodBuilder::ConstFloat(Location *loc, Builder *b, Value *result, float v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstFloat(v));
}

void
JB1MethodBuilder::ConstDouble(Location *loc, Builder *b, Value *result, double v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstDouble(v));
}

void
JB1MethodBuilder::ConstAddress(Location *loc, Builder *b, Value *result, const void * v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstAddress(v));
}

void
JB1MethodBuilder::Add(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->Add(map(left), map(right)));
}

void
JB1MethodBuilder::ConvertTo(Location *loc, Builder *b, Value *result, const Type *type, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConvertTo(map(type), map(value)));
}

void
JB1MethodBuilder::Mul(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->Mul(map(left), map(right)));
}

void
JB1MethodBuilder::Sub(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->Sub(map(left), map(right)));
}

void
JB1MethodBuilder::EntryPoint(Builder *entryBuilder) {
    TR::IlBuilder *omr_b = map(entryBuilder);
    _mb->AppendBuilder(omr_b);
}

void
JB1MethodBuilder::Call(Location *loc, Builder *b, Value *result, std::string targetName, std::vector<Value *> arguments) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    const char *function = findOrCreateString(targetName);
    TR::IlValue **omr_args = new TR::IlValue *[arguments.size()];
    for (auto a=0;a < arguments.size();a++)
        omr_args[a] = map(arguments[a]);
    registerValue(result, omr_b->Call(function, arguments.size(), omr_args));
}

void
JB1MethodBuilder::Call(Location *loc, Builder *b, std::string targetName, std::vector<Value *> arguments) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    const char *function = findOrCreateString(targetName);
    TR::IlValue **omr_args = new TR::IlValue *[arguments.size()];
    for (auto a=0;a < arguments.size();a++)
        omr_args[a] = map(arguments[a]);
    TR::IlValue *omr_rv = omr_b->Call(function, arguments.size(), omr_args);
    assert(omr_rv == NULL);
}

void
JB1MethodBuilder::ForLoopUp(Location *loc,
                           Builder *b,
                           Symbol *loopVariable,
                           Value *initial,
                           Value *final,
                           Value *bump,
                           Builder *loopBody,
                           Builder *loopBreak,
                           Builder *loopContinue) {

    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    TR::IlBuilder *omr_loopBody = map(loopBody);
    TR::IlBuilder *omr_loopBreak = NULL;
    TR::IlBuilder *omr_loopContinue = NULL;
    omr_b->ForLoop(true, findOrCreateString(loopVariable->name()), &omr_loopBody, &omr_loopBreak, &omr_loopContinue, map(initial), map(final), map(bump));
    if (loopBreak != NULL)
        registerBuilder(loopBreak, omr_loopBreak);
    if (loopContinue != NULL)
        registerBuilder(loopContinue, omr_loopContinue);
}

void
JB1MethodBuilder::Goto(Location *loc, Builder *b, Builder *target) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->Goto(map(target));
}

void
JB1MethodBuilder::IfCmpEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpEqual(map(target), map(left), map(right));
}

void
JB1MethodBuilder::IfCmpEqualZero(Location *loc, Builder *b, Builder *target, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpEqualZero(map(target), map(value));
}

void
JB1MethodBuilder::IfCmpGreaterThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpGreaterThan(map(target), map(left), map(right));
}

void
JB1MethodBuilder::IfCmpGreaterOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpGreaterOrEqual(map(target), map(left), map(right));
}

void
JB1MethodBuilder::IfCmpLessThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpLessThan(map(target), map(left), map(right));
}

void
JB1MethodBuilder::IfCmpLessOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpLessOrEqual(map(target), map(left), map(right));
}

void
JB1MethodBuilder::IfCmpNotEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpNotEqual(map(target), map(left), map(right));
}

void
JB1MethodBuilder::IfCmpNotEqualZero(Location *loc, Builder *b, Builder *target, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpNotEqualZero(map(target), map(value));
}

void
JB1MethodBuilder::IfCmpUnsignedGreaterThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpUnsignedGreaterThan(map(target), map(left), map(right));
}

void
JB1MethodBuilder::IfCmpUnsignedGreaterOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpUnsignedGreaterOrEqual(map(target), map(left), map(right));
}

void
JB1MethodBuilder::IfCmpUnsignedLessThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpUnsignedLessThan(map(target), map(left), map(right));
}

void
JB1MethodBuilder::IfCmpUnsignedLessOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfCmpUnsignedLessOrEqual(map(target), map(left), map(right));
}

void
JB1MethodBuilder::Return(Location *loc, Builder *b) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->Return();
}

void
JB1MethodBuilder::Return(Location *loc, Builder *b, Value *v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (v != NULL)
        omr_b->Return(map(v));
    else
        omr_b->Return();
}

void
JB1MethodBuilder::Load(Location *loc, Builder *b, Value *result, Symbol *sym) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->Load(findOrCreateString(sym->name())));
}

void
JB1MethodBuilder::Store(Location *loc, Builder *b, Symbol *sym, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->Store(findOrCreateString(sym->name()), map(value));
}

void
JB1MethodBuilder::LoadAt(Location *loc, Builder *b, Value *result, Value *ptrValue) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->LoadAt(map(ptrValue->type()), map(ptrValue)));
}

void
JB1MethodBuilder::StoreAt(Location *loc, Builder *b, Value *ptrValue, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->StoreAt(map(ptrValue), map(value));
}

void
JB1MethodBuilder::LoadIndirect(Location *loc, Builder *b, Value *result, std::string structName, std::string fieldName, Value *pStruct) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->LoadIndirect(findOrCreateString(structName), findOrCreateString(fieldName), map(pStruct)));
}

void
JB1MethodBuilder::StoreIndirect(Location *loc, Builder *b, std::string structName, std::string fieldName, Value *pStruct, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->StoreIndirect(findOrCreateString(structName), findOrCreateString(fieldName), map(pStruct), map(value));
}

void
JB1MethodBuilder::StoreOver(Location *loc, Builder *b, Value *target, Value *source) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->StoreOver(map(target), map(source));
}

void
JB1MethodBuilder::CreateLocalArray(Location *loc, Builder *b, Value *result, Literal *numElements, const Type *elementType) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    int32_t numElems = numElements->getInteger();
    registerValue(result, omr_b->CreateLocalArray(numElems, map(elementType)));
}

void
JB1MethodBuilder::CreateLocalStruct(Location *loc, Builder *b, Value *result, const Type * structType) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->CreateLocalStruct(map(structType)));
}

void
JB1MethodBuilder::IndexAt(Location *loc, Builder *b, Value *result, Value *base, Value *index) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    TR::IlType *ptrType = map(base->type());
    registerValue(result, omr_b->IndexAt(ptrType, map(base), map(index)));
}


//
// internal functions
//

char *
JB1MethodBuilder::findOrCreateString(std::string str) {
    if (_strings.find(str) != _strings.end())
        return _strings[str];

    char *s = new char[str.length()+1];
    strcpy(s, str.c_str());
    _strings[str] = s;
    return s;
}

void
JB1MethodBuilder::registerValue(const Value * v, TR::IlValue *omr_v) {
    _values[v->id()] = omr_v;
}

TR::IlBuilder *
JB1MethodBuilder::map(const Builder * b, bool checkNull) {
    if (b == NULL) {
        assert(!checkNull);
        return NULL;
    }
        
    if (_builders.find(b->id()) == _builders.end())
        registerBuilder(b);
    TR::IlBuilder *omr_b = _builders[b->id()];
    if (checkNull)
        assert(omr_b);
    return omr_b;
}

TR::BytecodeBuilder *
JB1MethodBuilder::mapBytecodeBuilder(const Builder * bcb, bool checkNull) {
    if (bcb == NULL) {
        assert(!checkNull);
        return NULL;
    }
        
    if (_bytecodeBuilders.find(bcb->id()) == _bytecodeBuilders.end())
        registerBytecodeBuilder(bcb);
    TR::BytecodeBuilder *omr_bcb = _bytecodeBuilders[bcb->id()];
    if (checkNull)
        assert(omr_bcb);
    return omr_bcb;
}

TR::IlValue *
JB1MethodBuilder::map(const Value * v) {
    assert(_values.find(v->id()) != _values.end());
    TR::IlValue *omr_v = _values[v->id()];
    assert(omr_v != NULL);
    return omr_v;
}

TR::IlType *
JB1MethodBuilder::map(const Type * t) {
    assert(_types.find(t->id()) != _types.end());
    TR::IlType *omr_type = _types[t->id()];
    return omr_type;
}

#if 0
// mapStructFields is designed to iterate (recursively) through the fields of a struct/union,
// defining the fields of that struct in the `types` dictionary. If a field has a struct or
// union type, then the fields of that struct/union are also walked and added directly to
// *this* structure. That means "inner" structs/unions are inlined into the current struct.
//
// structName/fieldName are the names for the entire structure (field names are assembled to
//     include containing struct/union names)
// sType/fType are the struct and field types for this specific struct
// As mapStructFields recursively visits inner types, structName/fieldName will stay the same,
//     but sType/fType will reflect each inner type in succession
TR::IlType *
JBCodeGenerator::registerStruct(TR::TypeDictionary * types, StructType * sType, char *structName, std::string fNamePrefix, size_t baseOffset) {
    for (auto fIt = sType->FieldsBegin(); fIt != sType->FieldsEnd(); fIt++) {
        FieldType *fType = fIt->second;
        std::string fieldName = fNamePrefix + fType->name();
        const char *fieldString = findOrCreateString(fieldName);
        size_t fieldOffset = baseOffset + fType->offset();

        if (fType->isStruct() || fType->isUnion()) {
            // define a "dummy" field corresponding to the struct field itself, so we can ask for its address easily
            // in case this field's struct needs to be passed to anything
            _mb->typeDictionary()->DefineField(structName, fieldString, types->NoType, fieldOffset/8);
            StructType *innerStructType = static_cast<StructType *>(fType->type());
            mapStructFields(types, innerStructType, structName, fieldName + ".", fieldOffset);
        }
        else
            _mb->typeDictionary()->DefineField(structName, fieldString, mapType(fType->type()), fieldOffset/8); // JB1 uses bytes offsets
    }
    return mapType(sType);
}

#define MAP_CASE(omr_b,c) (reinterpret_cast<TR::IlBuilder::JBCase *>(mapCase((omr_b),(c))))

void *
JBCodeGenerator::mapCase(TR::IlBuilder *omr_b, Case *c) {
    if (_cases.find(c->id()) == _cases.end()) {
        TR::IlBuilder *omr_target = mapBuilder(c->builder());
        TR::IlBuilder::JBCase *jbCase = omr_b->MakeCase(c->value(), &omr_target, c->fallsThrough());
        _cases[c->id()] = jbCase;
        return jbCase;
    }
    return _cases[c->id()];
}
#endif

void
JB1MethodBuilder::printAllMaps() {
    TextWriter *pLog = _comp->logger(traceEnabled());
    if (pLog) {
        TextWriter & log = *pLog;

        log << "[ printAllMaps" << log.endl();
        log.indentIn();

        log.indent() << "[ Builders" << log.endl();
        log.indentIn();
        for (auto builderIt = _builders.cbegin(); builderIt != _builders.cend(); builderIt++) {
            log.indent() << "[ builder " << builderIt->first << " -> TR::IlBuilder " << (int64_t *)(void *) builderIt->second << " ]" << log.endl();
        }
        log.indentOut();
        log.indent() << "]" << log.endl();

        log.indent() << "[ Values" << log.endl();
        log.indentIn();
        for (auto valueIt = _values.cbegin(); valueIt != _values.cend(); valueIt++) {
            log.indent() << "[ value " << valueIt->first << " -> TR::IlValue " << (int64_t *)(void *) valueIt->second << " ]" << log.endl();
        }
        log.indentOut();
        log.indent() << "]" << log.endl();

        log.indent() << "[ Types" << log.endl();
        log.indentIn();
        for (auto typeIt = _types.cbegin(); typeIt != _types.cend(); typeIt++) {
            log.indent() << "[ type " << typeIt->first << " -> TR::IlType " << (int64_t *)(void *) typeIt->second << " ]" << log.endl();
        }
        log.indentOut();
        log.indent() << "]" << log.endl();

        log.indentOut();
        log.indent() << "]" << log.endl();
    }
}

#if 0
// some of this may be handy so keep it at hand during migration

void
JBCodeGenerator::generateFunctionAPI(FunctionBuilder *fb) {
    TextWriter *log = fb->logger(traceEnabled());
    if (log) log->indent() << "JBCodeGenerator::generateFunctionAPI F" << fb->id() << log->endl();

    TR::TypeDictionary * typesJB1 = _mb->typeDictionary();
    _typeDictionaries[types->id()] = typesJB1;

    TypeDictionary *dict = _comp->dict();
    TypeID numTypes = _comp->numTypes();
    while (numTypes > 0) {
        TypeID startNumTypes = numTypes;
        for (auto it = dict->TypesBegin(); it !+ dict->TypeEnd(); it++) {
            Type *type = *it;
            if (_types[type->id()] == NULL) {
                bool mapped = type->registerJB1Type(this)
                if (mapped) {
                    assert(_types[type->id()] != NULL);
                    numTypes--;
                }
            }
        }
        assert(numTypes < startNumTypes);
    }

#if 0
    if (log) log->indent() << "First pass:" << log->endl();
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
        if (log) {
            log->indent();
            log->writeType(type, true);
            log << log->endl();
        }
        if (type->isStruct() || type->isUnion()) {
            char *name = findOrCreateString(type->name());
            storeType(type, _mb->typeDictionary()->DefineStruct(name));
        }
        else if (type->isFunction()) {
            // function types all map to Address in JitBuilder 1.0
            storeType(type, _mb->typeDictionary()->Address);
        }
        else if (type->isPointer() || type->isField()) {
            // skip function and pointer types in this pass
        }
        else {
            // should be a primitive type; verify that the type has been mapped already
            //assert(mapType(type));
        }
    }

    // Second pass: map all Pointer types now that anything a Pointer can point to has been mapped
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
        if (type->isPointer()) {
            TR::IlType *ptrIlType = mapPointerType(typesJB1, static_cast<PointerType *>(type));
            storeType(type, ptrIlType);
        }
    }

    // all basic types should have mappings now. what remains is to define the fields of
    // structs and unions to JB1 so that fields will be mapped to appropriate offsets
    // in this process, any inner structs/unions are inlined into containing struct

    // Third pass: revisit all Structs and Unions to iterate over field types to map them
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
        if (type->isStruct()) {
            StructType *sType = static_cast<StructType *>(type);
            char * structName = findOrCreateString(sType->name());
            mapStructFields(typesJB1, sType, structName, std::string(""), 0);
            _mb->typeDictionary()->CloseStruct(structName, sType->size()/8);
        }
        else if (type->isUnion()) {
            UnionType *uType = static_cast<UnionType *>(type);
            char * structName = findOrCreateString(uType->name());
            mapStructFields(typesJB1, uType, structName, std::string(""), 0);
            _mb->typeDictionary()->CloseStruct(structName, uType->size()/8);
        }
    }

    // All types should be represented in the JB1 layer now, and mapTypes should be set up for every
    // type in TypeDictionary
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
        assert(type->isField() || mapType(type) != NULL);
    }
#endif

    _methodBuilders[_comp->id()] = _mb;
    storeBuilder(fb, _mb);

    _comp->constructJB1Function(this);
}

FunctionBuilder *
JBCodeGenerator::transformFunctionBuilder(FunctionBuilder *fb) {
    TextWriter *log = _fb->logger(traceEnabled());
    if (log) log->indent() << "JBCodeGenerator transformFunctionBuilder F" << fb->id() << log->endl();
    if (log) log->indentIn();
    return NULL;
}

void
JBCodeGenerator::ConstInt8(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstInt8(literal->value<int8_t>()));
}

void
JBCodeGenerator::ConstInt16(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstInt16(literal->value<int16_t>()));
}

void
JBCodeGenerator::ConstInt32(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstInt32(literal->value<int32_t>()));
}

void
JBCodeGenerator::ConstInt64(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstInt64(literal->value<int64_t>()));
}

void
JBCodeGenerator::ConstFloat32(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstFloat(literal->value<float>()));
}

void
JBCodeGenerator::ConstFloat64(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstDouble(literal->value<double>()));
}

void
JBCodeGenerator::ConstAddress(Location *loc, Builder *b, Value *result, Literal *lv) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    storeValue(result, omr_b->ConstAddress(literal->value<void *>()));
}

void
JBCodeGenerator::Return(Location *loc, Builder *b, Value *value) {
    TR::IlBuilder *omr_b = mapBuilder(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (value)
        omr_b->Return(mapValue(value));
    else
        omr_b->Return();
}

   Builder * b = op->parent();
   TR::IlBuilder *omr_b = mapBuilder(b);
   omr_b->setBCIndex(op->location()->bcIndex())->SetCurrentIlGenerator();
   switch (op->action())
      {
      case aCoercePointer :
         {
         TR::IlValue *object = mapValue(op->operand());
         storeValue(op->result(), mapValue(op->operand()));
         }
         break;

      case aAdd :
         storeValue(op->result(), omr_b->Add(mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aSub :
         storeValue(op->result(), omr_b->Sub(mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aMul :
         storeValue(op->result(), omr_b->Mul(mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aLoad :
         {
         Symbol *sym = op->symbol();
         if (sym->isFunction())
            {
            FunctionSymbol *fnSym = static_cast<FunctionSymbol *>(sym);
            storeValue(op->result(), omr_b->ConstAddress(reinterpret_cast<void *>(fnSym->entryPoint())));
            }
         else
            storeValue(op->result(), omr_b->Load(findOrCreateString(sym->name())));
         }
         break;

      case aLoadAt :
         {
         Value *v = op->operand();
         TR::IlValue *jb_v = mapValue(v);
         Type *t = op->type();
         TR::IlType *jb_t = mapType(op->type());
         Value *r = op->result();
         storeValue(r, omr_b->LoadAt(jb_t, jb_v));
         }
         break;

      case aLoadIndirect :
         {
         LoadIndirect *liOp = static_cast<LoadIndirect *>(op);
         FieldType *fieldType = liOp->getFieldType();
         const char *structName = findOrCreateString(fieldType->owningStruct()->name());
         const char *fieldName = findOrCreateString(fieldType->name());
         TR::IlValue *object = mapValue(op->operand());
         assert(object);
         storeValue(op->result(), omr_b->LoadIndirect(structName, fieldName, mapValue(op->operand())));
         }
         break;

      case aStore :
         omr_b->Store(findOrCreateString(op->symbol()->name()), mapValue(op->operand()));
         break;

      case aStoreAt :
         omr_b->StoreAt(mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aStoreIndirect :
         {
         StoreIndirect *siOp = static_cast<StoreIndirect *>(op);
         FieldType *fieldType = siOp->getFieldType();
         const char *structName = findOrCreateString(fieldType->owningStruct()->name());
         const char *fieldName = findOrCreateString(fieldType->name());
         TR::IlValue *object = mapValue(op->operand(0));
         assert(object);
         TR::IlValue *value = mapValue(op->operand(1));
         assert(value);
         omr_b->StoreIndirect(structName, fieldName, object, value);
         }
         break;

      case aIndexAt :
         storeValue(op->result(), omr_b->IndexAt(mapType(op->type()), mapValue(op->operand(0)), mapValue(op->operand(1))));
         break;

      case aCall :
         {
         Call *callOp = static_cast<Call *>(op);
         FunctionType *fType = static_cast<FunctionType *>(callOp->function()->type());
         TR::IlValue *args[op->numOperands()];
         args[0] = mapValue(callOp->function());
         for (int32_t a=1;a < op->numOperands();a++)
            args[a] = mapValue(callOp->operand(a));

         if (op->result() != NULL)
            storeValue(op->result(), omr_b->ComputedCall(fType->name().c_str(), callOp->numOperands(), args));
         else
            omr_b->ComputedCall(fType->name().c_str(), callOp->numOperands(), args);
         }
         break;

      case aAppendBuilder :
         omr_b->AppendBuilder(mapBuilder(op->builder()));
         break;

      case aGoto :
         omr_b->Goto(mapBuilder(op->builder()));
         break;

      case aIfCmpGreaterThan :
         omr_b->IfCmpGreaterThan(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfCmpLessThan :
         omr_b->IfCmpLessThan(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfCmpGreaterOrEqual :
         omr_b->IfCmpGreaterOrEqual(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfCmpLessOrEqual :
         omr_b->IfCmpLessOrEqual(mapBuilder(op->builder()), mapValue(op->operand(0)), mapValue(op->operand(1)));
         break;

      case aIfThenElse :
         {
         TR::IlBuilder *omr_thenB = mapBuilder(op->builder(0));
         TR::IlBuilder *omr_elseB = mapBuilder(op->builder(1));
         omr_b->IfThenElse(&omr_thenB, &omr_elseB, mapValue(op->operand()));
         break;
         }

      case aForLoop :
         {
         TR::IlBuilder *omr_body = mapBuilder(op->builder(0));
         TR::IlBuilder *omr_break = mapBuilder(op->builder(1));
         TR::IlBuilder *omr_continue = mapBuilder(op->builder(2));
         omr_b->ForLoop(static_cast<bool>(op->literal()->getInt8()),
                        findOrCreateString(op->symbol()->name()),
                        &omr_body,
                        &omr_break,
                        &omr_continue,
                        mapValue(op->operand(0)),
                        mapValue(op->operand(1)),
                        mapValue(op->operand(2)));
         break;
         }

      case aSwitch :
         {
         TR::IlBuilder::JBCase *cases[op->numCases()];
         int32_t cNum = 0;
         for (auto cIt = op->CasesBegin(); cIt != op->CasesEnd(); cIt++)
            cases[cNum++] = MAP_CASE(omr_b, *cIt);
         TR::IlBuilder *omr_defaultTarget = mapBuilder(op->builder());
         omr_b->Switch(mapValue(op->operand()), &omr_defaultTarget, op->numCases(), cases);
         break;
         }

      case aCreateLocalArray :
         storeValue(op->result(), omr_b->CreateLocalArray(op->literal(0)->getInt32(), mapType(op->type())));
         break;

      case aCreateLocalStruct :
         storeValue(op->result(), omr_b->CreateLocalStruct(mapType(op->type())));
         break;

      default :
         assert(0); // unhandled action!!
         break;
      }

   return NULL;
   }

FunctionBuilder *
JBCodeGenerator::transformFunctionBuilderAtEnd(FunctionBuilder * fb)
   {
   TextWriter *log = fb->logger(traceEnabled());
   if (log) log->indentOut();
   printAllMaps();
   return NULL;
   }
#endif

} // namespace JitBuilder
} // namespace OMR


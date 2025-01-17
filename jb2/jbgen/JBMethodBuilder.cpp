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
#include "jbgen/JBCodeGenerator.hpp"
#include "jbgen/JBCodeGeneratorExtensionAddon.hpp"
#include "jbgen/JBMethodBuilder.hpp"

#include "ilgen/BytecodeBuilder.hpp"
#include "ilgen/MethodBuilder.hpp"
#include "ilgen/IlType.hpp"
#include "ilgen/TypeDictionary.hpp"
#include "ilgen/VirtualMachineState.hpp"

namespace OMR {
namespace JB2 {
namespace jbgen {

INIT_JBALLOC_REUSECAT(JBMethodBuilder, CodeGenerator)

JBMethodBuilder::JBMethodBuilder(Compilation *comp)
    : Loggable()
    , _comp(comp)
    , _mb(NULL)
    , _entryPoint(NULL)
    , _compileReturnCode(0) {
    setTraceEnabled(comp->config()->traceCodeGenerator());
}

JBMethodBuilder::~JBMethodBuilder() {
    for (auto it=_strings.begin();it != _strings.end(); it++) {
        const char *s = it->second;
        _comp->mem()->deallocate(const_cast<char *>(s));
    }
}

//
// Public functions
//

void
JBMethodBuilder::registerTypes(TypeDictionary *dict) {
    TypeID numTypes = dict->ir()->maxTypeID() + 1;
    Allocator myMem("Type mapping", _comp->mem());
    BitVector mappedTypes(&myMem, numTypes);
    while (numTypes > 0) {
        TypeID startNumTypes = numTypes;
        for (auto it = dict->iterator(); it.hasItem(); it++) {
            const Type *type = it.item();
            if (mappedTypes.getBit(type->id()) != true) {
                CodeGeneratorForExtension *cgForExt = type->ext()->addon<JBCodeGeneratorExtensionAddon>()->cgForExtension();
                bool mapped = cgForExt->registerType(type);
                if (mapped) {
                    numTypes--;
                    mappedTypes.setBit(type->id());
                }
            }
        }
        assert(numTypes < startNumTypes);
    }
}

bool
JBMethodBuilder::typeRegistered(const Type *t) {
    return (_types.find(t->id()) != _types.end());
}

void
JBMethodBuilder::registerNoType(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->NoType;
}

void
JBMethodBuilder::registerInt8(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Int8;
}

void
JBMethodBuilder::registerInt16(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Int16;
}

void
JBMethodBuilder::registerInt32(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Int32;
}

void
JBMethodBuilder::registerInt64(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Int64;
}

void
JBMethodBuilder::registerFloat(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Float;
}

void
JBMethodBuilder::registerDouble(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Double;
}

void
JBMethodBuilder::registerAddress(const Type * t) {
    assert(_types.find(t->id()) == _types.end());
    _types[t->id()] = _mb->typeDictionary()->Address;
}

void
JBMethodBuilder::registerPointer(const Type * pointerType, const Type *baseType) {
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
JBMethodBuilder::registerStruct(const Type * type) {
    auto found = _types.find(type->id());
    assert(found == _types.end());

    TR::TypeDictionary *dict = _mb->typeDictionary();
    TR::IlType *structIlType = dict->DefineStruct(findOrCreateString(type->name()));

    _types[type->id()] = structIlType;
}

void
JBMethodBuilder::registerField(const String & structName, const String & fieldName, const Type *type, size_t offset) {
    TR::TypeDictionary *dict = _mb->typeDictionary();
    dict->DefineField(findOrCreateString(structName), findOrCreateString(fieldName), map(type), offset/8); // JB uses byte offsets
}

void
JBMethodBuilder::closeStruct(const String & structName) {
    TR::TypeDictionary *dict = _mb->typeDictionary();
    dict->CloseStruct(findOrCreateString(structName));
}

void
JBMethodBuilder::registerBuilder(const Builder * b, TR::IlBuilder *omr_b) {
    if (_builders.find(b->id()) != _builders.end())
        return;

    if (omr_b == NULL) {
        createBuilder(b);
        omr_b = map(b);
    }
    _builders[b->id()] = omr_b;
}

void
JBMethodBuilder::registerBytecodeBuilder(const Builder * bcb, TR::BytecodeBuilder *omr_bcb) {
    if (_bytecodeBuilders.find(bcb->id()) != _bytecodeBuilders.end())
        return;

    if (omr_bcb != NULL) {
        _bytecodeBuilders[bcb->id()] = omr_bcb;
        _builders[bcb->id()] = static_cast<TR::IlBuilder *>(omr_bcb);
    }
}

void
JBMethodBuilder::createBuilder(const Builder * b) {
    if (_builders.find(b->id()) != _builders.end())
        return;

    TR::IlBuilder *omr_bcb = _mb->OrphanBuilder();
    _builders[b->id()] = omr_bcb;
}

void
JBMethodBuilder::createBytecodeBuilder(const Builder * bcb, int32_t bcIndex, const String & name) {
    if (_bytecodeBuilders.find(bcb->id()) != _bytecodeBuilders.end())
        return;

    TR::BytecodeBuilder *omr_bcb = _mb->OrphanBytecodeBuilder(bcIndex, findOrCreateString(name));
    TR::VirtualMachineState prototype;
    TR::VirtualMachineState *vmState = prototype.MakeCopy();
    omr_bcb->setVMState(vmState);
    _bytecodeBuilders[bcb->id()] = omr_bcb;
    _builders[bcb->id()] = static_cast<TR::IlBuilder *>(omr_bcb);
}

void
JBMethodBuilder::addFallThroughBuilder(const Builder *bcb, const Builder * ftbcb) {
    TR::BytecodeBuilder *omr_bcb = mapBytecodeBuilder(bcb);
    assert(omr_bcb);

    TR::BytecodeBuilder *omr_ftbcb = mapBytecodeBuilder(ftbcb);
    assert(omr_ftbcb);

    omr_bcb->AddFallThroughBuilder(omr_ftbcb);
}

void
JBMethodBuilder::addSuccessorBuilder(const Builder *bcb, const Builder * sbcb) {
    TR::BytecodeBuilder *omr_bcb = mapBytecodeBuilder(bcb);
    assert(omr_bcb);

    TR::BytecodeBuilder *omr_sbcb = mapBytecodeBuilder(sbcb);
    assert(omr_sbcb);

    omr_bcb->AddSuccessorBuilder(&omr_sbcb);
    _bytecodeBuilders[sbcb->id()] = omr_sbcb; // AddSuccesorBuilder may have changed it
    _builders[sbcb->id()] = static_cast<TR::IlBuilder *>(omr_sbcb);
}

void
JBMethodBuilder::FunctionName(const String & name) {
    _mb->DefineName(findOrCreateString(name));
}

void
JBMethodBuilder::FunctionFile(const String & file) {
    _mb->DefineFile(findOrCreateString(file));
}

void
JBMethodBuilder::FunctionLine(const String & line) {
    _mb->DefineLine(findOrCreateString(line));
}

void
JBMethodBuilder::FunctionReturnType(const Type *type) {
    _mb->DefineReturnType(map(type));
}

void
JBMethodBuilder::Parameter(const String & name, const Type * type) {
    _mb->DefineParameter(findOrCreateString(name), map(type));
}

void
JBMethodBuilder::Local(const String & name, const Type * type) {
    _mb->DefineLocal(findOrCreateString(name), map(type));
}

void
JBMethodBuilder::DefineFunction(const String & name,
                                const String & fileName,
                                const String & lineNumber,
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
    delete[] omr_parmTypes;
}

void
JBMethodBuilder::ConstInt8(Location *loc, Builder *b, Value *result, int8_t v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstInt8(v));
}

void
JBMethodBuilder::ConstInt16(Location *loc, Builder *b, Value *result, int16_t v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstInt16(v));
}

void
JBMethodBuilder::ConstInt32(Location *loc, Builder *b, Value *result, int32_t v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstInt32(v));
}

void
JBMethodBuilder::ConstInt64(Location *loc, Builder *b, Value *result, int64_t v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstInt64(v));
}

void
JBMethodBuilder::ConstFloat(Location *loc, Builder *b, Value *result, float v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstFloat(v));
}

void
JBMethodBuilder::ConstDouble(Location *loc, Builder *b, Value *result, double v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstDouble(v));
}

void
JBMethodBuilder::ConstAddress(Location *loc, Builder *b, Value *result, const void * v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConstAddress(v));
}

void
JBMethodBuilder::Add(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->Add(map(left), map(right)));
}

void
JBMethodBuilder::And(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->And(map(left), map(right)));
}

void
JBMethodBuilder::ConvertTo(Location *loc, Builder *b, Value *result, const Type *type, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->ConvertTo(map(type), map(value)));
}

void
JBMethodBuilder::Div(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->Div(map(left), map(right)));
}

void
JBMethodBuilder::EqualTo(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->EqualTo(map(left), map(right)));
}

void
JBMethodBuilder::Mul(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->Mul(map(left), map(right)));
}

void
JBMethodBuilder::NotEqualTo(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->NotEqualTo(map(left), map(right)));
}

void
JBMethodBuilder::Sub(Location *loc, Builder *b, Value *result, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->Sub(map(left), map(right)));
}

void
JBMethodBuilder::EntryPoint(Builder *entryBuilder) {
    TR::IlBuilder *omr_b = map(entryBuilder);
    if (omr_b->isBytecodeBuilder())
        _mb->Goto(omr_b);
    else
        _mb->AppendBuilder(omr_b);
}

void
JBMethodBuilder::AppendBuilder(Location *loc, Builder *b, Builder *toAppend) {
    TR::IlBuilder *omr_b = map(b);
    TR::IlBuilder *omr_toAppend = map(toAppend);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->AppendBuilder(omr_toAppend);
}

void
JBMethodBuilder::Call(Location *loc, Builder *b, Value *result, const String & targetName, size_t numArgs, ValueIterator argIt) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    const char *function = findOrCreateString(targetName);
    TR::IlValue **omr_args = (numArgs > 0) ? new TR::IlValue *[numArgs] : NULL;
    for (auto a=0; argIt.hasItem(); argIt++)
        omr_args[a++] = map(argIt.item());
    registerValue(result, omr_b->Call(function, numArgs, omr_args));
    delete[] omr_args;
}

void
JBMethodBuilder::Call(Location *loc, Builder *b, const String & targetName, size_t numArgs, ValueIterator argIt) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    const char *function = findOrCreateString(targetName);
    TR::IlValue **omr_args = new TR::IlValue *[numArgs];
    for (auto a=0; argIt.hasItem(); argIt++)
        omr_args[a++] = map(argIt.item());
    TR::IlValue *omr_rv = omr_b->Call(function, numArgs, omr_args);
    assert(omr_rv == NULL);
    delete[] omr_args;
}

void
JBMethodBuilder::ForLoopUp(Location *loc,
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
    createBuilder(loopBody);
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
JBMethodBuilder::Goto(Location *loc, Builder *b, Builder *target) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->Goto(mapBytecodeBuilder(target));
    else
        omr_b->Goto(map(target));
}

void
JBMethodBuilder::IfCmpEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpEqual(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpEqual(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfCmpEqualZero(Location *loc, Builder *b, Builder *target, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpEqualZero(mapBytecodeBuilder(target), map(value));
    else
        omr_b->IfCmpEqualZero(map(target), map(value));
}

void
JBMethodBuilder::IfCmpGreaterThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpGreaterThan(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpGreaterThan(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfCmpGreaterOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpGreaterOrEqual(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpGreaterOrEqual(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfCmpLessThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpLessThan(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpLessThan(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfCmpLessOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpLessOrEqual(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpLessOrEqual(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfCmpNotEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpNotEqual(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpNotEqual(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfCmpNotEqualZero(Location *loc, Builder *b, Builder *target, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpNotEqualZero(mapBytecodeBuilder(target), map(value));
    else
        omr_b->IfCmpNotEqualZero(map(target), map(value));
}

void
JBMethodBuilder::IfCmpUnsignedGreaterThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpUnsignedGreaterThan(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpUnsignedGreaterThan(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfCmpUnsignedGreaterOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpUnsignedGreaterOrEqual(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpUnsignedGreaterOrEqual(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfCmpUnsignedLessThan(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpUnsignedLessThan(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpUnsignedLessThan(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfCmpUnsignedLessOrEqual(Location *loc, Builder *b, Builder *target, Value *left, Value *right) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (omr_b->isBytecodeBuilder())
        mapBytecodeBuilder(b)->IfCmpUnsignedLessOrEqual(mapBytecodeBuilder(target), map(left), map(right));
    else
        omr_b->IfCmpUnsignedLessOrEqual(map(target), map(left), map(right));
}

void
JBMethodBuilder::IfThen(Location *loc, Builder *b, Builder *thenPath, Value *condition) {
    TR::IlBuilder *omr_b = map(b);
    TR::IlBuilder *omr_thenPath = map(thenPath);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfThen(&omr_thenPath, map(condition));
}

void
JBMethodBuilder::IfThenElse(Location *loc, Builder *b, Builder *thenPath, Builder *elsePath, Value *selector) {
    TR::IlBuilder *omr_b = map(b);
    TR::IlBuilder *omr_thenPath = map(thenPath);
    TR::IlBuilder *omr_elsePath = map(elsePath);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->IfThenElse(&omr_thenPath, &omr_elsePath, map(selector));
}

void
JBMethodBuilder::Return(Location *loc, Builder *b) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->Return();
}

void
JBMethodBuilder::Return(Location *loc, Builder *b, Value *v) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    if (v != NULL)
        omr_b->Return(map(v));
    else
        omr_b->Return();
}

void
JBMethodBuilder::Switch(Location *loc, Builder *b, Builder *defaultBuilder, Value *selector, int32_t numCases, Literal **lvs, Builder **caseBuilders, bool *fallThroughs) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    TR::IlBuilder::JBCase *cases[numCases];
    TR::IlBuilder *builders[numCases];
    int32_t cNum = 0;
    for (int32_t cNum=0;cNum < numCases;cNum++) {
        builders[cNum] = map(caseBuilders[cNum]);
        cases[cNum] = omr_b->MakeCase(lvs[cNum]->value<int32_t>(), builders+cNum, fallThroughs[cNum]);
    }
    TR::IlBuilder *omr_defaultTarget = map(defaultBuilder);
    omr_b->Switch(map(selector), &omr_defaultTarget, numCases, cases);
}

void
JBMethodBuilder::Load(Location *loc, Builder *b, Value *result, Symbol *sym) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->Load(findOrCreateString(sym->name())));
}

void
JBMethodBuilder::Store(Location *loc, Builder *b, Symbol *sym, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->Store(findOrCreateString(sym->name()), map(value));
}

void
JBMethodBuilder::LoadAt(Location *loc, Builder *b, Value *result, Value *ptrValue) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->LoadAt(map(ptrValue->type()), map(ptrValue)));
}

void
JBMethodBuilder::StoreAt(Location *loc, Builder *b, Value *ptrValue, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->StoreAt(map(ptrValue), map(value));
}

void
JBMethodBuilder::LoadIndirect(Location *loc, Builder *b, Value *result, const String & structName, const String & fieldName, Value *pStruct) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->LoadIndirect(findOrCreateString(structName), findOrCreateString(fieldName), map(pStruct)));
}

void
JBMethodBuilder::StoreIndirect(Location *loc, Builder *b, const String & structName, const String & fieldName, Value *pStruct, Value *value) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->StoreIndirect(findOrCreateString(structName), findOrCreateString(fieldName), map(pStruct), map(value));
}

void
JBMethodBuilder::StoreOver(Location *loc, Builder *b, Value *target, Value *source) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    omr_b->StoreOver(map(target), map(source));
}

void
JBMethodBuilder::CreateLocalArray(Location *loc, Builder *b, Value *result, Literal *numElements, const Type *elementType) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    int32_t numElems = numElements->getInteger();
    registerValue(result, omr_b->CreateLocalArray(numElems, map(elementType)));
}

void
JBMethodBuilder::CreateLocalStruct(Location *loc, Builder *b, Value *result, const Type * structType) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    registerValue(result, omr_b->CreateLocalStruct(map(structType)));
}

void
JBMethodBuilder::IndexAt(Location *loc, Builder *b, Value *result, Value *base, Value *index) {
    TR::IlBuilder *omr_b = map(b);
    omr_b->setBCIndex(loc->bcIndex())->SetCurrentIlGenerator();
    TR::IlType *ptrType = map(base->type());
    registerValue(result, omr_b->IndexAt(ptrType, map(base), map(index)));
}


//
// internal functions
//

// Very important that a reference to the String from the IR is passed all the way in here
// Too easy to get a stack copy that allocates a C string, then frees it, then another
// String can allocate that memory to store a different C string and then the mapping is
// messed up. Should really do the strcmp below under a Config "pedantic" option...
const char *
JBMethodBuilder::findOrCreateString(const String & str) {
    const char *cstr = str.c_str();
    //std::cout << "Looking for " << (intptr_t) cstr << " " << cstr << "\n";
    if (_strings.find(cstr) != _strings.end()) {
        const char *s = _strings[cstr];
        //std::cout << "    Found " << s << " (" << (intptr_t)(s) << ")\n";
        if (strncmp(cstr, s, strlen(cstr)) == 0) // disturbing that need to verify
            return s;
    }

    if (str.length() == 0)
        return "";

    char *s = _comp->mem()->allocate<char>(str.length()+1);
    strcpy(s, cstr);
    _strings[cstr] = s;
    //std::cout << "    Allocated " << s << " (" << (intptr_t) s << ") for " << str.c_str() << " (" << (intptr_t)(_strings[cstr]) << ")\n";
    return _strings[cstr];
}

void
JBMethodBuilder::registerValue(const Value * v, TR::IlValue *omr_v) {
    _values[v->id()] = omr_v;
}

TR::IlBuilder *
JBMethodBuilder::map(const Builder * b, bool checkNull) {
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
JBMethodBuilder::mapBytecodeBuilder(const Builder * bcb, bool checkNull) {
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
JBMethodBuilder::map(const Value * v) {
    assert(_values.find(v->id()) != _values.end());
    TR::IlValue *omr_v = _values[v->id()];
    assert(omr_v != NULL);
    return omr_v;
}

TR::IlType *
JBMethodBuilder::map(const Type * t) {
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
JBCodeGenerator::registerStruct(TR::TypeDictionary * types, StructType * sType, char *structName, String fNamePrefix, size_t baseOffset) {
    for (auto fIt = sType->FieldsBegin(); fIt != sType->FieldsEnd(); fIt++) {
        FieldType *fType = fIt->second;
        String fieldName = fNamePrefix + fType->name();
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
            _mb->typeDictionary()->DefineField(structName, fieldString, mapType(fType->type()), fieldOffset/8); // JB uses bytes offsets
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
JBMethodBuilder::printAllMaps() {
    TextLogger *pLgr = _comp->logger(traceEnabled());
    if (pLgr) {
        TextLogger & lgr = *pLgr;

        lgr << "[ printAllMaps" << lgr.endl();
        lgr.indentIn();

        lgr.indent() << "[ Builders" << lgr.endl();
        lgr.indentIn();
        for (auto builderIt = _builders.cbegin(); builderIt != _builders.cend(); builderIt++) {
            lgr.indent() << "[ builder " << builderIt->first << " -> TR::IlBuilder " << (int64_t *)(void *) builderIt->second << " ]" << lgr.endl();
        }
        lgr.indentOut();
        lgr.indent() << "]" << lgr.endl();

        lgr.indent() << "[ Values" << lgr.endl();
        lgr.indentIn();
        for (auto valueIt = _values.cbegin(); valueIt != _values.cend(); valueIt++) {
            lgr.indent() << "[ value " << valueIt->first << " -> TR::IlValue " << (int64_t *)(void *) valueIt->second << " ]" << lgr.endl();
        }
        lgr.indentOut();
        lgr.indent() << "]" << lgr.endl();

        lgr.indent() << "[ Types" << lgr.endl();
        lgr.indentIn();
        for (auto typeIt = _types.cbegin(); typeIt != _types.cend(); typeIt++) {
            lgr.indent() << "[ type " << typeIt->first << " -> TR::IlType " << (int64_t *)(void *) typeIt->second << " ]" << lgr.endl();
        }
        lgr.indentOut();
        lgr.indent() << "]" << lgr.endl();

        lgr.indentOut();
        lgr.indent() << "]" << lgr.endl();
    }
}

#if 0
// some of this may be handy so keep it at hand during migration

void
JBCodeGenerator::generateFunctionAPI(FunctionBuilder *fb) {
    TextLogger *lgr = fb->logger(traceEnabled());
    if (lgr) lgr->indent() << "JBCodeGenerator::generateFunctionAPI F" << fb->id() << lgr->endl();

    TR::TypeDictionary * typesJB = _mb->typeDictionary();
    _typeDictionaries[types->id()] = typesJB;

    TypeDictionary *dict = _comp->dict();
    TypeID numTypes = _comp->numTypes();
    while (numTypes > 0) {
        TypeID startNumTypes = numTypes;
        for (auto it = dict->typesIterator(); it.hasItem(); it++) {
            Type *type = it.item();
            if (_types[type->id()] == NULL) {
                bool mapped = type->registerJBType(this)
                if (mapped) {
                    assert(_types[type->id()] != NULL);
                    numTypes--;
                }
            }
        }
        assert(numTypes < startNumTypes);
    }

#if 0
    if (lgr) lgr->indent() << "First pass:" << lgr->endl();
    for (TypeIterator typeIt = types->typesIterator(); typeIt.hasItem(); typeIt++) {
        Type * type = typeIt.item();
        if (lgr) {
            lgr->indent();
            lgr->logType(type, true);
            lgr << lgr->endl();
        }
        if (type->isStruct() || type->isUnion()) {
            char *name = findOrCreateString(type->name());
            storeType(type, _mb->typeDictionary()->DefineStruct(name));
        }
        else if (type->isFunction()) {
            // function types all map to Address in JB2 1.0
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
    for (TypeIterator typeIt = types->typesIterator(); typeIt.hasItem(); typeIt++) {
        Type * type = typeIt.item();
        if (type->isPointer()) {
            TR::IlType *ptrIlType = mapPointerType(typesJB, static_cast<PointerType *>(type));
            storeType(type, ptrIlType);
        }
    }

    // all basic types should have mappings now. what remains is to define the fields of
    // structs and unions to JB so that fields will be mapped to appropriate offsets
    // in this process, any inner structs/unions are inlined into containing struct

    // Third pass: revisit all Structs and Unions to iterate over field types to map them
    for (TypeIterator typeIt = types->typesIterator(); typeIt.hasItem(); typeIt++) {
        Type * type = typeIt.item();
        if (type->isStruct()) {
            StructType *sType = static_cast<StructType *>(type);
            char * structName = findOrCreateString(sType->name());
            mapStructFields(typesJB, sType, structName, String(""), 0);
            _mb->typeDictionary()->CloseStruct(structName, sType->size()/8);
        }
        else if (type->isUnion()) {
            UnionType *uType = static_cast<UnionType *>(type);
            char * structName = findOrCreateString(uType->name());
            mapStructFields(typesJB, uType, structName, String(""), 0);
            _mb->typeDictionary()->CloseStruct(structName, uType->size()/8);
        }
    }

    // All types should be represented in the JB layer now, and mapTypes should be set up for every
    // type in TypeDictionary
    for (TypeIterator typeIt = types->typesIterator(); typeIt.hasItem(); typeIt++) {
        Type * type = typeIt.item();
        assert(type->isField() || mapType(type) != NULL);
    }
#endif

    _methodBuilders[_comp->id()] = _mb;
    storeBuilder(fb, _mb);

    _comp->constructJBFunction(this);
}

FunctionBuilder *
JBCodeGenerator::transformFunctionBuilder(FunctionBuilder *fb) {
    TextLogger *lgr = _fb->logger(traceEnabled());
    if (lgr) lgr->indent() << "JBCodeGenerator transformFunctionBuilder F" << fb->id() << lgr->endl();
    if (lgr) lgr->indentIn();
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

{
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
   TextLogger *lgr = fb->logger(traceEnabled());
   if (lgr) lgr->indentOut();
   printAllMaps();
   return NULL;
   }
#endif

} // namespace jbgen
} // namespace JB2
} // namespace OMR


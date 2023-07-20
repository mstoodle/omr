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

#include "Base/Base.hpp"
#include "JBCore.hpp"
#include "jb/BaseCodeGenerator.hpp"
#include "jb/JBMethodBuilder.hpp"


namespace OMR {
namespace JitBuilder {
namespace JB {

static const MajorID BASEDON_BASEEXT_MAJOR=0;
static const MajorID BASEDON_BASEEXT_MINOR=1;
static const MajorID BASEDON_BASEEXT_PATCH=0;
const static SemanticVersion correctBaseVersion(BASEDON_BASEEXT_MAJOR,BASEDON_BASEEXT_MINOR,BASEDON_BASEEXT_PATCH);

INIT_JBALLOC_REUSECAT(BaseJBCodeGenerator, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(BaseJBCodeGenerator,"BaseJBCodeGenerator",JBCodeGenerator,Extensible);

BaseJBCodeGenerator::BaseJBCodeGenerator(Allocator *a, Base::BaseExtension *base)
    : JBCodeGenerator(a, base)
    , _base(base)
    , _gencodeVFT(NULL, a)
    , _genconstVFT(NULL, a)
    , _regtypeVFT(NULL, a) {

    assert(base->semver()->isCompatibleWith(correctBaseVersion));

    // assign these in reverse order so VFT only has to be grown once
    _regtypeVFT.assign(base->Address->id(), &BaseJBCodeGenerator::regtypeAddress);
    _regtypeVFT.assign(base->Float64->id(), &BaseJBCodeGenerator::regtypeFloat64);
    _regtypeVFT.assign(base->Float32->id(), &BaseJBCodeGenerator::regtypeFloat32);
    _regtypeVFT.assign(base->Int64->id(), &BaseJBCodeGenerator::regtypeInt64);
    _regtypeVFT.assign(base->Int32->id(), &BaseJBCodeGenerator::regtypeInt32);
    _regtypeVFT.assign(base->Int16->id(), &BaseJBCodeGenerator::regtypeInt16);
    _regtypeVFT.assign(base->Int8->id(), &BaseJBCodeGenerator::regtypeInt8);

    // assign these in reverse order so VFT only has to be grown once
    _gencodeVFT.assign(base->aIndexAt, &BaseJBCodeGenerator::gencodeIndexAt);
    _gencodeVFT.assign(base->aCreateLocalStruct, &BaseJBCodeGenerator::gencodeCreateLocalStruct);
    _gencodeVFT.assign(base->aCreateLocalArray, &BaseJBCodeGenerator::gencodeCreateLocalArray);
    _gencodeVFT.assign(base->aStoreFieldAt, &BaseJBCodeGenerator::gencodeStoreFieldAt);
    _gencodeVFT.assign(base->aLoadFieldAt, &BaseJBCodeGenerator::gencodeLoadFieldAt);
    _gencodeVFT.assign(base->aStoreField, &BaseJBCodeGenerator::gencodeStoreField);
    _gencodeVFT.assign(base->aStoreAt, &BaseJBCodeGenerator::gencodeStoreAt);
    _gencodeVFT.assign(base->aLoadAt, &BaseJBCodeGenerator::gencodeLoadAt);
    _gencodeVFT.assign(base->aIfCmpUnsignedLessOrEqual, &BaseJBCodeGenerator::gencodeIfCmpUnsignedLessOrEqual);
    _gencodeVFT.assign(base->aIfCmpUnsignedLessThan, &BaseJBCodeGenerator::gencodeIfCmpUnsignedLessThan);
    _gencodeVFT.assign(base->aIfCmpUnsignedGreaterOrEqual, &BaseJBCodeGenerator::gencodeIfCmpUnsignedGreaterOrEqual);
    _gencodeVFT.assign(base->aIfCmpUnsignedGreaterThan, &BaseJBCodeGenerator::gencodeIfCmpUnsignedGreaterThan);
    _gencodeVFT.assign(base->aIfCmpNotEqualZero, &BaseJBCodeGenerator::gencodeIfCmpNotEqualZero);
    _gencodeVFT.assign(base->aIfCmpNotEqual, &BaseJBCodeGenerator::gencodeIfCmpNotEqual);
    _gencodeVFT.assign(base->aIfCmpLessOrEqual, &BaseJBCodeGenerator::gencodeIfCmpLessOrEqual);
    _gencodeVFT.assign(base->aIfCmpLessThan, &BaseJBCodeGenerator::gencodeIfCmpLessThan);
    _gencodeVFT.assign(base->aIfCmpGreaterOrEqual, &BaseJBCodeGenerator::gencodeIfCmpGreaterOrEqual);
    _gencodeVFT.assign(base->aIfCmpGreaterThan, &BaseJBCodeGenerator::gencodeIfCmpGreaterThan);
    _gencodeVFT.assign(base->aIfCmpEqualZero, &BaseJBCodeGenerator::gencodeIfCmpEqualZero);
    _gencodeVFT.assign(base->aIfCmpEqual, &BaseJBCodeGenerator::gencodeIfCmpEqual);
    _gencodeVFT.assign(base->aGoto, &BaseJBCodeGenerator::gencodeGoto);
    _gencodeVFT.assign(base->aForLoopUp, &BaseJBCodeGenerator::gencodeForLoopUp);
    _gencodeVFT.assign(base->aSub, &BaseJBCodeGenerator::gencodeSub);
    _gencodeVFT.assign(base->aMul, &BaseJBCodeGenerator::gencodeMul);
    _gencodeVFT.assign(base->aConvertTo, &BaseJBCodeGenerator::gencodeConvertTo);
    _gencodeVFT.assign(base->aAdd, &BaseJBCodeGenerator::gencodeAdd);
    _gencodeVFT.assign(base->aConst, &BaseJBCodeGenerator::gencodeConst);

    // assign these in reverse order so VFT only has to be grown once
    _genconstVFT.assign(base->Address->id(), &BaseJBCodeGenerator::genconstAddress);
    _genconstVFT.assign(base->Float64->id(), &BaseJBCodeGenerator::genconstFloat64);
    _genconstVFT.assign(base->Float32->id(), &BaseJBCodeGenerator::genconstFloat32);
    _genconstVFT.assign(base->Int64->id(), &BaseJBCodeGenerator::genconstInt64);
    _genconstVFT.assign(base->Int32->id(), &BaseJBCodeGenerator::genconstInt32);
    _genconstVFT.assign(base->Int16->id(), &BaseJBCodeGenerator::genconstInt16);
    _genconstVFT.assign(base->Int8->id(), &BaseJBCodeGenerator::genconstInt8);

    setTraceEnabled(false);
}

BaseJBCodeGenerator::~BaseJBCodeGenerator() {
}


//
// regtype functions per primitive type
//
void
BaseJBCodeGenerator::regtypeInt8(JBMethodBuilder *jbmb, const Type *Int8) {
    jbmb->registerInt8(Int8);
}

void
BaseJBCodeGenerator::regtypeInt16(JBMethodBuilder *jbmb, const Type *Int16) {
    jbmb->registerInt16(Int16);
}

void
BaseJBCodeGenerator::regtypeInt32(JBMethodBuilder *jbmb, const Type *Int32) {
    jbmb->registerInt32(Int32);
}

void
BaseJBCodeGenerator::regtypeInt64(JBMethodBuilder *jbmb, const Type *Int64) {
    jbmb->registerInt64(Int64);
}

void
BaseJBCodeGenerator::regtypeFloat32(JBMethodBuilder *jbmb, const Type *Float32) {
    jbmb->registerFloat(Float32);
}

void
BaseJBCodeGenerator::regtypeFloat64(JBMethodBuilder *jbmb, const Type *Float64) {
    jbmb->registerDouble(Float64);
}

void
BaseJBCodeGenerator::regtypeAddress(JBMethodBuilder *jbmb, const Type *Address) {
    jbmb->registerAddress(Address);
}

void
BaseJBCodeGenerator::registerAllStructFields(JBMethodBuilder *jbmb, const Base::StructType *sType, String structName, String fNamePrefix, size_t baseOffset) const {
    for (auto fIt = sType->FieldsBegin(); fIt != sType->FieldsEnd(); fIt++) {
        const Base::FieldType *fType = fIt->second;
        String fieldName = fNamePrefix + fType->name();
        size_t fieldOffset = baseOffset + fType->offset();

        if (fType->isKind<const Base::StructType>()) {
            // define a "dummy" field corresponding to the struct field itself, so we can ask for its address easily
            // in case this field's struct needs to be passed to anything
            jbmb->registerField(structName, fieldName, _base->NoType, fieldOffset);
            const Base::StructType *innerStructType = fType->type()->refine<const Base::StructType>();
            registerAllStructFields(jbmb, sType, structName, fieldName + ".", fieldOffset);
        }
        else {
            jbmb->registerField(structName, fieldName, fType->type(), fieldOffset);
        }
    }
}

bool
BaseJBCodeGenerator::registerType(JBMethodBuilder *jbmb, const Type *t) {
    if (t->isKind<Base::PointerType>()) {
        const Type *baseType = t->refine<Base::PointerType>()->baseType();
        if (!jbmb->typeRegistered(baseType))
            return false; // will be registered later in this registration pass
        jbmb->registerPointer(t, baseType);
    }
    else if (t->isKind<const Base::StructType>()) {
        if (!jbmb->typeRegistered(t)) {
            jbmb->registerStruct(t);
            return false; // first pass just creates struct types
        }

        registerAllStructFields(jbmb, t->refine<const Base::StructType>(), t->name(), String(""), 0); // second pass defines the fields
        jbmb->closeStruct(t->name());
    }
    else if (t->isKind<const Base::FieldType>()) {
        // fields are registered as part of registering the struct
    } else {
        TypeID id = t->id();
        regtypeFunction f = _regtypeVFT[id];
        if (f != NULL) {
            (this->*f)(jbmb, t);
            return true;
        }
    }

    return true;
}


//
// gencode functions per Operation
//

void
BaseJBCodeGenerator::gencode(JBMethodBuilder *jbmb, Operation *op) {
    ActionID a = op->action();
    gencodeFunction f = _gencodeVFT[a];
    (this->*f)(jbmb, op);
}

void
BaseJBCodeGenerator::gencodeAdd(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aAdd);
    jbmb->Add(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeConvertTo(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aConvertTo);
    jbmb->ConvertTo(op->location(), op->parent(), op->result(), op->type(), op->operand());
}

void
BaseJBCodeGenerator::gencodeMul(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aMul);
    jbmb->Mul(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeSub(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aSub);
    jbmb->Sub(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeForLoopUp(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aForLoopUp);
    jbmb->ForLoopUp(op->location(), op->parent(),
                    op->symbol(),    // loopVariable
                    op->operand(0),  // initial
                    op->operand(1),  // final
                    op->operand(2),  // bump
                    op->builder(0),  // loopBody
                    op->builder(1),  // loopBreak
                    op->builder(2)); // loopContinue
}

void
BaseJBCodeGenerator::gencodeGoto(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aGoto);
    jbmb->Goto(op->location(), op->parent(), op->builder());
}

void
BaseJBCodeGenerator::gencodeIfCmpEqual(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpEqual);
    jbmb->IfCmpEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeIfCmpEqualZero(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpEqualZero);
    jbmb->IfCmpEqualZero(op->location(), op->parent(), op->builder(), op->operand(0));
}

void
BaseJBCodeGenerator::gencodeIfCmpGreaterThan(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpGreaterThan);
    jbmb->IfCmpGreaterThan(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeIfCmpGreaterOrEqual(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpGreaterOrEqual);
    jbmb->IfCmpGreaterOrEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeIfCmpLessThan(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpLessThan);
    jbmb->IfCmpLessThan(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeIfCmpLessOrEqual(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpLessOrEqual);
    jbmb->IfCmpLessOrEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeIfCmpNotEqual(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpNotEqual);
    jbmb->IfCmpNotEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeIfCmpNotEqualZero(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpNotEqualZero);
    jbmb->IfCmpNotEqualZero(op->location(), op->parent(), op->builder(), op->operand(0));
}

void
BaseJBCodeGenerator::gencodeIfCmpUnsignedGreaterThan(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpUnsignedGreaterThan);
    jbmb->IfCmpUnsignedGreaterThan(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeIfCmpUnsignedGreaterOrEqual(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpUnsignedGreaterOrEqual);
    jbmb->IfCmpUnsignedGreaterOrEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeIfCmpUnsignedLessThan(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpUnsignedLessThan);
    jbmb->IfCmpUnsignedLessThan(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeIfCmpUnsignedLessOrEqual(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIfCmpUnsignedLessOrEqual);
    jbmb->IfCmpUnsignedLessOrEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeLoadAt(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aLoadAt);
    jbmb->LoadAt(op->location(), op->parent(), op->result(), op->operand());
}

void
BaseJBCodeGenerator::gencodeStoreAt(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aStoreAt);
    jbmb->StoreAt(op->location(), op->parent(),
                  op->operand(0),  // base
                  op->operand(1)); // value
}

void
BaseJBCodeGenerator::gencodeLoadField(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aLoadField);
    assert(0); // TODO
}

void
BaseJBCodeGenerator::gencodeStoreField(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aStoreField);
    assert(0); // TODO
}

void
BaseJBCodeGenerator::gencodeLoadFieldAt(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aLoadFieldAt);
    const Base::FieldType *ft = op->type()->refine<Base::FieldType>();
    jbmb->LoadIndirect(op->location(), op->parent(), op->result(), ft->owningStruct()->name(), ft->name(), op->operand());
}

void
BaseJBCodeGenerator::gencodeStoreFieldAt(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aStoreFieldAt);
    const Base::FieldType *ft = op->type()->refine<Base::FieldType>();
    jbmb->StoreIndirect(op->location(), op->parent(), ft->owningStruct()->name(), ft->name(), op->operand(0), op->operand(1));
}

void
BaseJBCodeGenerator::gencodeCreateLocalArray(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aCreateLocalArray);
    jbmb->CreateLocalArray(op->location(), op->parent(), op->result(), op->literal(), op->type());
}

void
BaseJBCodeGenerator::gencodeCreateLocalStruct(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aCreateLocalStruct);
    jbmb->CreateLocalStruct(op->location(), op->parent(), op->result(), op->type());
}

void
BaseJBCodeGenerator::gencodeIndexAt(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aIndexAt);
    jbmb->IndexAt(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
}


//
// genconst functions per primitive type
//
void
BaseJBCodeGenerator::gencodeConst(JBMethodBuilder *jbmb, Operation *op) {
    assert(op->action() == _base->aConst);
    const Type *retType = op->result()->type();
    if (retType->isKind<Base::PointerType>())
        retType = _base->Address;
    genconstFunction f = _genconstVFT[retType->id()];
    (this->*f)(jbmb, op->location(), op->parent(), op->result(), op->literal());
}


void
BaseJBCodeGenerator::genconstInt8(JBMethodBuilder *jbmb, Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb->ConstInt8(loc, b, result, lv->value<const int8_t>());
}

void
BaseJBCodeGenerator::genconstInt16(JBMethodBuilder *jbmb, Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb->ConstInt16(loc, b, result, lv->value<const int16_t>());
}

void
BaseJBCodeGenerator::genconstInt32(JBMethodBuilder *jbmb, Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb->ConstInt32(loc, b, result, lv->value<const int32_t>());
}

void
BaseJBCodeGenerator::genconstInt64(JBMethodBuilder *jbmb, Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb->ConstInt64(loc, b, result, lv->value<const int64_t>());
}

void
BaseJBCodeGenerator::genconstFloat32(JBMethodBuilder *jbmb, Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb->ConstFloat(loc, b, result, lv->value<const float>());
}

void
BaseJBCodeGenerator::genconstFloat64(JBMethodBuilder *jbmb, Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb->ConstDouble(loc, b, result, lv->value<const double>());
}

void
BaseJBCodeGenerator::genconstAddress(JBMethodBuilder *jbmb, Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb->ConstAddress(loc, b, result, lv->value<void * const>());
}


#if 0
void
JBCodeGenerator::visitPostCompilation(Compilation *comp) {
    #if 0
    assert(_jbmb);
    TextLogger *lgr = comp->logger(traceEnabled());
    if (lgr) {
        lgr->indentOut();
        _jbmb->printAllMaps();
    }
    #endif
}

// old impl of JBCodeGenerator kept here so it's handy during mgration

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
        for (auto it = dict->TypesBegin(); it !+ dict->TypeEnd(); it++) {
            Type *type = *it;
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

    if (lgr) lgr->indent() << "First pass:" << lgr->endl();
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
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
            TR::IlType *ptrIlType = mapPointerType(typesJB, static_cast<PointerType *>(type));
            storeType(type, ptrIlType);
        }
    }

    // all basic types should have mappings now. what remains is to define the fields of
    // structs and unions to JB so that fields will be mapped to appropriate offsets
    // in this process, any inner structs/unions are inlined into containing struct

    // Third pass: revisit all Structs and Unions to iterate over field types to map them
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
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
    for (TypeIterator typeIt = types->TypesBegin(); typeIt != types->TypesEnd(); typeIt++) {
        Type * type = *typeIt;
        assert(type->isField() || mapType(type) != NULL);
    }

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

} // namespace JB
} // namespace JitBuilder
} // namespace OMR

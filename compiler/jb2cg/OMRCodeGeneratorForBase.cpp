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

#include "jb2/JBCore.hpp"
#include "jb2/Base/Base.hpp"
#include "jb2cg/OMRCodeGenerator.hpp"
#include "jb2cg/OMRCodeGeneratorForBase.hpp"
#include "jb2cg/OMRIlGen.hpp"


namespace OMR {
namespace JB2 {
namespace omrgen {

static const MajorID BASEDON_BASEEXT_MAJOR=0;
static const MajorID BASEDON_BASEEXT_MINOR=1;
static const MajorID BASEDON_BASEEXT_PATCH=0;
const static SemanticVersion minimumBaseVersion(BASEDON_BASEEXT_MAJOR,BASEDON_BASEEXT_MINOR,BASEDON_BASEEXT_PATCH);

INIT_JBALLOC_REUSECAT(OMRCodeGeneratorForBase, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(OMRCodeGeneratorForBase,"OMRCodeGeneratorForBase",Base::CodeGeneratorForBase,Extensible);

OMRCodeGeneratorForBase::OMRCodeGeneratorForBase(Allocator *a, OMRCodeGenerator *omrcg, Base::BaseExtension *bx)
    : Base::CodeGeneratorForBase(a, omrcg, bx)
    , _bx(bx)
    INIT_CG_BASE_VFT_FIELDS(a) {

    assert(bx->semver()->isCompatibleWith(minimumBaseVersion));

    INIT_CG_BASE_HANDLERS(OMRCodeGeneratorForBase);

    setTraceEnabled(false);
}

OMRCodeGeneratorForBase::~OMRCodeGeneratorForBase() {
}

Base::BaseExtension *
OMRCodeGeneratorForBase::bx() const {
    return _bx;
}

OMRCodeGenerator *
OMRCodeGeneratorForBase::omrcg() const {
    return cg()->refine<OMRCodeGenerator>();
}

OMRIlGen *
OMRCodeGeneratorForBase::ilgen() const {
    return omrcg()->ilgen();
}

//
// regtype functions per primitive type
//
void
OMRCodeGeneratorForBase::regtypeInt8(const Type *Int8) {
    ilgen()->registerInt8(Int8);
}

void
OMRCodeGeneratorForBase::regtypeInt16(const Type *Int16) {
    ilgen()->registerInt16(Int16);
}

void
OMRCodeGeneratorForBase::regtypeInt32(const Type *Int32) {
    ilgen()->registerInt32(Int32);
}

void
OMRCodeGeneratorForBase::regtypeInt64(const Type *Int64) {
    ilgen()->registerInt64(Int64);
}

void
OMRCodeGeneratorForBase::regtypeFloat32(const Type *Float32) {
    ilgen()->registerFloat(Float32);
}

void
OMRCodeGeneratorForBase::regtypeFloat64(const Type *Float64) {
    ilgen()->registerDouble(Float64);
}

void
OMRCodeGeneratorForBase::regtypeAddress(const Type *Address) {
    ilgen()->registerAddress(Address);
}

void
OMRCodeGeneratorForBase::registerField(const Type *ft, String baseStructName, String fieldName, const Type *fieldType, size_t fieldOffset) {
}

bool
OMRCodeGeneratorForBase::registerType(const Type *t) {
    if (t->isKind<Base::PointerType>()) {
        ilgen()->registerAddress(t);
        return true;
    } else if (t->isKind<const Base::StructType>()) {
        if (ilgen()->typeRegistered(t)) {
            ilgen()->registerStructType(t);
            return false; // first pass just creates struct types
        }

        const Base::StructType *structType = t->refine<const Base::StructType>();
        registerAllStructFields(structType, structType, String(allocator(), ""), 0); // second pass defines the fields
        return true;
    } else if (t->isKind<const Base::FieldType>()) {
        //nothing to do, fields turn into symbol references during compilation
        //const Base::FieldType *ft = t->refine<const Base::FieldType>();
        //return ilgen()->registerFieldType(ft, ft->owningStruct()->name(), ft->fieldName(), ft->type(), ft->offset());
        return true;
    } else {
        TypeID id = t->id();
        regtypeFunction f = _regtypeVFT[id];
        if (f != NULL) {
            (this->*f)(t);
            return true;
        }
    }

    return false;
}


//
// gencode functions per Operation
//

Builder *
OMRCodeGeneratorForBase::gencode(Operation *op) {
    ActionID a = op->action();
    gencodeFunction f = _gencodeVFT[a];
    return (this->*f)(op);
}

Builder *
OMRCodeGeneratorForBase::gencodeAdd(Operation *op) {
    assert(op->action() == _bx->aAdd);
    ilgen()->add(op->location(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeAnd(Operation *op) {
    assert(op->action() == _bx->aAnd);
    ilgen()->and_(op->location(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeConvertTo(Operation *op) {
    assert(op->action() == _bx->aConvertTo);
    ilgen()->convertTo(op->location(), op->result(), op->type(0), op->operand(0), false);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeDiv(Operation *op) {
    assert(op->action() == _bx->aDiv);
    ilgen()->div(op->location(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeEqualTo(Operation *op) {
    assert(op->action() == _bx->aEqualTo);
    ilgen()->equalTo(op->location(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeMul(Operation *op) {
    assert(op->action() == _bx->aMul);
    ilgen()->mul(op->location(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeNotEqualTo(Operation *op) {
    assert(op->action() == _bx->aNotEqualTo);
    ilgen()->notEqualTo(op->location(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeSub(Operation *op) {
    assert(op->action() == _bx->aSub);
    ilgen()->sub(op->location(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeForLoopUp(Operation *op) {
    assert(op->action() == _bx->aForLoopUp);
    assert(0); // TODO
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeGoto(Operation *op) {
    assert(op->action() == _bx->aGoto);
    ilgen()->goto_(op->location(), op->builder());
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpEqual);
    ilgen()->ifCmpEqual(op->location(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpEqualZero(Operation *op) {
    assert(op->action() == _bx->aIfCmpEqualZero);
    ilgen()->ifCmpEqualZero(op->location(), op->builder(), op->operand());
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpGreaterThan(Operation *op) {
    assert(op->action() == _bx->aIfCmpGreaterThan);
    ilgen()->ifCmpGreaterThan(op->location(), op->builder(), op->operand(0), op->operand(1), false);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpGreaterOrEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpGreaterOrEqual);
    ilgen()->ifCmpGreaterOrEqual(op->location(), op->builder(), op->operand(0), op->operand(1), false);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpLessThan(Operation *op) {
    assert(op->action() == _bx->aIfCmpLessThan);
    ilgen()->ifCmpLessThan(op->location(), op->builder(), op->operand(0), op->operand(1), false);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpLessOrEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpLessOrEqual);
    ilgen()->ifCmpLessOrEqual(op->location(), op->builder(), op->operand(0), op->operand(1), false);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpNotEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpNotEqual);
    ilgen()->ifCmpNotEqual(op->location(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpNotEqualZero(Operation *op) {
    assert(op->action() == _bx->aIfCmpNotEqualZero);
    ilgen()->ifCmpNotEqualZero(op->location(), op->builder(), op->operand());
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpUnsignedGreaterThan(Operation *op) {
    assert(op->action() == _bx->aIfCmpUnsignedGreaterThan);
    ilgen()->ifCmpGreaterThan(op->location(), op->builder(), op->operand(0), op->operand(1), true);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpUnsignedGreaterOrEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpUnsignedGreaterOrEqual);
    ilgen()->ifCmpGreaterOrEqual(op->location(), op->builder(), op->operand(0), op->operand(1), true);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpUnsignedLessThan(Operation *op) {
    assert(op->action() == _bx->aIfCmpUnsignedLessThan);
    ilgen()->ifCmpLessThan(op->location(), op->builder(), op->operand(0), op->operand(1), true);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfCmpUnsignedLessOrEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpUnsignedLessOrEqual);
    ilgen()->ifCmpLessOrEqual(op->location(), op->builder(), op->operand(0), op->operand(1), true);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIfThenElse(Operation *op) {
    assert(op->action() == _bx->aIfThenElse);
    assert(0); // TODO
    #if 0
    if (op->builder(1))
        jbmb()->IfThenElse(op->location(), op->parent(), op->builder(0), op->builder(1), op->operand());
    jbmb()->IfThen(op->location(), op->parent(), op->builder(), op->operand());
    #endif
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeSwitch(Operation *op) {
    assert(op->action() == _bx->aSwitch);
    assert(0); // TODO
    #if 0
    Base::Op_Switch *opSwitch = static_cast<Base::Op_Switch *>(op);
    Allocator *mem = op->parent()->ir()->mem();
    int numCases = opSwitch->numCases();
    Literal **lvs = new (mem) Literal *[numCases];
    Builder **builders = new (mem) Builder *[numCases];
    bool *fallThroughs = new (mem) bool[numCases];
    int index = 0;
    for (auto it = opSwitch->cases(); it.hasItem(); it++) {
        Base::Case *c = it.item();
        lvs[index] = c->literal();
        builders[index] = c->builder();
        fallThroughs[index] = c->fallsThrough();
        index++;
    }
    jbmb()->Switch(op->location(), op->parent(), opSwitch->defaultBuilder(), opSwitch->selector(), numCases, lvs, builders, fallThroughs);
    delete[] fallThroughs;
    delete[] builders;
    delete[] lvs;
    #endif
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeLoadAt(Operation *op) {
    assert(op->action() == _bx->aLoadAt);
    Base::Op_LoadAt *opLoadAt = static_cast<Base::Op_LoadAt *>(op);
    Value *address = opLoadAt->operand();
    const Base::PointerType *addressType = address->type()->refine<Base::PointerType>();
    ilgen()->loadAt(opLoadAt->location(), opLoadAt->result(), address, addressType->baseType());
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeStoreAt(Operation *op) {
    assert(op->action() == _bx->aStoreAt);
    Base::Op_StoreAt *opStoreAt = static_cast<Base::Op_StoreAt *>(op);
    Value *address = opStoreAt->operand(0);
    const Base::PointerType *addressType = address->type()->refine<Base::PointerType>();
    ilgen()->storeAt(opStoreAt->location(), address, addressType->baseType(), opStoreAt->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeLoadField(Operation *op) {
    assert(op->action() == _bx->aLoadField);
    assert(0); // TODO
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeStoreField(Operation *op) {
    assert(op->action() == _bx->aStoreField);
    assert(0); // TODO
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeLoadFieldAt(Operation *op) {
    assert(op->action() == _bx->aLoadFieldAt);
    const Base::FieldType *ft = op->type()->refine<Base::FieldType>();
    ilgen()->loadFieldAt(op->location(), op->result(), op->operand(), ft, ft->owningStruct()->name(), ft->name(), ft->type(), ft->offset());
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeStoreFieldAt(Operation *op) {
    assert(op->action() == _bx->aStoreFieldAt);
    const Base::FieldType *ft = op->type()->refine<Base::FieldType>();
    ilgen()->storeFieldAt(op->location(), op->operand(0), ft, ft->owningStruct()->name(), ft->name(), ft->type(), ft->offset(), op->operand(1));
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeCreateLocalArray(Operation *op) {
    assert(op->action() == _bx->aCreateLocalArray);
    const Type *elementType = op->type()->refine<Base::PointerType>()->baseType();
    ilgen()->createlocalarray(op->location(), op->result(), op->literal(), elementType);
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeCreateLocalStruct(Operation *op) {
    assert(op->action() == _bx->aCreateLocalStruct);
    assert(0); // TODO
    return NULL;
}

Builder *
OMRCodeGeneratorForBase::gencodeIndexAt(Operation *op) {
    assert(op->action() == _bx->aIndexAt);
    Value *base = op->operand(0);
    const Type *elementType = base->type()->refine<Base::PointerType>()->baseType();
    ilgen()->indexAt(op->location(), op->result(), base, elementType, op->operand(1));
    return NULL;
}


//
// genconst functions per primitive type
//
Builder *
OMRCodeGeneratorForBase::gencodeConst(Operation *op) {
    assert(op->action() == _bx->aConst);
    const Type *retType = op->result()->type();
    if (retType->isKind<Base::PointerType>())
        retType = _bx->Address(retType->ir());
    genconstFunction f = _genconstVFT[retType->id()];
    (this->*f)(op->location(), op->parent(), op->result(), op->literal());
    return NULL;
}

void
OMRCodeGeneratorForBase::genconstInt8(Location *loc, Builder *b, Value *result, Literal *lv) {
    ilgen()->literalInt8(result, lv->value<int8_t>());
}

void
OMRCodeGeneratorForBase::genconstInt16(Location *loc, Builder *b, Value *result, Literal *lv) {
    ilgen()->literalInt16(result, lv->value<int16_t>());
}

void
OMRCodeGeneratorForBase::genconstInt32(Location *loc, Builder *b, Value *result, Literal *lv) {
    ilgen()->literalInt32(result, lv->value<int32_t>());
}

void
OMRCodeGeneratorForBase::genconstInt64(Location *loc, Builder *b, Value *result, Literal *lv) {
    ilgen()->literalInt64(result, lv->value<int64_t>());
}

void
OMRCodeGeneratorForBase::genconstFloat32(Location *loc, Builder *b, Value *result, Literal *lv) {
    ilgen()->literalFloat(result, lv->value<float>());
}

void
OMRCodeGeneratorForBase::genconstFloat64(Location *loc, Builder *b, Value *result, Literal *lv) {
    ilgen()->literalDouble(result, lv->value<double>());
}

void
OMRCodeGeneratorForBase::genconstAddress(Location *loc, Builder *b, Value *result, Literal *lv) {
    ilgen()->literalAddress(result, lv->value<uintptr_t>());
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

} // namespace omrgen
} // namespace JB2
} // namespace OMR

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
#include "jbgen/JBCodeGenerator.hpp"
#include "jbgen/JBCodeGeneratorForBase.hpp"
#include "jbgen/JBMethodBuilder.hpp"


namespace OMR {
namespace JB2 {
namespace jbgen {

static const MajorID BASEDON_BASEEXT_MAJOR=0;
static const MajorID BASEDON_BASEEXT_MINOR=1;
static const MajorID BASEDON_BASEEXT_PATCH=0;
const static SemanticVersion minimumBaseVersion(BASEDON_BASEEXT_MAJOR,BASEDON_BASEEXT_MINOR,BASEDON_BASEEXT_PATCH);

INIT_JBALLOC_REUSECAT(JBCodeGeneratorForBase, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(JBCodeGeneratorForBase,"JBCodeGeneratorForBase",JBCodeGenerator,Extensible);

JBCodeGeneratorForBase::JBCodeGeneratorForBase(Allocator *a, JBCodeGenerator *jbcg, Base::BaseExtension *bx)
    : Base::CodeGeneratorForBase(a, jbcg, bx)
    , _bx(bx)
    INIT_CG_BASE_VFT_FIELDS(a) {

    assert(bx->semver()->isCompatibleWith(minimumBaseVersion));

    INIT_CG_BASE_HANDLERS(JBCodeGeneratorForBase);

    setTraceEnabled(false);
}

JBCodeGeneratorForBase::~JBCodeGeneratorForBase() {
}

Base::BaseExtension *
JBCodeGeneratorForBase::bx() const {
    return _bx;
}

JBCodeGenerator *
JBCodeGeneratorForBase::jbcg() const {
    return cg()->refine<JBCodeGenerator>();
}

JBMethodBuilder *
JBCodeGeneratorForBase::jbmb() const {
    return jbcg()->jbmb();
}

//
// regtype functions per primitive type
//
void
JBCodeGeneratorForBase::regtypeInt8(const Type *Int8) {
    jbmb()->registerInt8(Int8);
}

void
JBCodeGeneratorForBase::regtypeInt16(const Type *Int16) {
    jbmb()->registerInt16(Int16);
}

void
JBCodeGeneratorForBase::regtypeInt32(const Type *Int32) {
    jbmb()->registerInt32(Int32);
}

void
JBCodeGeneratorForBase::regtypeInt64(const Type *Int64) {
    jbmb()->registerInt64(Int64);
}

void
JBCodeGeneratorForBase::regtypeFloat32(const Type *Float32) {
    jbmb()->registerFloat(Float32);
}

void
JBCodeGeneratorForBase::regtypeFloat64(const Type *Float64) {
    jbmb()->registerDouble(Float64);
}

void
JBCodeGeneratorForBase::regtypeAddress(const Type *Address) {
    jbmb()->registerAddress(Address);
}

void
JBCodeGeneratorForBase::registerField(const Type *ft, String baseStructName, String fieldName, const Type *fieldType, size_t fieldOffset) {
    jbmb()->registerField(baseStructName, fieldName, fieldType, fieldOffset);
}

bool
JBCodeGeneratorForBase::registerType(const Type *t) {
    if (t->isKind<Base::PointerType>()) {
        const Type *baseType = t->refine<Base::PointerType>()->baseType();
        if (!jbmb()->typeRegistered(baseType))
            return false; // will be registered later in this registration pass
        jbmb()->registerPointer(t, baseType);
    }
    else if (t->isKind<const Base::StructType>()) {
        if (!jbmb()->typeRegistered(t)) {
            jbmb()->registerStruct(t);
            return false; // first pass just creates struct types
        }

        const Base::StructType *structType = t->refine<const Base::StructType>();
        registerAllStructFields(structType, structType, String(allocator(), ""), 0); // second pass defines the fields
        jbmb()->closeStruct(t->name());
    }
    else if (t->isKind<const Base::FieldType>()) {
        // fields are registered as part of registering the struct
    } else {
        TypeID id = t->id();
        regtypeFunction f = _regtypeVFT[id];
        if (f != NULL) {
            (this->*f)(t);
            return true;
        }
    }

    return true;
}


//
// gencode functions per Operation
//

Builder *
JBCodeGeneratorForBase::gencode(Operation *op) {
    ActionID a = op->action();
    gencodeFunction f = _gencodeVFT[a];
    return (this->*f)(op);
}

Builder *
JBCodeGeneratorForBase::gencodeAdd(Operation *op) {
    assert(op->action() == _bx->aAdd);
    jbmb()->Add(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeAnd(Operation *op) {
    assert(op->action() == _bx->aAnd);
    jbmb()->And(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeConvertTo(Operation *op) {
    assert(op->action() == _bx->aConvertTo);
    jbmb()->ConvertTo(op->location(), op->parent(), op->result(), op->type(), op->operand());
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeDiv(Operation *op) {
    assert(op->action() == _bx->aDiv);
    jbmb()->Div(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeEqualTo(Operation *op) {
    assert(op->action() == _bx->aEqualTo);
    jbmb()->EqualTo(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeMul(Operation *op) {
    assert(op->action() == _bx->aMul);
    jbmb()->Mul(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeNotEqualTo(Operation *op) {
    assert(op->action() == _bx->aNotEqualTo);
    jbmb()->NotEqualTo(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeSub(Operation *op) {
    assert(op->action() == _bx->aSub);
    jbmb()->Sub(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeForLoopUp(Operation *op) {
    assert(op->action() == _bx->aForLoopUp);
    jbmb()->ForLoopUp(op->location(), op->parent(),
                      op->symbol(),    // loopVariable
                      op->operand(0),  // initial
                      op->operand(1),  // final
                      op->operand(2),  // bump
                      op->builder(0),  // loopBody
                      op->builder(1),  // loopBreak
                      op->builder(2)); // loopContinue
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeGoto(Operation *op) {
    assert(op->action() == _bx->aGoto);
    jbmb()->Goto(op->location(), op->parent(), op->builder());
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpEqual);
    jbmb()->IfCmpEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpEqualZero(Operation *op) {
    assert(op->action() == _bx->aIfCmpEqualZero);
    jbmb()->IfCmpEqualZero(op->location(), op->parent(), op->builder(), op->operand(0));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpGreaterThan(Operation *op) {
    assert(op->action() == _bx->aIfCmpGreaterThan);
    jbmb()->IfCmpGreaterThan(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpGreaterOrEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpGreaterOrEqual);
    jbmb()->IfCmpGreaterOrEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpLessThan(Operation *op) {
    assert(op->action() == _bx->aIfCmpLessThan);
    jbmb()->IfCmpLessThan(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpLessOrEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpLessOrEqual);
    jbmb()->IfCmpLessOrEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpNotEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpNotEqual);
    jbmb()->IfCmpNotEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpNotEqualZero(Operation *op) {
    assert(op->action() == _bx->aIfCmpNotEqualZero);
    jbmb()->IfCmpNotEqualZero(op->location(), op->parent(), op->builder(), op->operand(0));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpUnsignedGreaterThan(Operation *op) {
    assert(op->action() == _bx->aIfCmpUnsignedGreaterThan);
    jbmb()->IfCmpUnsignedGreaterThan(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpUnsignedGreaterOrEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpUnsignedGreaterOrEqual);
    jbmb()->IfCmpUnsignedGreaterOrEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpUnsignedLessThan(Operation *op) {
    assert(op->action() == _bx->aIfCmpUnsignedLessThan);
    jbmb()->IfCmpUnsignedLessThan(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfCmpUnsignedLessOrEqual(Operation *op) {
    assert(op->action() == _bx->aIfCmpUnsignedLessOrEqual);
    jbmb()->IfCmpUnsignedLessOrEqual(op->location(), op->parent(), op->builder(), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIfThenElse(Operation *op) {
    assert(op->action() == _bx->aIfThenElse);
    if (op->builder(1))
        jbmb()->IfThenElse(op->location(), op->parent(), op->builder(0), op->builder(1), op->operand());
    jbmb()->IfThen(op->location(), op->parent(), op->builder(), op->operand());
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeSwitch(Operation *op) {
    assert(op->action() == _bx->aSwitch);
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
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeLoadAt(Operation *op) {
    assert(op->action() == _bx->aLoadAt);
    jbmb()->LoadAt(op->location(), op->parent(), op->result(), op->operand());
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeStoreAt(Operation *op) {
    assert(op->action() == _bx->aStoreAt);
    jbmb()->StoreAt(op->location(), op->parent(),
                    op->operand(0),  // base
                    op->operand(1)); // value
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeLoadField(Operation *op) {
    assert(op->action() == _bx->aLoadField);
    assert(0); // TODO
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeStoreField(Operation *op) {
    assert(op->action() == _bx->aStoreField);
    assert(0); // TODO
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeLoadFieldAt(Operation *op) {
    assert(op->action() == _bx->aLoadFieldAt);
    const Base::FieldType *ft = op->type()->refine<Base::FieldType>();
    const Base::StructType *owningStruct = ft->owningStruct();
    const String & structName = owningStruct->name();
    jbmb()->LoadIndirect(op->location(), op->parent(), op->result(), structName, lookupFieldString(owningStruct, ft), op->operand());
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeStoreFieldAt(Operation *op) {
    assert(op->action() == _bx->aStoreFieldAt);
    const Base::FieldType *ft = op->type()->refine<Base::FieldType>();
    const Base::StructType *owningStruct = ft->owningStruct();
    const String & structName = owningStruct->name();
    jbmb()->StoreIndirect(op->location(), op->parent(), structName, lookupFieldString(owningStruct, ft), op->operand(0), op->operand(1));
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeCreateLocalArray(Operation *op) {
    assert(op->action() == _bx->aCreateLocalArray);
    jbmb()->CreateLocalArray(op->location(), op->parent(), op->result(), op->literal(), op->type());
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeCreateLocalStruct(Operation *op) {
    assert(op->action() == _bx->aCreateLocalStruct);
    jbmb()->CreateLocalStruct(op->location(), op->parent(), op->result(), op->type());
    return NULL;
}

Builder *
JBCodeGeneratorForBase::gencodeIndexAt(Operation *op) {
    assert(op->action() == _bx->aIndexAt);
    jbmb()->IndexAt(op->location(), op->parent(), op->result(), op->operand(0), op->operand(1));
    return NULL;
}


//
// genconst functions per primitive type
//
Builder *
JBCodeGeneratorForBase::gencodeConst(Operation *op) {
    assert(op->action() == _bx->aConst);
    const Type *retType = op->result()->type();
    if (retType->isKind<Base::PointerType>())
        retType = op->ir()->addon<Base::BaseIRAddon>()->Address;
    genconstFunction f = _genconstVFT[retType->id()];
    (this->*f)(op->location(), op->parent(), op->result(), op->literal());
    return NULL;
}


void
JBCodeGeneratorForBase::genconstInt8(Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb()->ConstInt8(loc, b, result, lv->value<const int8_t>());
}

void
JBCodeGeneratorForBase::genconstInt16(Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb()->ConstInt16(loc, b, result, lv->value<const int16_t>());
}

void
JBCodeGeneratorForBase::genconstInt32(Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb()->ConstInt32(loc, b, result, lv->value<const int32_t>());
}

void
JBCodeGeneratorForBase::genconstInt64(Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb()->ConstInt64(loc, b, result, lv->value<const int64_t>());
}

void
JBCodeGeneratorForBase::genconstFloat32(Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb()->ConstFloat(loc, b, result, lv->value<const float>());
}

void
JBCodeGeneratorForBase::genconstFloat64(Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb()->ConstDouble(loc, b, result, lv->value<const double>());
}

void
JBCodeGeneratorForBase::genconstAddress(Location *loc, Builder *b, Value *result, Literal *lv) {
    jbmb()->ConstAddress(loc, b, result, lv->value<void * const>());
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

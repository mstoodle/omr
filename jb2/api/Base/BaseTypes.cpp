/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.acompache.org/licenses/LICENSE-2.0.
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

#include <cstring>
#include "Base/BaseIRAddon.hpp"
#include "Base/BaseExtension.hpp"
#include "Base/BaseTypes.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

INIT_JBALLOC_REUSECAT(BaseType, Type)
SUBCLASS_KINDSERVICE_IMPL(BaseType, "BaseType", Type, Type);

BaseType::~BaseType() {

}

BaseExtension *
BaseType::baseExt() {
    return static_cast<BaseExtension *>(_ext);
}

BaseExtension *
BaseType::baseExt() const {
    return static_cast<BaseExtension * const>(_ext);
}


INIT_JBALLOC_REUSECAT(NumericType, BaseType)
SUBCLASS_KINDSERVICE_IMPL(NumericType, "NumericType", BaseType, Type);

NumericType::~NumericType() {

}


INIT_JBALLOC_REUSECAT(IntegerType, NumericType)
SUBCLASS_KINDSERVICE_IMPL(IntegerType, "IntegerType", NumericType, Type);

IntegerType::~IntegerType() {

}


INIT_JBALLOC_REUSECAT(Int8Type, IntegerType)
SUBCLASS_KINDSERVICE_IMPL(Int8Type, "Int8Type", IntegerType, Type);

Int8Type::Int8Type(Allocator *a, const Int8Type *source, IRCloner *cloner)
    : IntegerType(a, source, cloner) {

}

const Type *
Int8Type::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) Int8Type(a, this, cloner);
}

Int8Type::~Int8Type() {

}

Literal *
Int8Type::literal(LOCATION, IR *ir, const int8_t value) const {
    return this->Type::literal(PASSLOC, ir, reinterpret_cast<const LiteralBytes *>(&value));
}

bool
Int8Type::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<const int8_t *>(l1)) == (*reinterpret_cast<const int8_t *>(l2));
}

void
Int8Type::logValue(TextLogger & lgr, const void *p) const {
    lgr << name() << " " << ((int) *reinterpret_cast<const int8_t *>(p));
}

void
Int8Type::logLiteral(TextLogger & lgr, const Literal *lv) const {
    lgr << name() << "(" << ((int) lv->value<const int8_t>()) << ")";
}

const int64_t
Int8Type::getInteger(const Literal *lv) const {
    return (const int64_t) (lv->value<const int8_t>());
}


INIT_JBALLOC_REUSECAT(Int16Type, IntegerType)
SUBCLASS_KINDSERVICE_IMPL(Int16Type, "Int16Type", IntegerType, Type);

Int16Type::Int16Type(Allocator *a, const Int16Type *source, IRCloner *cloner)
    : IntegerType(a, source, cloner) {

}

const Type *
Int16Type::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) Int16Type(a, this, cloner);
}

Int16Type::~Int16Type() {

}

Literal *
Int16Type::literal(LOCATION, IR *ir, const int16_t value) const {
    return this->Type::literal(PASSLOC, ir, reinterpret_cast<const LiteralBytes *>(&value));
}

bool
Int16Type::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<const int16_t *>(l1)) == (*reinterpret_cast<const int16_t *>(l2));
}

void
Int16Type::logValue(TextLogger &lgr, const void *p) const {
    lgr << name() << " " << *reinterpret_cast<const int16_t *>(p);
}

void
Int16Type::logLiteral(TextLogger & lgr, const Literal *lv) const {
    lgr << name() << "(" << (lv->value<const int16_t>()) << ")";
}

const int64_t
Int16Type::getInteger(const Literal *lv) const {
    return (const int64_t) (lv->value<const int16_t>());
}


INIT_JBALLOC_REUSECAT(Int32Type, IntegerType)
SUBCLASS_KINDSERVICE_IMPL(Int32Type, "Int32Type", IntegerType, Type);

Int32Type::Int32Type(Allocator *a, const Int32Type *source, IRCloner *cloner)
    : IntegerType(a, source, cloner) {

}

const Type *
Int32Type::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) Int32Type(a, this, cloner);
}

Int32Type::~Int32Type() {

}

Literal *
Int32Type::literal(LOCATION, IR *ir, const int32_t value) const {
    return this->Type::literal(PASSLOC, ir, reinterpret_cast<const LiteralBytes *>(&value));
}

bool
Int32Type::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<const int32_t *>(l1)) == (*reinterpret_cast<const int32_t *>(l2));
}

void
Int32Type::logValue(TextLogger & lgr, const void *p) const {
    lgr << name() << " " << *reinterpret_cast<const int32_t *>(p);
}

void
Int32Type::logLiteral(TextLogger & lgr, const Literal *lv) const {
    lgr << name() << "(" << (lv->value<const int32_t>()) << ")";
}

const int64_t
Int32Type::getInteger(const Literal *lv) const {
    return (const int64_t) (lv->value<const int32_t>());
}


INIT_JBALLOC_REUSECAT(Int64Type, IntegerType)
SUBCLASS_KINDSERVICE_IMPL(Int64Type, "Int64Type", IntegerType, Type);

Int64Type::Int64Type(Allocator *a, const Int64Type *source, IRCloner *cloner)
    : IntegerType(a, source, cloner) {

}

const Type *
Int64Type::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) Int64Type(a, this, cloner);
}

Int64Type::~Int64Type() {

}

Literal *
Int64Type::literal(LOCATION, IR *ir, const int64_t value) const {
    return this->Type::literal(PASSLOC, ir, reinterpret_cast<const LiteralBytes *>(&value));
}

bool
Int64Type::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<const int64_t *>(l1)) == (*reinterpret_cast<const int64_t *>(l2));
}

void
Int64Type::logValue(TextLogger & lgr, const void *p) const {
    lgr << name() << " " << *reinterpret_cast<const int64_t *>(p);
}

void
Int64Type::logLiteral(TextLogger & lgr, const Literal *lv) const {
    lgr << name() << "(" << (lv->value<const int64_t>()) << ")";
}

const int64_t
Int64Type::getInteger(const Literal *lv) const {
    return (const int64_t) (lv->value<const int64_t>());
}


INIT_JBALLOC_REUSECAT(FloatingPointType, NumericType)
SUBCLASS_KINDSERVICE_IMPL(FloatingPointType, "FloatingPointType", IntegerType, Type);

FloatingPointType::~FloatingPointType() {

}

INIT_JBALLOC_REUSECAT(Float32Type, FloatingPointType)
SUBCLASS_KINDSERVICE_IMPL(Float32Type, "Float32Type", FloatingPointType, Type);

Float32Type::Float32Type(Allocator *a, const Float32Type *source, IRCloner *cloner)
    : FloatingPointType(a, source, cloner) {

}

const Type *
Float32Type::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) Float32Type(a, this, cloner);
}

Float32Type::~Float32Type() {

}

Literal *
Float32Type::literal(LOCATION, IR *ir, const float value) const {
    return this->Type::literal(PASSLOC, ir, reinterpret_cast<const LiteralBytes *>(&value));
}

bool
Float32Type::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<const float *>(l1)) == (*reinterpret_cast<const float *>(l2));
}

void
Float32Type::logValue(TextLogger & lgr, const void *p) const {
    lgr << name() << " " << *reinterpret_cast<const float *>(p);
}

void
Float32Type::logLiteral(TextLogger & lgr, const Literal *lv) const {
    lgr << name() << "(" << (lv->value<const float>()) << ")";
}

const double
Float32Type::getFloatingPoint(const Literal *lv) const {
    return (const double) (lv->value<const float>());
}


INIT_JBALLOC_REUSECAT(Float64Type, FloatingPointType)
SUBCLASS_KINDSERVICE_IMPL(Float64Type, "Float64Type", FloatingPointType, Type);

Float64Type::Float64Type(Allocator *a, const Float64Type *source, IRCloner *cloner)
    : FloatingPointType(a, source, cloner) {

}

const Type *
Float64Type::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) Float64Type(a, this, cloner);
}

Float64Type::~Float64Type() {

}

Literal *
Float64Type::literal(LOCATION, IR *ir, const double value) const {
    return this->Type::literal(PASSLOC, ir, reinterpret_cast<const LiteralBytes *>(&value));
}

bool
Float64Type::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<const double *>(l1)) == (*reinterpret_cast<const double *>(l2));
}

void
Float64Type::logValue(TextLogger & lgr, const void *p) const {
    lgr << name() << " " << *reinterpret_cast<const double *>(p);
}

void
Float64Type::logLiteral(TextLogger & lgr, const Literal *lv) const {
    lgr << name() << "(" << (lv->value<const double>()) << ")";
}

const double
Float64Type::getFloatingPoint(const Literal *lv) const {
    return (const double) (lv->value<const double>());
}


INIT_JBALLOC_REUSECAT(AddressType, BaseType)
SUBCLASS_KINDSERVICE_IMPL(AddressType, "AddressType", IntegerType, Type);

AddressType::AddressType(Allocator *a, const AddressType *source, IRCloner *cloner)
    : BaseType(a, source, cloner) {

}

const Type *
AddressType::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) AddressType(a, this, cloner);
}

AddressType::~AddressType() {

}

AddressType::AddressType(MEM_LOCATION(a), Extension *ext)
    : BaseType(MEM_PASSLOC(a), getTypeClassKind(), ext, "Address", ext->compiler()->platformWordSize()) {

}

AddressType::AddressType(MEM_LOCATION(a), Extension *ext, String name)
    : BaseType(MEM_PASSLOC(a), getTypeClassKind(), ext, name, ext->compiler()->platformWordSize() ) {
}

AddressType::AddressType(MEM_LOCATION(a), Extension *ext, TypeDictionary *dict, String name)
    : BaseType(MEM_PASSLOC(a), getTypeClassKind(), ext, dict, name, dict->compiler()->platformWordSize() ) {
}

AddressType::AddressType(MEM_LOCATION(a), Extension *ext, TypeDictionary *dict, TypeKind kind, String name)
    : BaseType(MEM_PASSLOC(a), kind, ext, dict, name, dict->compiler()->platformWordSize() ) {
}

Literal *
AddressType::literal(LOCATION, IR *ir, const void * value) const {
    return this->Type::literal(PASSLOC, ir, reinterpret_cast<const LiteralBytes *>(&value));
}
bool
AddressType::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<void * const *>(l1)) == (*reinterpret_cast<void * const *>(l2));
}

void
AddressType::logValue(TextLogger & lgr, const void *p) const {
    lgr << name() << " " << *(reinterpret_cast<const void * const *>(p));
}

void
AddressType::logLiteral(TextLogger & lgr, const Literal *lv) const {
    lgr << name() << "(" << (lv->value<void * const>()) << ")";
}


INIT_JBALLOC(PointerTypeBuilder)

PointerTypeBuilder::PointerTypeBuilder(Allocator *a, BaseExtension *ext, Compilation *comp)
    : Allocatable(a)
    , _ext(ext)
    , _ir(comp->ir())
    , _dict(_ir->typedict())
    , _baseType(NULL)
    , _helper(NULL) {

}

PointerTypeBuilder::PointerTypeBuilder(BaseExtension *ext, Compilation *comp)
    : Allocatable()
    ,  _ext(ext)
    , _ir(comp->ir())
    , _dict(_ir->typedict())
    , _baseType(NULL)
    , _helper(NULL) {

}

PointerTypeBuilder::PointerTypeBuilder(Allocator *a, BaseExtension *ext, IR *ir)
    : Allocatable(a)
    , _ext(ext)
    , _ir(ir)
    , _dict(ir->typedict())
    , _baseType(NULL)
    , _helper(NULL) {

}

PointerTypeBuilder::PointerTypeBuilder(BaseExtension *ext, IR *ir)
    : Allocatable()
    ,  _ext(ext)
    , _ir(ir)
    , _dict(ir->typedict())
    , _baseType(NULL)
    , _helper(NULL) {

}

PointerTypeBuilder::~PointerTypeBuilder() {

}

const PointerType *
PointerTypeBuilder::create(LOCATION) {
    const PointerType *existingType = _ir->addon<BaseIRAddon>()->pointerTypeFromBaseType(_baseType);
    if (existingType != NULL)
        return existingType;

    Allocator *mem = _ir->mem();
    const PointerType *newType = new (mem) PointerType(MEM_PASSLOC(mem), this);
    return newType;
}


INIT_JBALLOC_REUSECAT(PointerType, BaseType)
SUBCLASS_KINDSERVICE_IMPL(PointerType, "PointerType", IntegerType, Type);

PointerType::PointerType(Allocator *a, const PointerType *source, IRCloner *cloner)
    : AddressType(a, source, cloner)
    , _baseType(cloner->clonedType(source->_baseType)) {

}

const Type *
PointerType::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) PointerType(a, this, cloner);
}

PointerType::~PointerType() {

}

PointerType::PointerType(MEM_LOCATION(a), PointerTypeBuilder *builder)
    : AddressType(MEM_PASSLOC(a), builder->extension(), builder->dict(), getTypeClassKind(), builder->name()) {

    if (builder->helper())
        builder->helper()(this, builder);
    _baseType = builder->baseType();
    assert(_baseType);

    builder->ir()->addon<BaseIRAddon>()->registerPointerType(this);
}

Literal *
PointerType::literal(LOCATION, IR *ir, const void * value) const {
    return this->Type::literal(PASSLOC, ir, reinterpret_cast<const LiteralBytes *>(&value));
}

bool
PointerType::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return (*reinterpret_cast<void * const *>(l1)) == (*reinterpret_cast<void * const *>(l2));
}

String
PointerType::to_string(bool useHeader) const {
    String s(Type::base_string(useHeader));
    return s.append(String(" pointerType base t")).append(String::to_string(_baseType->id()));
}

void
PointerType::logValue(TextLogger & lgr, const void *p) const {
    lgr << name() << " " << *(reinterpret_cast<const void * const *>(p));
}

void
PointerType::logLiteral(TextLogger & lgr, const Literal *lv) const {
    lgr << name() << "(" << (lv->value<void * const>()) << ")";
}

const Type *
PointerType::replace(TypeReplacer *repl) {
    const Type *currentBaseType = baseType();
    const Type *newBaseType = repl->replacedType(currentBaseType);
    Compilation *comp = repl->comp();
    const Type *newPtrType = baseExt()->PointerTo(LOC, comp, newBaseType);
    return newPtrType;
}


INIT_JBALLOC_REUSECAT(FieldType, BaseType)
SUBCLASS_KINDSERVICE_IMPL(FieldType, "FieldType", BaseType, Type);

FieldType::FieldType(Allocator *a, const FieldType *source, IRCloner *cloner)
    : BaseType(a, source, cloner)
    , _structType(cloner->clonedType(source->_structType)->refine<StructType>())
    , _fieldName(source->_name)
    , _type(cloner->clonedType(source->_type))
    , _offset(source->_offset) {

}

const Type *
FieldType::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) FieldType(a, this, cloner);
}

FieldType::FieldType(MEM_LOCATION(a), BaseExtension *ext, TypeDictionary *dict, const StructType *structType, String name, const Type *type, size_t offset)
    : BaseType(MEM_PASSLOC(a), getTypeClassKind(), ext, dict, name, type->size())
    , _structType(structType)
    , _fieldName(name)
    , _type(type)
    , _offset(offset) {

}

FieldType::~FieldType() {

}

String
FieldType::to_string(bool useHeader) const {
    String s(Type::base_string(useHeader));
    s.append(String(" fieldType ")).append(_fieldName);
    s.append(String(" size ")).append(String::to_string(_type->size()));
    s.append(String(" t")).append(String::to_string(_type->id()));
    s.append(String("@")).append(String::to_string(_offset));
    return s;
}

String
FieldType::explodedName(TypeReplacer *repl, String & baseName) const {
    String fName = _fieldName;
    if (fName == _type->name())
        fName = repl->replacedType(_type)->name();
    if (baseName.length() > 0)
        return baseName + "." + fName;
    return fName;
}

INIT_JBALLOC(StructTypeBuilder)

StructTypeBuilder::StructTypeBuilder(Allocator *a, BaseExtension *ext, Compilation *comp)
    : Allocatable(a)
    , _ext(ext)
    , _ir(comp->ir())
    , _unit(_ir->unit())
    , _dict(_ir->typedict())
    , _size(0)
    , _fields(NULL, a)
    , _helper(NULL) {

}

StructTypeBuilder::StructTypeBuilder(BaseExtension *ext, Compilation *comp)
    : Allocatable()
    , _ext(ext)
    , _ir(comp->ir())
    , _unit(_ir->unit())
    , _dict(_ir->typedict())
    , _size(0)
    , _fields(NULL, _ir->mem())
    , _helper(NULL) {

}

StructTypeBuilder::StructTypeBuilder(Allocator *a, BaseExtension *ext, IR *ir)
    : Allocatable(a)
    , _ext(ext)
    , _ir(ir)
    , _unit(ir->unit())
    , _dict(ir->typedict())
    , _size(0)
    , _fields(NULL, a)
    , _helper(NULL) {

}

StructTypeBuilder::StructTypeBuilder(BaseExtension *ext, IR *ir)
    : Allocatable()
    , _ext(ext)
    , _ir(ir)
    , _unit(ir->unit())
    , _dict(ir->typedict())
    , _size(0)
    , _fields(NULL, ir->mem())
    , _helper(NULL) {

}

StructTypeBuilder::~StructTypeBuilder() {

}

void
StructTypeBuilder::createFields(MEM_LOCATION(a)) {
    for (auto it = _fields.iterator(); it.hasItem(); it++) {
        FieldInfo info = it.item();
        _structType->addField(MEM_PASSLOC(a), _ext, _dict, info._name, info._type, info._offset);
    }
}

bool
StructTypeBuilder::verifyFields(const StructType *sType) {
    // todo: check fields match!
    return true;
}

const StructType *
StructTypeBuilder::create(LOCATION) {
    const StructType *existingType = _ir->addon<BaseIRAddon>()->structTypeFromName(_name);
    if (existingType != NULL) {
        if (verifyFields(existingType))
            return existingType;
        // error code?
        return NULL;
    }

    Allocator *mem = _ir->mem();
    const StructType *newType = new (mem) StructType(MEM_PASSLOC(mem), this);
    return newType;
}


INIT_JBALLOC_REUSECAT(StructType, BaseType)
SUBCLASS_KINDSERVICE_IMPL(StructType, "StructType", BaseType, Type);

StructType::StructType(MEM_LOCATION(a), StructTypeBuilder *builder)
    : BaseType(MEM_PASSLOC(a), getTypeClassKind(), builder->extension(), builder->dict(), builder->name(), builder->size())
    , _structSize(0) {

    if (builder->helper())
        builder->helper()(this, builder);
    builder->setStructType(this);
    builder->createFields(MEM_PASSLOC(a));
    builder->ir()->addon<BaseIRAddon>()->registerStructType(this);
}

StructType::StructType(Allocator *a, const StructType *source, IRCloner *cloner)
    : BaseType(a, source, cloner)
    , _structSize(source->_structSize) {

    for (auto it = source->FieldsBegin(); it != source->FieldsEnd(); it++) {
        const FieldType *ft = it->second;
        const FieldType *cloned_ft = cloner->clonedType(ft)->refine<FieldType>();
        _fieldsByName.insert({cloned_ft->name(), cloned_ft});
        _fieldsByOffset.insert({cloned_ft->offset(), cloned_ft});
    }
}

const Type *
StructType::clone(Allocator *a, IRCloner *cloner) const {
    return new (a) StructType(a, this, cloner);
}

StructType::~StructType() {

}

const FieldType *
StructType::addField(MEM_LOCATION(a), Extension *ext, TypeDictionary *dict, String name, const Type *type, size_t offset) {
    const FieldType *preExistingField = LookupField(name);
    if (preExistingField) {
        if (preExistingField->type() == type && preExistingField->offset() == offset)
            return preExistingField;
        return NULL;
    }

    FieldType *field = new (a) FieldType(MEM_PASSLOC(a), baseExt(), dict, this, name, type, offset);
    _fieldsByName.insert({name, field});
    _fieldsByOffset.insert({offset, field});

    if (_structSize < offset + type->size())
        _structSize = offset + type->size();

    return field;
}

String
StructType::to_string(bool useHeader) const {
    String s(Type::base_string(useHeader));
    s.append(String(" structType size ")).append(String::to_string(size()));
    for (auto it = FieldsBegin(); it != FieldsEnd(); it++) {
        auto field = it->second;
        s.append(String(" t")).append(String::to_string(field->id()));
        s.append(String("@")).append(String::to_string(field->offset()));
    }
    return s;
}

Literal *
StructType::literal(LOCATION, IR *ir, const LiteralBytes * structValue) const {
    return this->Type::literal(PASSLOC, ir, structValue);
}

bool
StructType::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
    return memcmp(l1, l2, size()/8) == 0;
}

void
StructType::logValue(TextLogger & lgr, const void *p) const {
    // TODO
}

void
StructType::logLiteral(TextLogger & lgr, const Literal *lv) const {
    // TODO
}

void
StructType::explodeAsLayout(TypeReplacer *repl, size_t baseOffset, TypeMapper *m) const {
    for (auto fIt = this->FieldsBegin(); fIt != this->FieldsEnd(); fIt++) {
        const FieldType *fType = fIt->second;
        const Type *t = fType->type();
        transformTypeIfNeeded(repl, t);
 
        size_t fieldOffset = baseOffset + fType->offset();
        if (repl->isExploded(t)) {
            //if (_typesToExplode.find(t->id()) != _typesToExplode.end())
            const StructType *innerLayout = static_cast<const StructType *>(t->layout());
            assert(innerLayout);
            innerLayout->explodeAsLayout(repl, fieldOffset, m);
        }
        else {
            const Type *mappedType = repl->replacedType(t);
            String fieldName = mappedType->name();
            m->add(mappedType, fieldName, fieldOffset);
        }
    }
}
 
void
StructType::transformFields(TypeReplacer *repl,
                            StructTypeBuilder *stb,
                            StructType *origStruct,
                            String baseName,
                            size_t baseOffset) const {

    bool removeFields = true;
    if (origStruct == this && repl->isRemovedType(origStruct))
       removeFields = false;

    for (auto fIt = FieldsBegin(); fIt != FieldsEnd(); fIt++) {
        const FieldType *fType = fIt->second;
        String fieldName = fType->explodedName(repl, baseName);
        const Type *t = fType->type();
        repl->transformTypeIfNeeded(t);

        if (repl->isExploded(t)) {
            const StructType *layout = static_cast<const StructType *>(t->layout());
            layout->transformFields(repl, stb, origStruct, fieldName, fType->offset());
        }
        else {
            const Type *mappedType = repl->replacedType(t);
            stb->addField(fieldName, mappedType, baseOffset + fType->offset());
            if (removeFields)
                repl->removeType(fType);
        }
    }
}

void
StructType::mapTransformedFields(TypeReplacer *repl,
                                 const StructType *type,
                                 String baseName,
                                 TypeMapper *mapper) const {

    Allocator *mem = mapper->allocator();
    for (auto fIt = FieldsBegin(); fIt != FieldsEnd(); fIt++) {
        const FieldType *fType = fIt->second;
        String fieldName = fType->explodedName(repl, baseName);
        const Type *t = fType->type();
        if (repl->isExploded(t)) {
            TypeMapper *m = new (mem) TypeMapper(mem);
            const StructType *layout = static_cast<const StructType *>(t->layout());
            layout->mapTransformedFields(repl, type, fieldName, m);
            repl->recordMapper(fType, m);
        }
        else {
            const FieldType *newType = type->LookupField(fieldName);
            repl->recordMapper(fType, new (mem) TypeMapper(mem, newType));
            if (mapper)
                mapper->add(newType);
        }
    }
}

const Type *
StructType::replace(TypeReplacer *repl) {
    Base::BaseExtension *base = baseExt();

    bool needToReplace = false;
    if (repl->isReplacedType(this)) {
        needToReplace = true;
    } else {
        for (auto it = FieldsBegin(); it != FieldsEnd();it++) {
            const FieldType *fieldType = it->second;
            const Type *fType = fieldType->type();
            if (repl->isModified(fType) || repl->isExploded(fType))
                needToReplace = true;
        }
    }

    if (!needToReplace) 
        return this;

    String newName = String("_X_::").append(name());
    StructTypeBuilder stb(base, repl->comp()->ir());
    stb.setName(newName)
       ->setSize(this->size());

    // TODO: don't believe this works for recursive Types...need to move this work into the Helper function

    String baseName("");
    transformFields(repl, &stb, this, baseName, 0);

    const StructType *newType = stb.create(LOC);

    mapTransformedFields(repl, this, baseName, NULL);

    return newType;
}

#if NEED_UNION
void
UnionType::printValue(TextLogger &w, const void *p) const {
    // TODO
}
#endif

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

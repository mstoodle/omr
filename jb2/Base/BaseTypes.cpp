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
namespace JB2 {
namespace Base {

#define DEFINE_BASETYPE_CLASS(C,Super,user_code) \
    INIT_JBALLOC_REUSECAT(C, Type) \
    SUBCLASS_KINDSERVICE_IMPL(C, #C, Super, Extensible); \
    C::C(MEM_LOCATION(a), ExtensibleKind kind, Extension *ext, String name, size_t size) \
        : Super(MEM_PASSLOC(a), kind, ext, name, size) { } \
    C::C(MEM_LOCATION(a), ExtensibleKind kind, Extension *ext, IR *ir, String name, size_t size) \
        : Super(MEM_PASSLOC(a), kind, ext, ir, name, size) { } \
    C::C(MEM_LOCATION(a), ExtensibleKind kind, Extension *ext, IR *ir, TypeID tid, String name, size_t size) \
        : Super(MEM_PASSLOC(a), kind, ext, ir, tid, name, size) { } \
    C::C(Allocator *a, const C *source, IRCloner *cloner) \
        : Super(a, source, cloner) { } \
    C::~C() { } \
    user_code

#define DEFINE_ABSTRACT_BASETYPE_CLASS(C,Super,user_code) \
    DEFINE_BASETYPE_CLASS(C,Super, \
        const Type * C::cloneType(Allocator *a, IRCloner *cloner) const { assert(0); return NULL; } \
        void C::logValue(TextLogger &lgr, const void *p) const { assert(0); } \
        void C::logLiteral(TextLogger & lgr, const Literal *lv) const { assert(0); } \
        bool C::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const { return false; } \
        user_code \
    )


#define DEFINE_CONCRETE_BASETYPE_CLASS(C,Name,Super,size,user_code) \
    DEFINE_BASETYPE_CLASS(C,Super, \
        C::C(MEM_LOCATION(a), Extension *ext) : Super(MEM_PASSLOC(a), getExtensibleClassKind(), ext, #Name, size) { } \
        C::C(MEM_LOCATION(a), Extension *ext, IR *ir) : Super(MEM_PASSLOC(a), getExtensibleClassKind(), ext, ir, #Name, size) { } \
        C::C(MEM_LOCATION(a), Extension *ext, IR *ir, TypeID tid) : Super(MEM_PASSLOC(a), getExtensibleClassKind(), ext, ir, tid, #Name, size) { } \
        const Type * \
        C::cloneType(Allocator *a, IRCloner *cloner) const { \
            assert(_kind == KIND(Extensible)); \
            return new (a) C(a, this, cloner); \
        } \
        user_code \
    )


DEFINE_ABSTRACT_BASETYPE_CLASS(BaseType, Type,
    BaseExtension *
    BaseType::baseExt() {
        return static_cast<BaseExtension *>(_ext);
    }

    BaseExtension *
    BaseType::baseExt() const {
        return static_cast<BaseExtension * const>(_ext);
    }
)

DEFINE_ABSTRACT_BASETYPE_CLASS(NumericType, BaseType,)

//
// Integer types
//

DEFINE_ABSTRACT_BASETYPE_CLASS(IntegerType, NumericType,)

#define DEFINE_INTTYPE_CLASS(size) \
    DEFINE_CONCRETE_BASETYPE_CLASS(Int ## size ## Type, Int ## size, IntegerType, size, \
        Literal * \
        Int ## size ## Type::zero(LOCATION) const { \
            return literal(PASSLOC, (int ## size ## _t) 0); \
        } \
        Literal * \
        Int ## size ## Type::identity(LOCATION) const { \
            return literal(PASSLOC, (int ## size ## _t) 1); \
        } \
        Literal * \
        Int ## size ## Type::literal(LOCATION, const int ## size ## _t value) const { \
            return this->Type::literal(PASSLOC, reinterpret_cast<const LiteralBytes *>(&value)); \
        } \
        bool \
        Int ## size ## Type::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const { \
            return (*reinterpret_cast<const int ## size ## _t *>(l1)) == (*reinterpret_cast<const int ## size ## _t *>(l2)); \
        } \
        void \
        Int ## size ## Type::logValue(TextLogger & lgr, const void *p) const { \
            lgr << name() << " " << ((int) *reinterpret_cast<const int ## size ## _t *>(p)); \
        } \
        void \
        Int ## size ## Type::logLiteral(TextLogger & lgr, const Literal *lv) const { \
            lgr << name() << "(" << ((int) lv->value<const int ## size ## _t>()) << ")"; \
        } \
        const int64_t \
        Int ## size ## Type::getInteger(const Literal *lv) const { \
            return (const int64_t) (lv->value<const int ## size ## _t>()); \
        } \
    )

DEFINE_INTTYPE_CLASS(8)
DEFINE_INTTYPE_CLASS(16)
DEFINE_INTTYPE_CLASS(32)
DEFINE_INTTYPE_CLASS(64)

//
// Floating point types
//

DEFINE_ABSTRACT_BASETYPE_CLASS(FloatingPointType, NumericType,)

#define DEFINE_FLOATTYPE_CLASS(size, ctype) \
    DEFINE_CONCRETE_BASETYPE_CLASS(Float ## size ## Type, Float ## size, FloatingPointType, size, \
        Literal * \
        Float ## size ## Type::zero(LOCATION) const { \
            return literal(PASSLOC, (ctype) 0.0); \
        } \
        Literal * \
        Float ## size ## Type::identity(LOCATION) const { \
            return literal(PASSLOC, (ctype) 1.0); \
        } \
        Literal * \
        Float ## size ## Type::literal(LOCATION, const ctype value) const { \
            return this->Type::literal(PASSLOC, reinterpret_cast<const LiteralBytes *>(&value)); \
        } \
        bool \
        Float ## size ## Type::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const { \
            return (*reinterpret_cast<const ctype *>(l1)) == (*reinterpret_cast<const ctype *>(l2)); \
        } \
        void \
        Float ## size ## Type::logValue(TextLogger & lgr, const void *p) const { \
            lgr << name() << " " << ((int) *reinterpret_cast<const ctype *>(p)); \
        } \
        void \
        Float ## size ## Type::logLiteral(TextLogger & lgr, const Literal *lv) const { \
            lgr << name() << "(" << ((int) lv->value<const ctype>()) << ")"; \
        } \
        const double \
        Float ## size ## Type::getFloatingPoint(const Literal *lv) const { \
            return (const int64_t) (lv->value<const ctype>()); \
        } \
    )

DEFINE_FLOATTYPE_CLASS(32, float)
DEFINE_FLOATTYPE_CLASS(64, double)


DEFINE_CONCRETE_BASETYPE_CLASS(AddressType, Address, IntegerType, ext->compiler()->platformWordSize(),
    Literal *
    AddressType::zero(LOCATION) const {
        return literal(PASSLOC, (const void *) 0);
    }

    Literal *
    AddressType::identity(LOCATION) const {
        assert(0);
        return NULL;
    }

    Literal *
    AddressType::literal(LOCATION, const void * value) const {
        return this->Type::literal(PASSLOC, reinterpret_cast<const LiteralBytes *>(&value));
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
)

INIT_JBALLOC(PointerTypeBuilder)

PointerTypeBuilder::PointerTypeBuilder(Allocator *a, BaseExtension *ext, Compilation *comp)
    : Allocatable(a)
    , _ext(ext)
    , _ir(comp->ir())
    , _baseType(NULL)
    , _helper(NULL) {

}

PointerTypeBuilder::PointerTypeBuilder(BaseExtension *ext, Compilation *comp)
    : Allocatable()
    ,  _ext(ext)
    , _ir(comp->ir())
    , _baseType(NULL)
    , _helper(NULL) {

}

PointerTypeBuilder::PointerTypeBuilder(Allocator *a, BaseExtension *ext, IR *ir)
    : Allocatable(a)
    , _ext(ext)
    , _ir(ir)
    , _baseType(NULL)
    , _helper(NULL) {

}

PointerTypeBuilder::PointerTypeBuilder(BaseExtension *ext, IR *ir)
    : Allocatable()
    ,  _ext(ext)
    , _ir(ir)
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


DEFINE_CONCRETE_BASETYPE_CLASS(PointerType, Pointer, AddressType, ext->compiler()->platformWordSize(),
    PointerType::PointerType(MEM_LOCATION(a), PointerTypeBuilder *builder)
        : AddressType(MEM_PASSLOC(a), getExtensibleClassKind(), builder->extension(), builder->ir(), builder->name(), builder->extension()->compiler()->platformWordSize()) {

        if (builder->helper())
            builder->helper()(this, builder);
        _baseType = builder->baseType();
        assert(_baseType);

        builder->ir()->addon<BaseIRAddon>()->registerPointerType(this);
    }

    Literal *
    PointerType::zero(LOCATION) const {
        return literal(PASSLOC, (const void *) 0);
    }

    Literal *
    PointerType::identity(LOCATION) const {
        assert(0);
        return NULL;
    }

    Literal *
    PointerType::literal(LOCATION, const void * value) const {
        return this->Type::literal(PASSLOC, reinterpret_cast<const LiteralBytes *>(&value));
    }

    bool
    PointerType::literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const {
        return (*reinterpret_cast<void * const *>(l1)) == (*reinterpret_cast<void * const *>(l2));
    }

    String
    PointerType::to_string(Allocator *mem, bool useHeader) const {
        String s(Type::base_string(mem, useHeader));
        return s.append(String(mem, " pointerType base t")).append(String::to_string(mem, _baseType->id()));
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
        const Type *newPtrType = baseExt()->PointerTo(LOC, newBaseType);
        return newPtrType;
    }
)



INIT_JBALLOC_REUSECAT(FieldType, BaseType)
SUBCLASS_KINDSERVICE_IMPL(FieldType, "FieldType", BaseType, Extensible);

FieldType::FieldType(Allocator *a, const FieldType *source, IRCloner *cloner)
    : BaseType(a, source, cloner)
    , _structType(cloner->clonedType(source->_structType)->refine<StructType>())
    , _fieldName(source->_name)
    , _type(cloner->clonedType(source->_type))
    , _offset(source->_offset) {

}

const Type *
FieldType::cloneType(Allocator *a, IRCloner *cloner) const {
    return new (a) FieldType(a, this, cloner);
}

FieldType::FieldType(MEM_LOCATION(a), BaseExtension *ext, const StructType *structType, String name, const Type *type, size_t offset)
    : BaseType(MEM_PASSLOC(a), getExtensibleClassKind(), ext, type->ir(), name, type->size())
    , _structType(structType)
    , _fieldName(name)
    , _type(type)
    , _offset(offset) {

}

FieldType::~FieldType() {

}

String
FieldType::to_string(Allocator *mem, bool useHeader) const {
    String s(Type::base_string(mem, useHeader));
    s.append(String(mem, "fieldType "));
    s.append(String(mem, " t")).append(String::to_string(mem, _type->id()));
    s.append(String(mem, "@")).append(String::to_string(mem, _offset));
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
    , _size(0)
    , _fields(NULL, a)
    , _helper(NULL) {

}

StructTypeBuilder::StructTypeBuilder(BaseExtension *ext, Compilation *comp)
    : Allocatable()
    , _ext(ext)
    , _ir(comp->ir())
    , _unit(_ir->unit())
    , _size(0)
    , _fields(NULL, _ir->mem())
    , _helper(NULL) {

}

StructTypeBuilder::StructTypeBuilder(Allocator *a, BaseExtension *ext, IR *ir)
    : Allocatable(a)
    , _ext(ext)
    , _ir(ir)
    , _unit(ir->unit())
    , _size(0)
    , _fields(NULL, a)
    , _helper(NULL) {

}

StructTypeBuilder::StructTypeBuilder(BaseExtension *ext, IR *ir)
    : Allocatable()
    , _ext(ext)
    , _ir(ir)
    , _unit(ir->unit())
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
        _structType->addField(MEM_PASSLOC(a), _ext, info._name, info._type, info._offset);
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
SUBCLASS_KINDSERVICE_IMPL(StructType, "StructType", BaseType, Extensible);

StructType::StructType(MEM_LOCATION(a), StructTypeBuilder *builder)
    : BaseType(MEM_PASSLOC(a), getExtensibleClassKind(), builder->extension(), builder->ir(), builder->name(), builder->size())
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
StructType::cloneType(Allocator *a, IRCloner *cloner) const {
    return new (a) StructType(a, this, cloner);
}

StructType::~StructType() {

}

const FieldType *
StructType::addField(MEM_LOCATION(a), Extension *ext, String name, const Type *type, size_t offset) {
    const FieldType *preExistingField = LookupField(name);
    if (preExistingField) {
        if (preExistingField->type() == type && preExistingField->offset() == offset)
            return preExistingField;
        return NULL;
    }

    FieldType *field = new (a) FieldType(MEM_PASSLOC(a), baseExt(), this, name, type, offset);
    _fieldsByName.insert({name, field});
    _fieldsByOffset.insert({offset, field});

    if (_structSize < offset + type->size())
        _structSize = offset + type->size();

    return field;
}

String
StructType::to_string(Allocator *mem, bool useHeader) const {
    String s(Type::base_string(mem, useHeader));
    s.append(String(mem, " structType size ")).append(String::to_string(mem, size()));
    for (auto it = FieldsBegin(); it != FieldsEnd(); it++) {
        auto field = it->second;
        s.append(String(mem, " t")).append(String::to_string(mem, field->id()));
        s.append(String(mem, "@")).append(String::to_string(mem, field->offset()));
    }
    return s;
}

Literal *
StructType::literal(LOCATION, const LiteralBytes * structValue) const {
    return this->Type::literal(PASSLOC, structValue);
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

    String newName = String(allocator(), "_X_::").append(name());
    StructTypeBuilder stb(base, repl->comp()->ir());
    stb.setName(newName)
       ->setSize(this->size());

    // TODO: don't believe this works for recursive Types...need to move this work into the Helper function

    String baseName(allocator(), "");
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
} // namespace JB2
} // namespace OMR

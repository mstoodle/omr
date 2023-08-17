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

#ifndef BASETYPES_INCL
#define BASETYPES_INCL

#include <map>
#include "JBCore.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

class BaseExtension;

class BaseType : public Type {
    JBALLOC_(BaseType)

    friend class BaseExtension;

    protected:
    BaseType(MEM_LOCATION(a), TypeKind kind, Extension *ext, String name, size_t size)
        : Type(MEM_PASSLOC(a), kind, ext, name, size) {

    }
    BaseType(MEM_LOCATION(a), TypeKind kind, Extension *ext, TypeDictionary *dict, String name, size_t size)
        : Type(MEM_PASSLOC(a), kind, ext, dict, name, size) {

    }
    BaseType(Allocator *a, const BaseType *source, IRCloner *cloner)
        : Type(a, source, cloner) {

    }

    BaseExtension *baseExt();
    BaseExtension *baseExt() const;

    SUBCLASS_KINDSERVICE_DECL(Type, BaseType);
};

class NumericType : public BaseType {
    JBALLOC_(NumericType)

    friend class BaseExtension;

protected:
    NumericType(MEM_LOCATION(a), TypeKind kind, Extension *ext, String name, size_t size)
        : BaseType(MEM_PASSLOC(a), kind, ext, name, size) {

    }
    NumericType(Allocator *a, const NumericType *source, IRCloner *cloner)
        : BaseType(a, source, cloner) {

    }

    SUBCLASS_KINDSERVICE_DECL(Type, NumericType);
};

class IntegerType : public NumericType {
    JBALLOC_(IntegerType)

    friend class BaseExtension;
public:
    virtual bool isInteger() const { return true; }

protected:
    IntegerType(MEM_LOCATION(a), TypeKind kind, Extension *ext, String name, size_t size)
        : NumericType(MEM_PASSLOC(a), kind, ext, name, size) {

    }
    IntegerType(Allocator *a, const IntegerType *source, IRCloner *cloner)
        : NumericType(a, source, cloner) {

    }

    SUBCLASS_KINDSERVICE_DECL(Type, IntegerType);
};

class Int8Type : public IntegerType {
    JBALLOC_(Int8Type)

    friend class BaseExtension;

public:
    virtual size_t size() const { return 8; }
    virtual Literal *literal(LOCATION, IR *ir, const int8_t value) const;
    virtual Literal *zero(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 0); }
    virtual Literal *identity(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 1); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;
    virtual const int64_t getInteger(const Literal *lv) const;

protected:
    Int8Type(MEM_LOCATION(a), Extension *ext) : IntegerType(MEM_PASSLOC(a), getTypeClassKind(), ext, "Int8", 8) { }
    Int8Type(Allocator *a, const Int8Type *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Type, Int8Type);
};

class Int16Type : public IntegerType {
    JBALLOC_(Int16Type)

    friend class BaseExtension;

public:
    virtual size_t size() const { return 16; }
    virtual Literal *literal(LOCATION, IR *ir, const int16_t value) const;
    virtual Literal *zero(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 0); }
    virtual Literal *identity(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 1); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;
    virtual const int64_t getInteger(const Literal *lv) const;

protected:
    Int16Type(MEM_LOCATION(a), Extension *ext) : IntegerType(MEM_PASSLOC(a), getTypeClassKind(), ext, "Int16", 16) { }
    Int16Type(Allocator *a, const Int16Type *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Type, Int16Type);
};

class Int32Type : public IntegerType {
    JBALLOC_(Int32Type)

    friend class BaseExtension;

public:
    virtual size_t size() const { return 32; }
    virtual Literal *literal(LOCATION, IR *ir, const int32_t value) const;
    virtual Literal *zero(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 0); }
    virtual Literal *identity(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 1); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;
    virtual const int64_t getInteger(const Literal *lv) const;

protected:
    Int32Type(MEM_LOCATION(a), Extension *ext) : IntegerType(MEM_PASSLOC(a), getTypeClassKind(), ext, "Int32", 32) { }
    Int32Type(Allocator *a, const Int32Type *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Type, Int32Type);
};

class Int64Type : public IntegerType {
    JBALLOC_(Int64Type)

    friend class BaseExtension;

public:
    virtual size_t size() const { return 64; }
    virtual Literal *literal(LOCATION, IR *ir, const int64_t value) const;
    virtual Literal *zero(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 0); }
    virtual Literal *identity(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 1); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;
    virtual const int64_t getInteger(const Literal *lv) const;

protected:
    Int64Type(MEM_LOCATION(a), Extension *ext) : IntegerType(MEM_PASSLOC(a), getTypeClassKind(), ext, "Int64", 64) { }
    Int64Type(Allocator *a, const Int64Type *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Type, Int64Type);
};

class FloatingPointType : public NumericType {
    JBALLOC_(FloatingPointType)

    friend class BaseExtension;

protected:
    FloatingPointType(MEM_LOCATION(a), TypeKind kind, Extension *ext, String name, size_t size)
        : NumericType(MEM_PASSLOC(a), kind, ext, name, size) {

    }
    FloatingPointType(Allocator *a, const FloatingPointType *source, IRCloner *cloner)
        : NumericType(a, source, cloner) {

    }

    SUBCLASS_KINDSERVICE_DECL(Type, FloatingPointType);
};

class Float32Type : public FloatingPointType {
    JBALLOC_(Float32Type)

    friend class BaseExtension;

public:
    virtual size_t size() const { return 32; }
    virtual Literal *literal(LOCATION, IR *ir, const float value) const;
    virtual Literal *zero(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 0.0); }
    virtual Literal *identity(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 1.0); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;
    virtual const double getFloatingPoint(const Literal *lv) const;

protected:
    Float32Type(MEM_LOCATION(a), Extension *ext) : FloatingPointType(MEM_PASSLOC(a), getTypeClassKind(), ext, "Float32", 32) { }
    Float32Type(Allocator *a, const Float32Type *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Type, Float32Type);
};

class Float64Type : public FloatingPointType {
    JBALLOC_(Float64Type)

    friend class BaseExtension;

public:
    virtual size_t size() const { return 64; }
    virtual Literal *literal(LOCATION, IR *ir, const double value) const;
    virtual Literal *zero(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 0.0); }
    virtual Literal *identity(LOCATION, IR *ir) const { return literal(PASSLOC, ir, 1.0); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;
    virtual const double getFloatingPoint(const Literal *lv) const;

protected:
    Float64Type(MEM_LOCATION(a), Extension *ext) : FloatingPointType(MEM_PASSLOC(a), getTypeClassKind(), ext, "Float64", 64) { }
    Float64Type(Allocator *a, const Float64Type *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Type, Float64Type);
};

class AddressType : public BaseType {
    JBALLOC_(AddressType)

    friend class BaseExtension;

public:
    virtual size_t size() const { return 64; } // should be platform specific
    virtual Literal *literal(LOCATION, IR *ir, const void * value) const;
    virtual Literal *zero(LOCATION, IR *ir) const { return literal(PASSLOC, ir, NULL); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;

protected:
    DYNAMIC_ALLOC_ONLY(AddressType, LOCATION, Extension *ext);
    DYNAMIC_ALLOC_ONLY(AddressType, LOCATION, Extension *ext, String name);
    DYNAMIC_ALLOC_ONLY(AddressType, LOCATION, Extension *ext, TypeDictionary *dict, String name);
    DYNAMIC_ALLOC_ONLY(AddressType, LOCATION, Extension *ext, TypeDictionary *dict, TypeKind kind, String name);
    AddressType(Allocator *a, const AddressType *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    SUBCLASS_KINDSERVICE_DECL(Type, AddressType);
};

class PointerType;
class PointerTypeBuilder;
typedef void (PointerTypeHelper)(PointerType *pType, PointerTypeBuilder *builder);

class PointerTypeBuilder : public Allocatable {
    JBALLOC_(PointerTypeBuilder)

public:
    ALL_ALLOC_ALLOWED(PointerTypeBuilder, BaseExtension *ext, Compilation *comp);
    ALL_ALLOC_ALLOWED(PointerTypeBuilder, BaseExtension *ext, IR *ir);

    PointerTypeBuilder *setBaseType(const Type *type) { _baseType = type; return this; }
    PointerTypeBuilder *setHelper(PointerTypeHelper *helper) { _helper = helper; return this; }

    BaseExtension *extension() const { return _ext; }
    IR *ir() const { return _ir; }
    TypeDictionary *dict() const { return _dict; }
    const Type *baseType() const { return _baseType; }
    PointerTypeHelper *helper() const { return _helper; }
    String name() const { return String("PointerTo(") + _baseType->name() + String(")"); }

    const PointerType *create(LOCATION);

protected:
    BaseExtension * _ext;
    IR * _ir;
    TypeDictionary *_dict;
    const Type * _baseType;
    PointerTypeHelper *_helper;
};
    
class PointerType : public AddressType {
    JBALLOC_(PointerType)

    friend class PointerTypeBuilder;

public:
    const Type * baseType() const { return _baseType; }

    virtual Literal *literal(LOCATION, IR *ir, const void * value) const;
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual String to_string(bool useHeader=false) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;
    virtual const Type * replace(TypeReplacer *repl);

protected:
    DYNAMIC_ALLOC_ONLY(PointerType, LOCATION, PointerTypeBuilder *builder);
    PointerType(Allocator *a, const PointerType *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    const Type * _baseType;

    SUBCLASS_KINDSERVICE_DECL(Type, PointerType);
};

struct StructType;

class FieldType : public BaseType {
    JBALLOC_(FieldType)

    friend class StructType;
    #if NEED_UNION
    friend class UnionType; // ?
    #endif

public:
    const StructType *owningStruct() const  { return _structType; }
    String fieldName() const { return _fieldName; }
    const Type *type() const { return _type; }
    size_t offset() const { return _offset; }

    Literal *literal(LOCATION, IR *ir, const LiteralBytes * structValue) const { return NULL; };
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const { return false; }
    virtual String to_string(bool useHeader=false) const;
    virtual void logValue(TextLogger & lgr, const void *p) const { }
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const { }

protected:
    protected:
    DYNAMIC_ALLOC_ONLY(FieldType, LOCATION, BaseExtension *ext, TypeDictionary *dict, const StructType *structType, String fieldName, const Type *type, size_t offset);
    FieldType(Allocator *a, const FieldType *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    String explodedName(TypeReplacer *repl, String & baseName) const;

    const StructType *_structType;
    String _fieldName;
    const Type *_type;
    size_t _offset;

    SUBCLASS_KINDSERVICE_DECL(Type, FieldType);
};

typedef std::map<String, const FieldType *>::const_iterator FieldIterator;

class StructTypeBuilder;
typedef void (StructHelperFunction)(const StructType *sType, StructTypeBuilder *builder);
#if NEED_UNION
class UnionType;
#endif

class StructTypeBuilder : public Allocatable {
    JBALLOC_(StructTypeBuilder);

    friend class StructType;

    // FieldInfo is used to record fields
    struct FieldInfo {
        String _name;
        const Type * _type;
        size_t _offset;
        FieldInfo(String name, const Type *type, size_t offset)
            : _name(name), _type(type), _offset(offset) {
        }
    };

public:
    ALL_ALLOC_ALLOWED(StructTypeBuilder, BaseExtension *ext, Compilation *comp);
    ALL_ALLOC_ALLOWED(StructTypeBuilder, BaseExtension *ext, IR *ir);

    #if NEED_UNION
    StructTypeBuilder *setUnion(bool v=false); { _buildUnion = v; return this; }
    #endif
    StructTypeBuilder *setName(String n) { _name = n; return this; }
    StructTypeBuilder *setSize(size_t size) { _size = size; return this; }
    StructTypeBuilder *setHelper(StructHelperFunction *helper) { _helper = helper; return this; }
    StructTypeBuilder *addField(String name, const Type *fieldType, size_t offset) {
        FieldInfo info(name, fieldType, offset);
        _fields.push_back(info);
        return this;
    }

    BaseExtension *extension() const { return _ext; }
    IR *ir() const { return _ir; }
    TypeDictionary *dict() const { return _dict; }
    String name() const { return _name; }
    size_t size() const { return _size; }
    StructHelperFunction *helper() const { return _helper; }

    const StructType * create(LOCATION);

protected:
    #if NEED_UNION
    const UnionType *createUnion();
    #endif
    virtual void innerCreate(const StructType *sType) {
        // TODO: eliminate helper field and just use this virtual function
        if (_helper != NULL)
            _helper(sType, this);
    };

    void createFields(MEM_LOCATION(a));
    void createFields(LOCATION) { createFields(MEM_PASSLOC(allocator())); }
    bool verifyFields(const StructType *sType);
    void setStructType(StructType *structType) { _structType = structType; }

    BaseExtension * _ext;
    IR * _ir;
    CompileUnit * _unit;
    TypeDictionary * _dict;
    String _name;
    size_t _size;
    #if NEED_UNION
    bool _buildUnion;
    #endif
    List<FieldInfo> _fields;
    StructHelperFunction *_helper;
    StructType *_structType;
};

class StructType : public BaseType {
    JBALLOC_(StructType)

    friend class StructTypeBuilder;
    friend class TypeReplacer;

public:
    virtual size_t size() const { return _structSize; }

    virtual Literal *literal(LOCATION, IR *ir, const LiteralBytes * structValue) const;
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual String to_string(bool useHeader=false) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;

    FieldIterator FieldsBegin() const { return _fieldsByName.cbegin(); }
    FieldIterator FieldsEnd() const   { return _fieldsByName.cend(); }
    const FieldType *LookupField(String fieldName) const
        {
        auto it = _fieldsByName.find(fieldName);
        if (it == _fieldsByName.end())
            return NULL;
        return it->second;
        }

    virtual const Type * replace(TypeReplacer *repl);
    virtual bool canBeLayout() const { return true; }
    virtual void explodeAsLayout(TypeReplacer *repl, size_t baseOffset, TypeMapper *m) const;

protected:
    DYNAMIC_ALLOC_ONLY(StructType, LOCATION, StructTypeBuilder *builder);
    StructType(Allocator *a, const StructType *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    virtual const FieldType * addField(MEM_LOCATION(a), Extension *ext, TypeDictionary *dict, String name, const Type *type, size_t offset);
    virtual const FieldType * addField(LOCATION, Extension *ext, TypeDictionary *dict, String name, const Type *type, size_t offset) {
        return addField(MEM_PASSLOC(allocator()), ext, dict, name, type, offset);
    }
    void transformFields(TypeReplacer *repl, StructTypeBuilder *stb, StructType *origStruct, String baseName, size_t baseOffset) const;
    void mapTransformedFields(TypeReplacer *repl, const StructType *type, String baseName, TypeMapper *mapper) const;

    size_t _structSize;

    std::map<String, const FieldType *> _fieldsByName;
    std::multimap<size_t, const FieldType *> _fieldsByOffset;

    SUBCLASS_KINDSERVICE_DECL(Type, StructType);
};

#if NEED_UNION
// why is this class needed?
class UnionType : public StructType {
    friend class StructTypeBuilder;

public:
    Literal *literal(LOCATION, IR *ir, void * unionValue);

    virtual FieldType * addField(LOCATION, Literal *name, Type *type, size_t unused)
        {
        if (type->size() > _size)
            _size = type->size();
        return this->StructType::addField(PASSLOC, name, type, 0);
        }

    virtual void logValue(TextLogger & lgr, const void *p) const;

protected:
    SUBCLASS_KINDSERVICE_DECL(Type, UnionType);
};
#endif

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // !defined(BASETYPES_INCL)

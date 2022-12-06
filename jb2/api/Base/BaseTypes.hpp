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

#include <list>
#include <map>
#include "JBCore.hpp"
#include "Func/Func.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

class BaseCompilation;
class BaseExtension;

class BaseType : public Type {
    friend class BaseExtension;

    protected:
    BaseType(LOCATION, TypeKind kind, Extension *ext, String name, size_t size)
        : Type(PASSLOC, kind, ext, name, size) {

    }
    BaseType(LOCATION, TypeKind kind, Extension *ext, TypeDictionary *dict, String name, size_t size)
        : Type(PASSLOC, kind, ext, dict, name, size) {

    }

    BaseExtension *baseExt();
    BaseExtension *baseExt() const;
};

class NumericType : public BaseType {
    friend class BaseExtension;

public:
    static const TypeKind getTypeClassKind();

protected:
    NumericType(LOCATION, TypeKind kind, Extension *ext, String name, size_t size)
        : BaseType(PASSLOC, TYPEKIND, ext, name, size) {

    }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class IntegerType : public NumericType {
    friend class BaseExtension;
public:
    virtual bool isInteger() const { return true; }

    static const TypeKind getTypeClassKind();

protected:
    IntegerType(LOCATION, TypeKind kind, Extension *ext, String name, size_t size)
        : NumericType(PASSLOC, TYPEKIND, ext, name, size) {

    }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class Int8Type : public IntegerType {
    friend class BaseExtension;

public:
    virtual size_t size() const { return 8; }
    Literal *literal(LOCATION, Compilation *comp, const int8_t value) const;
    Literal *zero(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 0); }
    Literal *identity(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 1); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;
    virtual void createJB1ConstOp(Location *loc, JB1MethodBuilder *j1mb, Builder *b, Value *result, Literal *lv) const;
    virtual const int64_t getInteger(const Literal *lv) const;

    static const TypeKind getTypeClassKind();

protected:
    Int8Type(LOCATION, Extension *ext) : IntegerType(PASSLOC, TYPEKIND, ext, "Int8", 8) { }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class Int16Type : public IntegerType {
    friend class BaseExtension;

public:
    virtual size_t size() const { return 16; }
    Literal *literal(LOCATION, Compilation *comp, const int16_t value) const;
    Literal *zero(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 0); }
    Literal *identity(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 1); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;
    virtual void createJB1ConstOp(Location *loc, JB1MethodBuilder *j1mb, Builder *b, Value *result, Literal *lv) const;
    virtual const int64_t getInteger(const Literal *lv) const;

    static const TypeKind getTypeClassKind();

protected:
    Int16Type(LOCATION, Extension *ext) : IntegerType(PASSLOC, TYPEKIND, ext, "Int16", 16) { }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class Int32Type : public IntegerType {
    friend class BaseExtension;

public:
    virtual size_t size() const { return 32; }
    Literal *literal(LOCATION, Compilation *comp, const int32_t value) const;
    Literal *zero(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 0); }
    Literal *identity(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 1); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;
    virtual void createJB1ConstOp(Location *loc, JB1MethodBuilder *j1mb, Builder *b, Value *result, Literal *lv) const;
    virtual const int64_t getInteger(const Literal *lv) const;

    static const TypeKind getTypeClassKind();

protected:
    Int32Type(LOCATION, Extension *ext) : IntegerType(PASSLOC, TYPEKIND, ext, "Int32", 32) { }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class Int64Type : public IntegerType {
    friend class BaseExtension;

public:
    virtual size_t size() const { return 64; }
    Literal *literal(LOCATION, Compilation *comp, const int64_t value) const;
    Literal *zero(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 0); }
    Literal *identity(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 1); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;
    virtual void createJB1ConstOp(Location *loc, JB1MethodBuilder *j1mb, Builder *b, Value *result, Literal *lv) const;
    virtual const int64_t getInteger(const Literal *lv) const;

    static const TypeKind getTypeClassKind();

protected:
    Int64Type(LOCATION, Extension *ext) : IntegerType(PASSLOC, TYPEKIND, ext, "Int64", 64) { }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class FloatingPointType : public NumericType {
    friend class BaseExtension;

    static const TypeKind getTypeClassKind();

protected:
    FloatingPointType(LOCATION, TypeKind kind, Extension *ext, String name, size_t size)
        : NumericType(PASSLOC, kind, ext, name, size) {

    }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class Float32Type : public FloatingPointType {
    friend class BaseExtension;

public:
    virtual size_t size() const { return 32; }
    Literal *literal(LOCATION, Compilation *comp, const float value) const;
    Literal *zero(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 0.0); }
    Literal *identity(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 1.0); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;
    virtual void createJB1ConstOp(Location *loc, JB1MethodBuilder *j1mb, Builder *b, Value *result, Literal *lv) const;
    virtual const double getFloatingPoint(const Literal *lv) const;

    static const TypeKind getTypeClassKind();

protected:
    Float32Type(LOCATION, Extension *ext) : FloatingPointType(PASSLOC, TYPEKIND, ext, "Float32", 32) { }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class Float64Type : public FloatingPointType {
    friend class BaseExtension;

public:
    virtual size_t size() const { return 64; }
    Literal *literal(LOCATION, Compilation *comp, const double value) const;
    Literal *zero(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 0.0); }
    Literal *identity(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, 1.0); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;
    virtual void createJB1ConstOp(Location *loc, JB1MethodBuilder *j1mb, Builder *b, Value *result, Literal *lv) const;
    virtual const double getFloatingPoint(const Literal *lv) const;

    static const TypeKind getTypeClassKind();

protected:
    Float64Type(LOCATION, Extension *ext) : FloatingPointType(PASSLOC, TYPEKIND, ext, "Float64", 64) { }

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class AddressType : public BaseType {
    friend class BaseExtension;

public:
    virtual size_t size() const { return 64; } // should be platform specific
    Literal *literal(LOCATION, Compilation *comp, const void * value) const;
    Literal *zero(LOCATION, Compilation *comp) const { return literal(PASSLOC, comp, NULL); }
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;
    virtual void createJB1ConstOp(Location *loc, JB1MethodBuilder *j1mb, Builder *b, Value *result, Literal *lv) const;

    static const TypeKind getTypeClassKind();

protected:
    AddressType(LOCATION, Extension *ext);
    AddressType(LOCATION, Extension *ext, String name);
    AddressType(LOCATION, Extension *ext, TypeDictionary *dict, String name);
    AddressType(LOCATION, Extension *ext, TypeDictionary *dict, TypeKind kind, String name);

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

class PointerType;
class PointerTypeBuilder;
typedef void (PointerTypeHelper)(PointerType *pType, PointerTypeBuilder *builder);

class PointerTypeBuilder {
public:
    PointerTypeBuilder(BaseExtension *ext, Base::BaseCompilation *comp);
    PointerTypeBuilder *setBaseType(const Type *type) { _baseType = type; return this; }
    PointerTypeBuilder *setHelper(PointerTypeHelper *helper) { _helper = helper; return this; }

    BaseExtension *extension() const { return _ext; }
    Base::BaseCompilation *comp() const { return _comp; }
    TypeDictionary *dict() const { return _dict; }
    const Type *baseType() const { return _baseType; }
    PointerTypeHelper *helper() const { return _helper; }
    String name() const { return String("PointerTo(") + _baseType->name() + String(")"); }

    const PointerType *create(LOCATION);

protected:
    BaseExtension * _ext;
    Base::BaseCompilation * _comp;
    TypeDictionary *_dict;
    const Type * _baseType;
    PointerTypeHelper *_helper;
};
    
class PointerType : public AddressType {
    friend class PointerTypeBuilder;

public:
    const Type * baseType() const { return _baseType; }

    Literal *literal(LOCATION, Compilation *comp, const void * value) const;
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual String to_string(bool useHeader=false) const;
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;
    virtual const Type * replace(TypeReplacer *repl);

    static const TypeKind getTypeClassKind();

protected:
    PointerType(LOCATION, PointerTypeBuilder *builder);
    const Type * _baseType;

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

struct StructType;

class FieldType : public BaseType {
    friend class StructType;
    #if NEED_UNION
    friend class UnionType; // ?
    #endif

public:
    const StructType *owningStruct() const  { return _structType; }
    String fieldName() const { return _fieldName; }
    const Type *type() const { return _type; }
    size_t offset() const { return _offset; }

    Literal *literal(LOCATION, Compilation *comp, const LiteralBytes * structValue) const { return NULL; };
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const { return false; }
    virtual String to_string(bool useHeader=false) const;
    virtual void printValue(TextWriter &w, const void *p) const { }
    virtual void printLiteral(TextWriter &w, const Literal *lv) const { }
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;

    static const TypeKind getTypeClassKind();

protected:
    protected:
    FieldType(LOCATION, BaseExtension *ext, TypeDictionary *dict, const StructType *structType, String fieldName, const Type *type, size_t offset);

    String explodedName(TypeReplacer *repl, String & baseName) const;

    const StructType *_structType;
    String _fieldName;
    const Type *_type;
    size_t _offset;

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

typedef std::map<String, const FieldType *>::const_iterator FieldIterator;

class StructTypeBuilder;
typedef void (StructHelperFunction)(const StructType *sType, StructTypeBuilder *builder);
#if NEED_UNION
class UnionType;
#endif

class StructTypeBuilder {
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
    StructTypeBuilder(BaseExtension *ext, Base::BaseCompilation *comp);
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
    Base::BaseCompilation *comp() const { return _comp; }
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

    void createFields(LOCATION);
    bool verifyFields(const StructType *sType);
    void setStructType(StructType *structType) { _structType = structType; }

    BaseExtension * _ext;
    Func::Function * _func;
    Base::BaseCompilation * _comp;
    TypeDictionary * _dict;
    String _name;
    size_t _size;
    #if NEED_UNION
    bool _buildUnion;
    #endif
    std::list<FieldInfo> _fields;
    StructHelperFunction *_helper;
    StructType *_structType;
};

class StructType : public BaseType {
    friend class StructTypeBuilder;
    friend class TypeReplacer;

public:
    virtual size_t size() const { return _structSize; }

    Literal *literal(LOCATION, Compilation *comp, const LiteralBytes * structValue) const;
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual String to_string(bool useHeader=false) const;
    virtual void printValue(TextWriter &w, const void *p) const;
    virtual void printLiteral(TextWriter &w, const Literal *lv) const;
    virtual bool registerJB1Type(JB1MethodBuilder *j1mb) const;

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

    static const TypeKind getTypeClassKind();

protected:
    StructType(LOCATION, StructTypeBuilder *builder);
    virtual const FieldType * addField(LOCATION, Extension *ext, TypeDictionary *dict, String name, const Type *type, size_t offset);
    void registerAllFields(JB1MethodBuilder *j1mb, String structName, String fNamePrefix, size_t baseOffset) const;
    void transformFields(TypeReplacer *repl, StructTypeBuilder *stb, StructType *origStruct, String baseName, size_t baseOffset) const;
    void mapTransformedFields(TypeReplacer *repl, const StructType *type, String baseName, TypeMapper *mapper) const;

    size_t _structSize;

    std::map<String, const FieldType *> _fieldsByName;
    std::multimap<size_t, const FieldType *> _fieldsByOffset;

    static TypeKind TYPEKIND;
    static bool kindRegistered;
};

#if NEED_UNION
// why is this class needed?
class UnionType : public StructType {
    friend class StructTypeBuilder;

    static const TypeKind getTypeClassKind();

public:
    Literal *literal(LOCATION, Compilation *comp, void * unionValue);

    virtual FieldType * addField(LOCATION, Literal *name, Type *type, size_t unused)
        {
        if (type->size() > _size)
            _size = type->size();
        return this->StructType::addField(PASSLOC, name, type, 0);
        }

    virtual void printValue(TextWriter *w, const void *p) const;

protected:
    static TypeKind TYPEKIND;
    static bool kindRegistered;
};
#endif

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // !defined(BASETYPES_INCL)

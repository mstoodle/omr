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
namespace JB2 {
namespace Base {

class BaseExtension;

#define DECL_BASETYPE_CLASS(C,Super,user_decl) \
    DECL_TYPE_CLASS_WITH_STATE_AND_KIND(C,Super,BaseExtension, \
    protected: \
        DYNAMIC_ALLOC_ONLY(C, LOCATION, ExtensibleKind kind, Extension *ext, String name, size_t size=0); \
        DYNAMIC_ALLOC_ONLY(C, LOCATION, ExtensibleKind kind, Extension *ext, IR *ir, String name, size_t size=0); \
        DYNAMIC_ALLOC_ONLY(C, LOCATION, ExtensibleKind kind, Extension *ext, IR *ir, TypeID tid, String name, size_t size=0); \
        user_decl \
    )

#define DECL_CONCRETE_BASETYPE(C,Super,size,user_decl) \
    DECL_BASETYPE_CLASS(C,Super, \
        public: \
            virtual Literal *zero(LOCATION) const; \
            virtual Literal *identity(LOCATION) const; \
            C(MEM_LOCATION(a), Extension *ext); \
            C(MEM_LOCATION(a), Extension *ext, IR *ir); \
            C(MEM_LOCATION(a), Extension *ext, IR *ir, TypeID tid); \
        protected: \
        user_decl \
    )

DECL_BASETYPE_CLASS(BaseType, Type,
    protected:
        BaseExtension *baseExt();
        BaseExtension *baseExt() const;
)

DECL_BASETYPE_CLASS(NumericType, BaseType,
    public:
        virtual bool isInteger() const { return false; }
        virtual bool isFloatingPoint() const { return false; }
)

DECL_BASETYPE_CLASS(IntegerType, NumericType,
    public:
        virtual bool isInteger() const { return true; }
)

DECL_CONCRETE_BASETYPE(Int8Type, IntegerType, 8, 
    public:
        Literal *literal(LOCATION, const int8_t value) const;
        virtual const int64_t getInteger(const Literal *lv) const;
)
DECL_CONCRETE_BASETYPE(Int16Type, IntegerType, 16, 
    public:
        Literal *literal(LOCATION, const int16_t value) const;
        virtual const int64_t getInteger(const Literal *lv) const;
)
DECL_CONCRETE_BASETYPE(Int32Type, IntegerType, 32, 
    public:
        Literal *literal(LOCATION, const int32_t value) const;
        virtual const int64_t getInteger(const Literal *lv) const;
)
DECL_CONCRETE_BASETYPE(Int64Type, IntegerType, 64, 
    public:
        Literal *literal(LOCATION, const int64_t value) const;
        virtual const int64_t getInteger(const Literal *lv) const;
)


DECL_BASETYPE_CLASS(FloatingPointType, NumericType,
    public:
        virtual bool isFloatingPoint() const { return true; }
)

DECL_CONCRETE_BASETYPE(Float32Type, FloatingPointType, 32, 
    public:
        Literal *literal(LOCATION, const float value) const;
        virtual const double getFloatingPoint(const Literal *lv) const;
)

DECL_CONCRETE_BASETYPE(Float64Type, FloatingPointType, 64, 
    public:
        Literal *literal(LOCATION, const double value) const;
        virtual const double getFloatingPoint(const Literal *lv) const;
)


DECL_CONCRETE_BASETYPE(AddressType,IntegerType,size,
    public:
        Literal *literal(LOCATION, const void * value) const;
)


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
    const Type *baseType() const { return _baseType; }
    PointerTypeHelper *helper() const { return _helper; }
    String name() const { return String(_ir->mem(), "PointerTo(") + _baseType->name() + String(_ir->mem(), ")"); }

    const PointerType *create(LOCATION);

protected:
    BaseExtension * _ext;
    IR * _ir;
    const Type * _baseType;
    PointerTypeHelper *_helper;
};
    
DECL_CONCRETE_BASETYPE(PointerType,AddressType,size,
    friend class PointerTypeBuilder;
    public:
        const Type * baseType() const { return _baseType; }
        Literal *literal(LOCATION, const void * value) const;
        virtual String to_string(Allocator *mem, bool useHeader=false) const;
        virtual const Type * replace(TypeReplacer *repl);
    protected:
        DYNAMIC_ALLOC_ONLY(PointerType, LOCATION, PointerTypeBuilder *builder);
        const Type * _baseType;
)

#if 0
class PointerType : public AddressType {
    JBALLOC_(PointerType)

    friend class PointerTypeBuilder;

public:
    const Type * baseType() const { return _baseType; }

    virtual Literal *literal(LOCATION, const void * value) const;
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual String to_string(Allocator *mem, bool useHeader=false) const;
    virtual void logValue(TextLogger & lgr, const void *p) const;
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const;
    virtual const Type * replace(TypeReplacer *repl);

protected:
    DYNAMIC_ALLOC_ONLY(PointerType, LOCATION, PointerTypeBuilder *builder);
    PointerType(Allocator *a, const PointerType *source, IRCloner *cloner);

    virtual const Type *clone(Allocator *a, IRCloner *cloner) const;

    const Type * _baseType;

    SUBCLASS_KINDSERVICE_DECL(Extensible, PointerType);
};
#endif

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

    Literal *literal(LOCATION, const LiteralBytes * structValue) const { return NULL; };
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const { return false; }
    virtual String to_string(Allocator *mem, bool useHeader=false) const;
    virtual void logValue(TextLogger & lgr, const void *p) const { }
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const { }

protected:
    protected:
    DYNAMIC_ALLOC_ONLY(FieldType, LOCATION, BaseExtension *ext, const StructType *structType, String fieldName, const Type *type, size_t offset);
    FieldType(Allocator *a, const FieldType *source, IRCloner *cloner);

    virtual const Type *cloneType(Allocator *a, IRCloner *cloner) const;

    String explodedName(TypeReplacer *repl, String & baseName) const;

    const StructType *_structType;
    String _fieldName;
    const Type *_type;
    size_t _offset;

    SUBCLASS_KINDSERVICE_DECL(Extensible, FieldType);
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

    virtual Literal *literal(LOCATION, const LiteralBytes * structValue) const;
    virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const;
    virtual String to_string(Allocator *mem, bool useHeader=false) const;
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

    virtual const Type *cloneType(Allocator *a, IRCloner *cloner) const;

    virtual const FieldType * addField(MEM_LOCATION(a), Extension *ext, String name, const Type *type, size_t offset);
    virtual const FieldType * addField(LOCATION, Extension *ext, String name, const Type *type, size_t offset) {
        return addField(MEM_PASSLOC(allocator()), ext, name, type, offset);
    }
    void transformFields(TypeReplacer *repl, StructTypeBuilder *stb, StructType *origStruct, String baseName, size_t baseOffset) const;
    void mapTransformedFields(TypeReplacer *repl, const StructType *type, String baseName, TypeMapper *mapper) const;

    size_t _structSize;

    std::map<String, const FieldType *> _fieldsByName;
    std::multimap<size_t, const FieldType *> _fieldsByOffset;

    SUBCLASS_KINDSERVICE_DECL(Extensible, StructType);
};

#if NEED_UNION
// why is this class needed?
class UnionType : public StructType {
    friend class StructTypeBuilder;

public:
    Literal *literal(LOCATION, void * unionValue);

    virtual FieldType * addField(LOCATION, Literal *name, Type *type, size_t unused)
        {
        if (type->size() > _size)
            _size = type->size();
        return this->StructType::addField(PASSLOC, name, type, 0);
        }

    virtual void logValue(TextLogger & lgr, const void *p) const;

protected:
    SUBCLASS_KINDSERVICE_DECL(Extensible, UnionType);
};
#endif

} // namespace Base
} // namespace JB2
} // namespace OMR

#endif // !defined(BASETYPES_INCL)

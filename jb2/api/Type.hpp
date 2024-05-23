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

#ifndef TYPE_INCL
#define TYPE_INCL

#include "common.hpp"
#include "CreateLoc.hpp"
#include "KindService.hpp"
#include "Mapper.hpp"

namespace OMR {
namespace JitBuilder {

class Allocator;
class Builder;
class Compiler;
class Extension;
class IR;
class IRCloner;
class Location;
class TextLogger;
class Type;
class TypeDictionary;
class TypeReplacer;

KINDSERVICE_CATEGORY(Type);

class Type : public Allocatable {
    JBALLOC_(Type)

    friend class Compiler;
    friend class Extension;
    friend class IRCloner;
    friend class TypeDictionary;

public:
    Extension *ext() const                         { return _ext; }
    TypeID id() const                              { return _id; }
    const String & name() const                    { return _name; }
    TypeDictionary *owningDictionary() const       { return _dict; }
    virtual size_t size() const                    { return _size; } // some Types cannot set size at construction

    bool operator!=(const Type & other) const {
        return _dict != other._dict || _id != other._id;
    }
    bool operator==(const Type & other) const {
        return _dict == other._dict && _id == other._id;
    }

    String base_string(Allocator *mem, bool useHeader=false) const;
    virtual String to_string(Allocator *mem, bool useHeader=false) const;
    void logType(TextLogger & lgr, bool useHeader=false) const;
    virtual void logValue(TextLogger & lgr, const void *p) const { }
    virtual void logLiteral(TextLogger & lgr, const Literal *lv) const { }
    virtual bool literalsAreEqual(const LiteralBytes *lv1, const LiteralBytes *lv2) const { return false; }

    virtual const int64_t getInteger(const Literal *lv) const { return 0; }
    virtual const double getFloatingPoint(const Literal *lv) const { return 0.0; }

    virtual bool isManaged() const { return false; }

    // creates a Literal of this Type from the raw LiteralBytes
    virtual Literal * literal(LOCATION, IR *ir, const LiteralBytes *value) const;

    // for Types that can, return a zero or "one" literal (NULL means it doesn't exist for this Type)
    virtual Literal *zero(LOCATION, IR *comp) const { return NULL; }
    virtual Literal *identity(LOCATION, IR *comp) const { return NULL; }

    // returning NULL from the next function means that values of this Type cannot be broken down further
    virtual const Type *layout() const { return _layout; }

    // for Types with non-NULL layout, converts a literal of Type to the literals of the layout type in the LiteralMapper
    virtual LiteralMapper *explode(Literal *value, LiteralMapper *m=NULL) const { return NULL; }

    // if this Type should be mapped to another, use repl and create that Type if needed and return it (NULL if not mapped to another Type)
    //virtual const Type * mapIfNeeded(TypeReplacer *repl);

    // return true if this Type can be used as a layout
    virtual bool canBeLayout() const { return false; }

    // if this Type can be a layout, break its parts into the given TypeMapper using individual offsets starting at baseOffset
    virtual void explodeAsLayout(TypeReplacer *repl, size_t baseOffset, TypeMapper *m) const {
        assert(0); // default must be to assert TODO convert to CompilationException
    }

protected:
    DYNAMIC_ALLOC_ONLY(Type, LOCATION, TypeKind kind, Extension *ext, String name, size_t size=0, TypeDictionary *dict=NULL, const Type *layout=NULL);
    Type(Allocator *a, const Type *source, IRCloner *cloner); // used by clone

    void transformTypeIfNeeded(TypeReplacer *repl, const Type *type) const;

    virtual const Type *clone(Allocator *mem, IRCloner *cloner) const;
    const Type *cloneType(Allocator *mem, IRCloner *cloner) const;

    Extension *_ext;
    CreateLocation _createLoc;
    TypeDictionary * _dict;
    const TypeID _id;
    const String _name;
    const size_t _size;
    const Type * _layout;

    BASECLASS_KINDSERVICE_DECL_CONST_ONLY(Type);
};

// All DECL_* macros are designed to be used in headers

#define DECL_TYPE_CLASS_COMMON(C,Super,Ext,user_decl) \
    class C : public Super { \
        JBALLOC_NO_DESTRUCTOR_(C); \
        friend class Ext; \
    public: \
        virtual bool literalsAreEqual(const LiteralBytes *l1, const LiteralBytes *l2) const; \
        virtual void logValue(TextLogger &lgr, const void *p) const; \
        virtual void logLiteral(TextLogger & lgr, const Literal *lv) const; \
    protected: \
        virtual const Type *clone(Allocator *mem, IRCloner *cloner) const; \
        SUBCLASS_KINDSERVICE_DECL(Type, C); \
        user_decl \
    };

// C(Allocator *a, const C *source, IRCloner *cloner); 

#define DECL_TYPE_CLASS_WITH_STATE(C,Super,Ext,user_decl) \
    DECL_TYPE_CLASS_COMMON(C,Super,Ext, \
        C(Allocator *a, const C *source, IRCloner *cloner); \
        virtual ~C(); \
    protected: \
        DYNAMIC_ALLOC_ONLY(C, LOCATION, Extension *ext, TypeDictionary *dict=NULL); \
    user_decl \
    )

#define DECL_TYPE_CLASS(C,Super,Ext) \
    DECL_TYPE_CLASS_WITH_STATE(C, Super, Ext, )

#define DECL_TYPE_CLASS_WITH_STATE_AND_KIND(C,Super,Ext,user_decl) \
    DECL_TYPE_CLASS_COMMON(C,Super,Ext, \
        C(Allocator *a, const C *source, IRCloner *cloner); \
        virtual ~C(); \
    protected: \
        DYNAMIC_ALLOC_ONLY(C, LOCATION, TypeKind kind, Extension *ext, String name, TypeDictionary *dict=NULL); \
    user_decl \
    )

#define DECL_TYPE_CLASS_WITH_KIND(C,Super,Ext) \
    DECL_TYPE_CLASS_WITH_STATE_AND_KIND(C,Super,Ext,)


// All DEFINE_* macros are designed to be used in cpp file
#define DEFINE_TYPE_CLASS_COMMON(C,Super,name,user_code) \
    INIT_JBALLOC_REUSECAT(C, Type) \
    SUBCLASS_KINDSERVICE_IMPL(C, name, Super, Super); \
    const Type * \
    C::clone(Allocator *a, IRCloner *cloner) const { \
        assert(_kind == KIND(Type)); \
        return new (a) C(a, this, cloner); \
    } \
    user_code

// Define class C extending Super with the given name
#define DEFINE_TYPE_CLASS(C,Super,name,user_code) \
    DEFINE_TYPE_CLASS_COMMON(C,Super,name, \
        C::C(Allocator *a, const C *source, IRCloner *cloner) \
            : Super(a, source, cloner) { } \
        C::~C() { } \
        C::C(MEM_LOCATION(a), Extension *ext, TypeDictionary *dict) \
            : Super(MEM_PASSLOC(a), getTypeClassKind(), ext, name, 0, dict) { } \
        user_code \
    )

#define DEFINE_TYPE_CLASS_WITH_KIND(C,Super,name,user_code) \
    DEFINE_TYPE_CLASS_COMMON(C,Super,name, \
        C::C(Allocator *a, const C *source, IRCloner *cloner) \
            : Super(a, source, cloner) { } \
        C::~C() { } \
        C::C(MEM_LOCATION(a), TYPEKind kind, Extension *ext, TypeDictionary *dict) \
            : Super(MEM_PASSLOC(a), kind, ext, name, 0, dict) { } \
        user_code \
    )

    
DECL_TYPE_CLASS(NoTypeType, Type, CoreExtension)

} // namespace JitBuilder
} // namespace OMR

#endif // defined(TYPE_INCL)


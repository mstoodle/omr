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

#ifndef CODEGENERATORFORBASE_INCL
#define CODEGENERATORFORBASE_INCL

#include "JBCore.hpp"
#include "Base/BaseExtension.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

// Can be used to define VFTs for Base extension Operations
#define DEFINE_CG_BASE_VFT_FIELDS \
    Array<gencodeFunction> _gencodeVFT; \
    Array<genconstFunction> _genconstVFT; \
    Array<regtypeFunction> _regtypeVFT

#define INIT_CG_BASE_VFT_FIELDS(a) \
    , _gencodeVFT(NULL, a) \
    , _genconstVFT(NULL, a) \
    , _regtypeVFT(NULL, a)

// Can be used to define dispatch handlers for Base extension Operations
#define DEFINE_CG_BASE_HANDLERS(C) \
    typedef Builder * (C::*gencodeFunction)(Operation *op); \
    Builder * gencodeConst(Operation *op); \
    Builder * gencodeAdd(Operation *op); \
    Builder * gencodeAnd(Operation *op); \
    Builder * gencodeConvertTo(Operation *op); \
    Builder * gencodeDiv(Operation *op); \
    Builder * gencodeEqualTo(Operation *op); \
    Builder * gencodeMul(Operation *op); \
    Builder * gencodeNotEqualTo(Operation *op); \
    Builder * gencodeSub(Operation *op); \
    Builder * gencodeForLoopUp(Operation *op); \
    Builder * gencodeGoto(Operation *op); \
    Builder * gencodeIfCmpEqual(Operation *op); \
    Builder * gencodeIfCmpEqualZero(Operation *op); \
    Builder * gencodeIfCmpGreaterThan(Operation *op); \
    Builder * gencodeIfCmpGreaterOrEqual(Operation *op); \
    Builder * gencodeIfCmpLessThan(Operation *op); \
    Builder * gencodeIfCmpLessOrEqual(Operation *op); \
    Builder * gencodeIfCmpNotEqual(Operation *op); \
    Builder * gencodeIfCmpNotEqualZero(Operation *op); \
    Builder * gencodeIfCmpUnsignedGreaterThan(Operation *op); \
    Builder * gencodeIfCmpUnsignedGreaterOrEqual(Operation *op); \
    Builder * gencodeIfCmpUnsignedLessThan(Operation *op); \
    Builder * gencodeIfCmpUnsignedLessOrEqual(Operation *op); \
    Builder * gencodeIfThenElse(Operation *op); \
    Builder * gencodeSwitch(Operation *op); \
    Builder * gencodeLoadAt(Operation *op); \
    Builder * gencodeStoreAt(Operation *op); \
    Builder * gencodeLoadField(Operation *op); \
    Builder * gencodeStoreField(Operation *op); \
    Builder * gencodeLoadFieldAt(Operation *op); \
    Builder * gencodeStoreFieldAt(Operation *op); \
    Builder * gencodeCreateLocalArray(Operation *op); \
    Builder * gencodeCreateLocalStruct(Operation *op); \
    Builder * gencodeIndexAt(Operation *op); \
    typedef void (C::*genconstFunction)(Location *loc, Builder *parent, Value *result, Literal *lv); \
    void genconstInt8(Location *loc, Builder *parent, Value *result, Literal *lv); \
    void genconstInt16(Location *loc, Builder *parent, Value *result, Literal *lv); \
    void genconstInt32(Location *loc, Builder *parent, Value *result, Literal *lv); \
    void genconstInt64(Location *loc, Builder *parent, Value *result, Literal *lv); \
    void genconstFloat32(Location *loc, Builder *parent, Value *result, Literal *lv); \
    void genconstFloat64(Location *loc, Builder *parent, Value *result, Literal *lv); \
    void genconstAddress(Location *loc, Builder *parent, Value *result, Literal *lv); \
    void genconstStruct(Location *loc, Builder *parent, Value *result, Literal *lv); \
    typedef void (C::*regtypeFunction)(const Type *t); \
    void regtypeInt8(const Type *Int8); \
    void regtypeInt16(const Type *Int16); \
    void regtypeInt32(const Type *Int32); \
    void regtypeInt64(const Type *Int32); \
    void regtypeFloat32(const Type *Float32); \
    void regtypeFloat64(const Type *Float64); \
    void regtypeAddress(const Type *Address)

// assign these in reverse order so VFT only has to be grown once (technically only last one has to go first)
#define INIT_CG_BASE_HANDLERS(C) \
    _regtypeVFT.assign(bx->tAddress, &C::regtypeAddress); \
    _regtypeVFT.assign(bx->tFloat64, &C::regtypeFloat64); \
    _regtypeVFT.assign(bx->tFloat32, &C::regtypeFloat32); \
    _regtypeVFT.assign(bx->tInt64, &C::regtypeInt64); \
    _regtypeVFT.assign(bx->tInt32, &C::regtypeInt32); \
    _regtypeVFT.assign(bx->tInt16, &C::regtypeInt16); \
    _regtypeVFT.assign(bx->tInt8, &C::regtypeInt8); \
    _gencodeVFT.assign(bx->aIndexAt, &C::gencodeIndexAt); \
    _gencodeVFT.assign(bx->aCreateLocalStruct, &C::gencodeCreateLocalStruct); \
    _gencodeVFT.assign(bx->aCreateLocalArray, &C::gencodeCreateLocalArray); \
    _gencodeVFT.assign(bx->aStoreFieldAt, &C::gencodeStoreFieldAt); \
    _gencodeVFT.assign(bx->aLoadFieldAt, &C::gencodeLoadFieldAt); \
    _gencodeVFT.assign(bx->aStoreField, &C::gencodeStoreField); \
    _gencodeVFT.assign(bx->aStoreAt, &C::gencodeStoreAt); \
    _gencodeVFT.assign(bx->aLoadAt, &C::gencodeLoadAt); \
    _gencodeVFT.assign(bx->aSwitch, &C::gencodeSwitch); \
    _gencodeVFT.assign(bx->aIfThenElse, &C::gencodeIfThenElse); \
    _gencodeVFT.assign(bx->aIfCmpUnsignedLessOrEqual, &C::gencodeIfCmpUnsignedLessOrEqual); \
    _gencodeVFT.assign(bx->aIfCmpUnsignedLessThan, &C::gencodeIfCmpUnsignedLessThan); \
    _gencodeVFT.assign(bx->aIfCmpUnsignedGreaterOrEqual, &C::gencodeIfCmpUnsignedGreaterOrEqual); \
    _gencodeVFT.assign(bx->aIfCmpUnsignedGreaterThan, &C::gencodeIfCmpUnsignedGreaterThan); \
    _gencodeVFT.assign(bx->aIfCmpNotEqualZero, &C::gencodeIfCmpNotEqualZero); \
    _gencodeVFT.assign(bx->aIfCmpNotEqual, &C::gencodeIfCmpNotEqual); \
    _gencodeVFT.assign(bx->aIfCmpLessOrEqual, &C::gencodeIfCmpLessOrEqual); \
    _gencodeVFT.assign(bx->aIfCmpLessThan, &C::gencodeIfCmpLessThan); \
    _gencodeVFT.assign(bx->aIfCmpGreaterOrEqual, &C::gencodeIfCmpGreaterOrEqual); \
    _gencodeVFT.assign(bx->aIfCmpGreaterThan, &C::gencodeIfCmpGreaterThan); \
    _gencodeVFT.assign(bx->aIfCmpEqualZero, &C::gencodeIfCmpEqualZero); \
    _gencodeVFT.assign(bx->aIfCmpEqual, &C::gencodeIfCmpEqual); \
    _gencodeVFT.assign(bx->aGoto, &C::gencodeGoto); \
    _gencodeVFT.assign(bx->aForLoopUp, &C::gencodeForLoopUp); \
    _gencodeVFT.assign(bx->aSub, &C::gencodeSub); \
    _gencodeVFT.assign(bx->aNotEqualTo, &C::gencodeNotEqualTo); \
    _gencodeVFT.assign(bx->aMul, &C::gencodeMul); \
    _gencodeVFT.assign(bx->aEqualTo, &C::gencodeEqualTo); \
    _gencodeVFT.assign(bx->aDiv, &C::gencodeDiv); \
    _gencodeVFT.assign(bx->aConvertTo, &C::gencodeConvertTo); \
    _gencodeVFT.assign(bx->aAdd, &C::gencodeAdd); \
    _gencodeVFT.assign(bx->aAnd, &C::gencodeAnd); \
    _gencodeVFT.assign(bx->aConst, &C::gencodeConst); \
    _genconstVFT.assign(bx->tAddress, &C::genconstAddress); \
    _genconstVFT.assign(bx->tFloat64, &C::genconstFloat64); \
    _genconstVFT.assign(bx->tFloat32, &C::genconstFloat32); \
    _genconstVFT.assign(bx->tInt64, &C::genconstInt64); \
    _genconstVFT.assign(bx->tInt32, &C::genconstInt32); \
    _genconstVFT.assign(bx->tInt16, &C::genconstInt16); \
    _genconstVFT.assign(bx->tInt8, &C::genconstInt8)

#define DEFINE_OP_HANDLERS_DISPATCH(C) \
    Builder * \
    C::gencode(Operation *op) { \
        ActionID a = op->action(); \
        gencodeFunction f = _gencodeVFT[a]; \
        return (this->*f)(op); \
    } \
    void \
    C::gencodeConst(Operation *op) { \
        assert(op->action() == _bx->aConst); \
        const Type *retType = op->result()->type(); \
        if (retType->isKind<Base::PointerType>()) \
            retType = _bx->Address; \
        genconstFunction f = _genconstVFT[retType->id()]; \
        (this->*f)(op->location(), op->parent(), op->result(), op->literal()); \
    } \


#define MISSING_CG_TYPE_REGISTRATION(C,jb2Type) \
    void \
    C::regtype ## jb2Type(const Type *type) { \
        return missingCodeGeneratorTypeRegistration(LOC, type); \
    }

#define MISSING_CG_CONSTFORTYPE_HANDLER(C,jb2Type) \
    void \
    C::genconst ## jb2Type(Location *loc, Builder *parent, Value *result, Literal *lv) { \
        return missingCodeGeneratorConstForTypeHandler(LOC, loc, parent, result, lv); \
    }

class CodeGeneratorForBase : public CodeGeneratorForExtension {
    JBALLOC_(CodeGeneratorForBase)

public:
    DYNAMIC_ALLOC_ONLY(CodeGeneratorForBase, CodeGenerator *cg, Base::BaseExtension *base);

    virtual Builder * gencode(Operation *op);

    virtual bool registerSymbol(Symbol *sym) { return false; }
    virtual bool registerType(const Type *type) { return false; }

protected:
    Base::BaseExtension *bx() const;

    virtual void registerField(const Type *ft, String baseStructName, String fieldName, const Type *fieldType, size_t fieldOffset) { }
    const String & registerFieldString(const Base::StructType * sType, const Base::FieldType *fType, const String & name);
    const String & lookupFieldString(const Base::StructType * sType, const Base::FieldType *fType);
    void registerAllStructFields(const Base::StructType *sType, const Base::StructType * baseStructType, const String & fNamePrefix, size_t baseOffset);

    void missingCodeGeneratorTypeRegistration(LOCATION, const Type *type);
    void missingCodeGeneratorConstForTypeHandler(LOCATION, Location *loc, Builder *parent, Value *result, Literal *lv);

    DEFINE_CG_BASE_HANDLERS(CodeGeneratorForBase);

    DEFINE_CG_BASE_VFT_FIELDS;
    typedef std::map<const Base::FieldType *, String *> FieldMapType;
    std::map<const Base::StructType *, FieldMapType> _structFieldNameMap;

    SUBCLASS_KINDSERVICE_DECL(Extensible,CodeGeneratorForBase);
};

} // namespace Base
} // namespace JB2
} // namespace OMR

#endif // defined(CODEGENERATORFORBASE_INCL)

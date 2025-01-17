/*******************************************************************************
 * Copyright IBM Corp. and others 2022
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

#ifndef BASEEXTENSION_INCL
#define BASEEXTENSION_INCL

#include <cstdarg>
#include <stdint.h>
#include "JBCore.hpp"
#include "Func/Func.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

class Case;
class IntegerType;
class Int8Type;
class Int16Type;
class Int32Type;
class Int64Type;
class Float32Type;
class Float64Type;
class AddressType;
class PointerType;
class PointerTypeBuilder;
class FieldType;
class StructType;
class SwitchBuilder;
class UnionType;

class BaseExtensionChecker;
class ForLoopBuilder;
class IfThenElseBuilder;

class BaseExtension : public Extension {
    JBALLOC_(BaseExtension)

    friend class PointerTypeBuilder;

protected:
    CoreExtension *_cx;

public:
    DYNAMIC_ALLOC_ONLY(BaseExtension, LOCATION, Compiler *compiler, bool extended=false, String extensionName="");

    static const String NAME;

    // 1 == FieldSymbol
    uint32_t numSymbolTypes() const { return 1; }

    virtual const SemanticVersion * semver() const {
        return &version;
    }

    //
    // Types
    //

    const TypeID tInt8;
    const TypeID tInt16;
    const TypeID tInt32;
    const TypeID tInt64;
    const TypeID tFloat32;
    const TypeID tFloat64;
    const TypeID tAddress;
    const TypeID tWord;

    const Int8Type *Int8(IR *ir) const;
    const Int16Type *Int16(IR *ir) const;
    const Int32Type *Int32(IR *ir) const;
    const Int64Type *Int64(IR *ir) const;
    const Float32Type *Float32(IR *ir) const;
    const Float64Type *Float64(IR *ir) const;
    const AddressType *Address(IR *ir) const;
    const IntegerType *Word(IR *ir) const;
    const PointerType *PointerTo(LOCATION, const Type *baseType);


    //
    // Actions
    //

    // Const actions
    const ActionID aConst;

    // Arithmetic actions
    const ActionID aAdd;
    const ActionID aAnd;
    const ActionID aConvertTo;
    const ActionID aDiv;
    const ActionID aEqualTo;
    const ActionID aMul;
    const ActionID aNotEqualTo;
    const ActionID aSub;

    // Control actions
    const ActionID aCall;
    const ActionID aCallVoid;
    const ActionID aForLoopUp;
    const ActionID aGoto;
    const ActionID aIfCmpEqual;
    const ActionID aIfCmpEqualZero;
    const ActionID aIfCmpGreaterThan;
    const ActionID aIfCmpGreaterOrEqual;
    const ActionID aIfCmpLessThan;
    const ActionID aIfCmpLessOrEqual;
    const ActionID aIfCmpNotEqual;
    const ActionID aIfCmpNotEqualZero;
    const ActionID aIfCmpUnsignedGreaterThan;
    const ActionID aIfCmpUnsignedGreaterOrEqual;
    const ActionID aIfCmpUnsignedLessThan;
    const ActionID aIfCmpUnsignedLessOrEqual;
    const ActionID aIfThenElse;
    const ActionID aSwitch;

    // Memory actions
    const ActionID aLoadAt;
    const ActionID aStoreAt;
    const ActionID aLoadField;
    const ActionID aStoreField;
    const ActionID aLoadFieldAt;
    const ActionID aStoreFieldAt;
    const ActionID aCreateLocalArray;
    const ActionID aCreateLocalStruct;
    const ActionID aIndexAt;


    //
    // CompilerReturnCodes
    //

    const CompilerReturnCode CompileFail_BadInputTypes_Add;
    const CompilerReturnCode CompileFail_BadInputTypes_And;
    const CompilerReturnCode CompileFail_BadInputTypes_ConvertTo;
    const CompilerReturnCode CompileFail_BadInputTypes_Div;
    const CompilerReturnCode CompileFail_BadInputTypes_EqualTo;
    const CompilerReturnCode CompileFail_BadInputTypes_Mul;
    const CompilerReturnCode CompileFail_BadInputTypes_NotEqualTo;
    const CompilerReturnCode CompileFail_BadInputTypes_Sub;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpEqual;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpEqualZero;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpGreaterThan;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpGreaterOrEqual;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpLessThan;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpLessOrEqual;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpNotEqual;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpNotEqualZero;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpUnsignedGreaterThan;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpUnsignedGreaterOrEqual;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpUnsignedLessThan;
    const CompilerReturnCode CompileFail_BadInputTypes_IfCmpUnsignedLessOrEqual;
    const CompilerReturnCode CompileFail_BadInputTypes_ForLoopUp;
    const CompilerReturnCode CompileFail_BadInputTypes_IfThenElse;
    const CompilerReturnCode CompileFail_BadInputTypes_Switch;
    const CompilerReturnCode CompileFail_BadInputArray_OffsetAt;
    const CompilerReturnCode CompileFail_MismatchedArgumentTypes_Call;
    const CompilerReturnCode CompileFail_CodeGeneratorMissingOperationHandler;
    const CompilerReturnCode CompileFail_CodeGeneratorMissingTypeRegistration;
    const CompilerReturnCode CompileFail_CodeGeneratorMissingConstForTypeHandler;


    //
    // Operations
    //

    // Constant operations
    Value * Const(LOCATION, Builder *, Literal *literal);

    // Arithmetic operations
    Value * Add(LOCATION, Builder *b, Value *left, Value *right);
    Value * And(LOCATION, Builder *b, Value *left, Value *right);
    Value * ConvertTo(LOCATION, Builder *b, const Type *type, Value *value);
    Value * Div(LOCATION, Builder *b, Value *left, Value *right);
    Value * EqualTo(LOCATION, Builder *b, Value *left, Value *right);
    Value * Mul(LOCATION, Builder *b, Value *left, Value *right);
    Value * NotEqualTo(LOCATION, Builder *b, Value *left, Value *right);
    Value * Sub(LOCATION, Builder *b, Value *left, Value *right);

    // Control operations
    ForLoopBuilder ForLoopUp(LOCATION, Builder *b, Symbol *loopVariable, Value *initial, Value *final, Value *bump);
    void Goto(LOCATION, Builder *b, Builder *target);
    void IfCmpEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpEqualZero(LOCATION, Builder *b, Builder *target, Value *condition);
    void IfCmpLessOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpLessThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpGreaterOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpGreaterThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpNotEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpNotEqualZero(LOCATION, Builder *b, Builder *target, Value *condition);
    void IfCmpUnsignedLessOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpUnsignedLessThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpUnsignedGreaterOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    void IfCmpUnsignedGreaterThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right);
    IfThenElseBuilder IfThenElse(LOCATION, Builder *b, Value *selector);
    void Switch(LOCATION, Builder *b, SwitchBuilder *builder);

    // Memory operations
    Value * LoadAt(LOCATION, Builder *b, Value *ptrValue);
    void StoreAt(LOCATION, Builder *b, Value *ptrValue, Value *value);
    Value * LoadField(LOCATION, Builder *b, const FieldType *fieldType, Value *structValue);
    void StoreField(LOCATION, Builder *b, const FieldType *fieldType, Value *structValue, Value *value);
    Value * LoadFieldAt(LOCATION, Builder *b, const FieldType *fieldType, Value *pStruct);
    void StoreFieldAt(LOCATION, Builder *b, const FieldType *fieldType, Value *pStruct, Value *value);
    Value * CreateLocalArray(LOCATION, Builder *b, Literal *numElements, const PointerType *pElementType);
    Value * CreateLocalStruct(LOCATION, Builder *b, const PointerType *pStructType);
    Value * IndexAt(LOCATION, Builder *b, Value *base, Value *index);

    // Pseudo operations
    Value * ConstInt8(LOCATION, Builder *b, int8_t v);
    Value * ConstInt16(LOCATION, Builder *b, int16_t v);
    Value * ConstInt32(LOCATION, Builder *b, int32_t v);
    Value * ConstInt64(LOCATION, Builder *b, int64_t v);
    Value * ConstFloat32(LOCATION, Builder *b, float v);
    Value * ConstFloat64(LOCATION, Builder *b, double v);
    Value * ConstAddress(LOCATION, Builder *b, void *v);
    Value * ConstPointer(LOCATION, Builder *b, const PointerType *type, void *v);

    Value * Zero(LOCATION, Builder *b, const Type *type);
    Value * One(LOCATION, Builder *b, const Type *type);

    Value * OffsetAt(LOCATION, Builder *b, Value *array, size_t elementIndex);
    Value * LoadArray(LOCATION, Builder *b, Value *array, size_t elementIndex);
    void StoreArray(LOCATION, Builder *b, Value *array, size_t elementIndex, Value *value);
    Value * OffsetAt(LOCATION, Builder *b, Value *array, Value *indexValue);
    Value * LoadArray(LOCATION, Builder *b, Value *array, Value *indexValue);
    void StoreArray(LOCATION, Builder *b, Value *array, Value *indexValue, Value *value);

    void registerChecker(BaseExtensionChecker *checker);

protected:
    void failValidateOffsetAt(LOCATION, Builder *b, Value *array);
    virtual void createAddon(Extensible *e);

    List<BaseExtensionChecker *> _checkers;

    static const MajorID BASEEXT_MAJOR=0;
    static const MinorID BASEEXT_MINOR=1;
    static const PatchID BASEEXT_PATCH=0;
    static const SemanticVersion version;

    SUBCLASS_KINDSERVICE_DECL(Extensible,BaseExtension);
};

class BaseExtensionChecker : public Allocatable {
    JBALLOC(BaseExtensionChecker, NoAllocationCategory)

public:
    BaseExtensionChecker(Allocator *a, BaseExtension *base)
        : Allocatable(a)
        , _base(base) {

    }

    virtual bool validateAdd(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateAnd(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateConvertTo(LOCATION, Builder *b, const Type *type, Value *value);
    virtual bool validateDiv(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateEqualTo(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateMul(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateNotEqualTo(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateSub(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateIfCmp(LOCATION, Builder *b, Builder *target, Value *left, Value *right, CompilerReturnCode failCode, String opCodeName);
    virtual bool validateIfCmpUnsigned(LOCATION, Builder *b, Builder *target, Value *left, Value *right, CompilerReturnCode failCode, String opCodeName);
    virtual bool validateIfCmpZero(LOCATION, Builder *b, Builder *target, Value *value, CompilerReturnCode failCode, String opCodeName);
    virtual bool validateForLoopUp(LOCATION, Builder *b, Symbol *loopVariable, Value *initial, Value *final, Value *bump);

protected:
    virtual void failValidateAdd(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateAnd(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateConvertTo(LOCATION, Builder *b, const Type *type, Value *value);
    virtual void failValidateDiv(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateEqualTo(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateMul(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateNotEqualTo(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateSub(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateIfCmp(LOCATION, Builder *b, Builder *target, Value *left, Value *right, CompilerReturnCode failCode, String opCodeName);
    virtual void failValidateIfCmpZero(LOCATION, Builder *b, Builder *target, Value *value, CompilerReturnCode failCode, String opCodeName);
    virtual void failValidateForLoopUp(LOCATION, Builder *b, Symbol *loopVariable, Value *initial, Value *final, Value *bump);

    BaseExtension *_base;
};

class ForLoopBuilder {
    friend class BaseExtension;
    friend class Op_ForLoopUp;

public:
    ForLoopBuilder()
        : _loopVariable(NULL)
        , _initial(NULL)
        , _final(NULL)
        , _bump(NULL)
        , _loopBody(NULL)
        , _loopBreak(NULL)
        , _loopContinue(NULL) {

    }

    Symbol * loopVariable() const { return _loopVariable; }
    Value *initialValue() const { return _initial; }
    Value *finalValue() const { return _final; }
    Value *bumpValue() const { return _bump; }
    Builder *loopBody() const { return _loopBody; }
    Builder *loopBreak() const { return _loopBreak; }
    Builder *loopContinue()const { return _loopContinue; }

private:
    ForLoopBuilder &setLoopVariable(Symbol *s) { _loopVariable = s; return *this; }
    ForLoopBuilder &setInitialValue(Value *v) { _initial = v; return *this; }
    ForLoopBuilder &setFinalValue(Value *v) { _final = v; return *this; }
    ForLoopBuilder &setBumpValue(Value *v) { _bump = v; return *this; }
    ForLoopBuilder &setLoopBody(Builder *b) { _loopBody = b; return *this; }
    ForLoopBuilder &setLoopBreak(Builder *b) { _loopBreak = b; return *this; }
    ForLoopBuilder &setLoopContinue(Builder *b) { _loopContinue = b; return *this; }

    Symbol * _loopVariable;
    Value * _initial;
    Value * _final;
    Value * _bump;
    Builder * _loopBody;
    Builder * _loopBreak;
    Builder * _loopContinue;
};

class IfThenElseBuilder {
    friend class BaseExtension;
    friend class Op_IfThenElse;

public:
    IfThenElseBuilder()
        : _selector(NULL)
        , _thenPath(NULL)
        , _elsePath(NULL) {

    }

    Value *selector() const { return _selector; }
    Builder *thenPath() const { return _thenPath; }
    Builder *elsePath() const { return _elsePath; }

private:
    IfThenElseBuilder &setSelector(Value *v) { _selector = v; return *this; }
    IfThenElseBuilder &setThenPath(Builder *b) { _thenPath = b; return *this; }
    IfThenElseBuilder &setElsePath(Builder *b) { _elsePath = b; return *this; }

    Value * _selector;
    Builder * _thenPath;
    Builder * _elsePath;
};

} // namespace Base
} // namespace JB2
} // namespace OMR

#endif // defined(BASEEXTENSION_INCL)

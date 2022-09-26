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

#ifndef BASEEXTENSION_INCL
#define BASEEXTENSION_INCL

#include <cstdarg>
#include <map>
#include <stdint.h>
#include "CreateLoc.hpp"
#include "Extension.hpp"
#include "IDs.hpp"
#include "SemanticVersion.hpp"
#include "typedefs.hpp"


namespace OMR {
namespace JitBuilder {

class Compilation;
class Context;
class FieldType;
class FunctionType;
class Literal;
class Location;
class OperationCloner;
class OperationReplacer;
class PointerType;
class StructType;
class UnionType;
class Value;

namespace Base {

class NoTypeType;
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
class UnionType;
class FunctionType;

class BaseExtensionChecker;
class ForLoopBuilder;
class FunctionCompilation;
class FunctionSymbol;
class LocalSymbol;

class BaseExtension : public Extension {
    friend class PointerTypeBuilder;

public:
    BaseExtension(Compiler *compiler, bool extended=false, std::string extensionName="");
    virtual ~BaseExtension();

    static const std::string NAME;
    static const MajorID BASEEXT_MAJOR=0;
    static const MinorID BASEEXT_MINOR=1;
    static const PatchID BASEEXT_PATCH=0;

    // 4 == LocalSymbol, ParameterSymbol, FunctionSymbol, FieldSymbol
    uint32_t numSymbolTypes() const { return 4; }

    virtual const SemanticVersion * semver() const {
        return &version;
    }

    //
    // Types
    //

    const NoTypeType *NoType;
    const Int8Type *Int8;
    const Int16Type *Int16;
    const Int32Type *Int32;
    const Int64Type *Int64;
    const Float32Type *Float32;
    const Float64Type *Float64;
    const AddressType *Address;
    const Type *Word;

    const PointerType *PointerTo(LOCATION, FunctionCompilation *comp, const Type *baseType);

    #if 0
    const FunctionType * DefineFunctionType(LOCATION, FunctionTypeBuilder *builder);
    #endif
    // deprecated
    const FunctionType * DefineFunctionType(LOCATION, FunctionCompilation *comp, const Type *returnType, int32_t numParms, const Type **parmTypes);

    //
    // Actions
    //

    // Const actions
    const ActionID aConst;

    // Arithmetic actions
    const ActionID aAdd;
    const ActionID aConvertTo;
    const ActionID aMul;
    const ActionID aSub;

    // Control actions
    const ActionID aCall;
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
    const ActionID aReturn;

    // Memory actions
    const ActionID aLoad;
    const ActionID aStore;
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
    const CompilerReturnCode CompileFail_BadInputTypes_ConvertTo;
    const CompilerReturnCode CompileFail_BadInputTypes_Mul;
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
    const CompilerReturnCode CompileFail_BadInputArray_OffsetAt;
    const CompilerReturnCode CompileFail_MismatchedArgumentTypes_Call;


    //
    // Operations
    //

    // Constant operations
    Value * Const(LOCATION, Builder *, Literal *literal);

    // Arithmetic operations
    Value * Add(LOCATION, Builder *b, Value *left, Value *right);
    Value * ConvertTo(LOCATION, Builder *b, const Type *type, Value *value);
    Value * Mul(LOCATION, Builder *b, Value *left, Value *right);
    Value * Sub(LOCATION, Builder *b, Value *left, Value *right);

    // Control operations
    Value *Call(LOCATION, Builder *b, FunctionSymbol *funcSym, ...);
    Value *CallWithArgArray(LOCATION, Builder *b, FunctionSymbol *funcSym, int32_t numArgs, Value **args);
    ForLoopBuilder *ForLoopUp(LOCATION, Builder *b, LocalSymbol *loopVariable, Value *initial, Value *final, Value *bump);
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
    void Return(LOCATION, Builder *b);
    void Return(LOCATION, Builder *b, Value *v);

    // Memory operations
    Value * Load(LOCATION, Builder *b, Symbol *sym);
    void Store(LOCATION, Builder *b, Symbol *sym, Value *value);
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
    Location * SourceLocation(LOCATION, Builder *b, std::string func);
    Location * SourceLocation(LOCATION, Builder *b, std::string func, std::string lineNumber);
    Location * SourceLocation(LOCATION, Builder *b, std::string func, std::string lineNumber, int32_t bcIndex);

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

    void Increment(LOCATION, Builder *b, Symbol *sym, Value *bump);
    void Increment(LOCATION, Builder *b, LocalSymbol *sym);

    Value * OffsetAt(LOCATION, Builder *b, Value *array, size_t elementIndex);
    Value * LoadArray(LOCATION, Builder *b, Value *array, size_t elementIndex);
    void StoreArray(LOCATION, Builder *b, Value *array, size_t elementIndex, Value *value);
    Value * OffsetAt(LOCATION, Builder *b, Value *array, Value *indexValue);
    Value * LoadArray(LOCATION, Builder *b, Value *array, Value *indexValue);
    void StoreArray(LOCATION, Builder *b, Value *array, Value *indexValue, Value *value);

    // JB1 compilation support
    CompilerReturnCode jb1cgCompile(Compilation *comp);

protected:
    void failValidateOffsetAt(LOCATION, Builder *b, Value *array);

    StrategyID _jb1cgStrategyID;
    std::vector<BaseExtensionChecker *> _checkers;

    static const SemanticVersion version;
};

class BaseExtensionChecker {
public:
    BaseExtensionChecker(BaseExtension *base)
        : _base(base) {

    }

    virtual bool validateAdd(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args);
    //virtual bool validateCallWithArgArray(LOCATION, Builder *b, FunctionSymbol *target, int32_t numArgs, Value **args);
    virtual bool validateConvertTo(LOCATION, Builder *b, const Type *type, Value *value);
    virtual bool validateMul(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateSub(LOCATION, Builder *b, Value *left, Value *right);
    virtual bool validateIfCmp(LOCATION, Builder *b, Builder *target, Value *left, Value *right, CompilerReturnCode failCode, std::string opCodeName);
    virtual bool validateIfCmpZero(LOCATION, Builder *b, Builder *target, Value *value, CompilerReturnCode failCode, std::string opCodeName);
    virtual bool validateForLoopUp(LOCATION, Builder *b, LocalSymbol *loopVariable, Value *initial, Value *final, Value *bump);

protected:
    virtual void failValidateAdd(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args);
    //virtual void failValidateCallWithArgArray(LOCATION, Builder *b, FunctionSymbol *target, int32_t numArgs, Value **args);
    virtual void failValidateConvertTo(LOCATION, Builder *b, const Type *type, Value *value);
    virtual void failValidateMul(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateSub(LOCATION, Builder *b, Value *left, Value *right);
    virtual void failValidateIfCmp(LOCATION, Builder *b, Builder *target, Value *left, Value *right, CompilerReturnCode failCode, std::string opCodeName);
    virtual void failValidateIfCmpZero(LOCATION, Builder *b, Builder *target, Value *value, CompilerReturnCode failCode, std::string opCodeName);
    virtual void failValidateForLoopUp(LOCATION, Builder *b, LocalSymbol *loopVariable, Value *initial, Value *final, Value *bump);

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

    LocalSymbol * loopVariable() const { return _loopVariable; }
    Value *initialValue() const { return _initial; }
    Value *finalValue() const { return _final; }
    Value *bumpValue() const { return _bump; }
    Builder *loopBody() const { return _loopBody; }
    Builder *loopBreak() const { return _loopBreak; }
    Builder *loopContinue()const { return _loopContinue; }

private:
    ForLoopBuilder *setLoopVariable(LocalSymbol *s) { _loopVariable = s; return this; }
    ForLoopBuilder *setInitialValue(Value *v) { _initial = v; return this; }
    ForLoopBuilder *setFinalValue(Value *v) { _final = v; return this; }
    ForLoopBuilder *setBumpValue(Value *v) { _bump = v; return this; }
    ForLoopBuilder *setLoopBody(Builder *b) { _loopBody = b; return this; }
    ForLoopBuilder *setLoopBreak(Builder *b) { _loopBreak = b; return this; }
    ForLoopBuilder *setLoopContinue(Builder *b) { _loopContinue = b; return this; }

    LocalSymbol * _loopVariable;
    Value * _initial;
    Value * _final;
    Value * _bump;
    Builder * _loopBody;
    Builder * _loopBreak;
    Builder * _loopContinue;
};

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // defined(BASEEXTENSION_INCL)

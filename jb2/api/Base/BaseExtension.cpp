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

#include <cstdarg>

#include "ArithmeticOperations.hpp"
#include "BaseExtension.hpp"
#include "BaseSymbols.hpp"
#include "BaseTypes.hpp"
#include "Builder.hpp"
#include "ConstOperations.hpp"
#include "ControlOperations.hpp"
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Context.hpp"
#include "FunctionCompilation.hpp"
#include "JB1CodeGenerator.hpp"
#include "Literal.hpp"
#include "Location.hpp"
#include "MemoryOperations.hpp"
#include "Strategy.hpp"
#include "TextWriter.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

const SemanticVersion BaseExtension::version(BASEEXT_MAJOR,BASEEXT_MINOR,BASEEXT_PATCH);
const std::string BaseExtension::NAME("jb2base");

extern "C" {
    Extension *create(Compiler *compiler) {
        return new BaseExtension(compiler);
    }
}

BaseExtension::BaseExtension(Compiler *compiler, bool extended, std::string extensionName)
    : Extension(compiler, (extended ? extensionName : NAME))
    , NoType(new NoTypeType(LOC, this))
    , Int8(new Int8Type(LOC, this))
    , Int16(new Int16Type(LOC, this))
    , Int32(new Int32Type(LOC, this))
    , Int64(new Int64Type(LOC, this))
    , Float32(new Float32Type(LOC, this))
    , Float64(new Float64Type(LOC, this))
    , Address(new AddressType(LOC, this))
    , Word(compiler->platformWordSize() == 64 ? this->Int64->refine<Type>() : this->Int32->refine<Type>())
    , aConst(registerAction(std::string("Const")))
    , aAdd(registerAction(std::string("Add")))
    , aConvertTo(registerAction(std::string("ConvertTo")))
    , aMul(registerAction(std::string("Mul")))
    , aSub(registerAction(std::string("Sub")))
    , aLoad(registerAction(std::string("Load")))
    , aStore(registerAction(std::string("Store")))
    , aLoadAt(registerAction(std::string("LoadAt")))
    , aStoreAt(registerAction(std::string("StoreAt")))
    , aLoadField(registerAction(std::string("LoadField")))
    , aStoreField(registerAction(std::string("StoreField")))
    , aLoadFieldAt(registerAction(std::string("LoadFieldAt")))
    , aStoreFieldAt(registerAction(std::string("StoreFieldAt")))
    , aCreateLocalArray(registerAction(std::string("CreateLocalArray")))
    , aCreateLocalStruct(registerAction(std::string("CreateLocalStruct")))
    , aIndexAt(registerAction(std::string("IndexAt")))
    , aCall(registerAction(std::string("Call")))
    , aForLoopUp(registerAction(std::string("ForLoopUp")))
    , aGoto(registerAction(std::string("Goto")))
    , aIfCmpEqual(registerAction(std::string("IfCmpEqual")))
    , aIfCmpEqualZero(registerAction(std::string("IfCmpEqualZero")))
    , aIfCmpGreaterThan(registerAction(std::string("IfCmpGreaterThan")))
    , aIfCmpGreaterOrEqual(registerAction(std::string("IfCmpGreaterOrEqual")))
    , aIfCmpLessThan(registerAction(std::string("IfCmpLessThan")))
    , aIfCmpLessOrEqual(registerAction(std::string("IfCmpLessOrEqual")))
    , aIfCmpNotEqual(registerAction(std::string("IfCmpNotEqual")))
    , aIfCmpNotEqualZero(registerAction(std::string("IfCmpNotEqualZero")))
    , aIfCmpUnsignedGreaterThan(registerAction(std::string("IfCmpUnsignedGreaterThan")))
    , aIfCmpUnsignedGreaterOrEqual(registerAction(std::string("IfCmpUnsignedGreaterOrEqual")))
    , aIfCmpUnsignedLessThan(registerAction(std::string("IfCmpUnsignedLessThan")))
    , aIfCmpUnsignedLessOrEqual(registerAction(std::string("IfCmpUnsignedLessOrEqual")))
    , aReturn(registerAction(std::string("Return")))
    , CompileFail_BadInputTypes_Add(registerReturnCode("CompileFail_BadInputTypes_Add"))
    , CompileFail_BadInputTypes_ConvertTo(registerReturnCode("CompileFail_BadInputTypes_ConvertTo"))
    , CompileFail_BadInputTypes_Mul(registerReturnCode("CompileFail_BadInputTypes_Mul"))
    , CompileFail_BadInputTypes_Sub(registerReturnCode("CompileFail_BadInputTypes_Sub"))
    , CompileFail_BadInputTypes_IfCmpEqual(registerReturnCode("CompileFail_BadInputTypes_IfCmpEqual"))
    , CompileFail_BadInputTypes_IfCmpEqualZero(registerReturnCode("CompileFail_BadInputTypes_IfCmpEqualZero"))
    , CompileFail_BadInputTypes_IfCmpGreaterThan(registerReturnCode("CompileFail_BadInputTypes_IfCmpGreaterThan"))
    , CompileFail_BadInputTypes_IfCmpGreaterOrEqual(registerReturnCode("CompileFail_BadInputTypes_IfCmpGreaterOrEqual"))
    , CompileFail_BadInputTypes_IfCmpLessThan(registerReturnCode("CompileFail_BadInputTypes_IfCmpLessThan"))
    , CompileFail_BadInputTypes_IfCmpLessOrEqual(registerReturnCode("CompileFail_BadInputTypes_IfCmpLessOrEqual"))
    , CompileFail_BadInputTypes_IfCmpNotEqual(registerReturnCode("CompileFail_BadInputTypes_IfCmpNotEqual"))
    , CompileFail_BadInputTypes_IfCmpNotEqualZero(registerReturnCode("CompileFail_BadInputTypes_IfCmpNotEqualZero"))
    , CompileFail_BadInputTypes_IfCmpUnsignedGreaterThan(registerReturnCode("CompileFail_BadInputTypes_IfCmpUnsignedGreaterThan"))
    , CompileFail_BadInputTypes_IfCmpUnsignedGreaterOrEqual(registerReturnCode("CompileFail_BadInputTypes_IfCmpUnsignedGreaterOrEqual"))
    , CompileFail_BadInputTypes_IfCmpUnsignedLessThan(registerReturnCode("CompileFail_BadInputTypes_IfCmpUnsignedLessThan"))
    , CompileFail_BadInputTypes_IfCmpUnsignedLessOrEqual(registerReturnCode("CompileFail_BadInputTypes_IfCmpUnsignedLessOrEqual"))
    , CompileFail_BadInputTypes_ForLoopUp(registerReturnCode("CompileFail_BadInputTypes_ForLoopUp"))
    , CompileFail_BadInputArray_OffsetAt(registerReturnCode("CompileFail_BadInputArray_OffsetAt"))
    , CompileFail_MismatchedArgumentTypes_Call(registerReturnCode("CompileFail_MismatchedArgumentTypes_Call")) {

    if (!extended) {
        Strategy *jb1cgStrategy = new Strategy(compiler, "jb1cg");
        Pass *jb1cg = new JB1CodeGenerator(compiler);
        jb1cgStrategy->addPass(jb1cg);
        _jb1cgStrategyID = jb1cgStrategy->id();
        _checkers.push_back(new BaseExtensionChecker(this));
    }
}

BaseExtension::~BaseExtension() {
    delete Address;
    delete Float64;
    delete Float32;
    delete Int64;
    delete Int32;
    delete Int16;
    delete Int8;
    delete NoType;
    // what about other types!?
}

//
// Const Operations
//

Value *
BaseExtension::Const(LOCATION, Builder *b, Literal * lv) {
    Value * result = createValue(b, lv->type());
    addOperation(b, new Op_Const(PASSLOC, this, b, this->aConst, result, lv));
    return result;
}


//
// Arithmetic operations
//
bool
BaseExtensionChecker::validateAdd(LOCATION, Builder *b, Value *left, Value *right) {
    const Type *lType = left->type();
    const Type *rType = right->type();

    const Type *Address = _base->Address;
    const Type *Word = _base->Word;

    if (lType == Address) {
        if (rType != Word)
            failValidateAdd(PASSLOC, b, left, right);
        return true;
    }

    if (rType == Address) {
        if (lType != Word)
            failValidateAdd(PASSLOC, b, left, right);
        return true;
    }

    if (lType == _base->Int8
     || lType == _base->Int16
     || lType == _base->Int32
     || lType == _base->Int64
     || lType == _base->Float32
     || lType == _base->Float64) {
        if (rType != lType)
            failValidateAdd(PASSLOC, b, left, right);
        return true;
    }

    // we defined this operation, so if we can't validate it we have to fail it
    failValidateAdd(PASSLOC, b, left, right);
    return true;
}

void
BaseExtensionChecker::failValidateAdd(LOCATION, Builder *b, Value *left, Value *right) {
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_BadInputTypes_Add);
    const Type *lType = left->type();
    const Type *rType = right->type();
    e.setMessageLine(std::string("Add: invalid input types"))
     .appendMessageLine(std::string("    left ").append(lType->to_string()))
     .appendMessageLine(std::string("   right ").append(rType->to_string()))
     .appendMessageLine(std::string("Left and right types are expected to be the same for integer types (Int8,Int16,Int32,Int64,Float32,Float64)"))
     .appendMessageLine(std::string("If left/right type is Address then the right/left (respectively) type must be Word"));
    throw e;
}

Value *
BaseExtension::Add(LOCATION, Builder *b, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateAdd(PASSLOC, b, left, right))
            break;
    }

    if (right->type() == Address) { // reverse left and right
        Value *save = left;
        left = right;
        right = save;
    }

    Value *result = createValue(b, left->type());
    addOperation(b, new Op_Add(PASSLOC, this, b, aAdd, result, left, right));
    return result;
}

bool
BaseExtensionChecker::validateConvertTo(LOCATION, Builder *b, const Type *type, Value *value) {
    // TODO: enhance type checking
    const Type *vType = value->type();
    if (type == _base->Int8
     || type == _base->Int16
     || type == _base->Int32
     || type == _base->Int64
     || type == _base->Float32
     || type == _base->Float64
     || type == _base->Address) {
        if (vType == _base->Int8
         || vType == _base->Int16
         || vType == _base->Int32
         || vType == _base->Int64
         || vType == _base->Float32
         || vType == _base->Float64
         || vType == _base->Address) {

            return true;
        }
    }

    // we defined this operation, so if we can't validate it we have to fail it
    failValidateConvertTo(PASSLOC, b, type, value);
    return true;
}

void
BaseExtensionChecker::failValidateConvertTo(LOCATION, Builder *b, const Type *type, Value *value) {
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_BadInputTypes_ConvertTo);
    const Type *vType = value->type();
    e.setMessageLine(std::string("ConvertTo: invalid input types"))
     .appendMessageLine(std::string("    type ").append(type->to_string()))
     .appendMessageLine(std::string("   value ").append(vType->to_string()))
     .appendMessageLine(std::string("Source value and destination types must be a primitive type (Int8,Int16,Int32,Int64,Float32,Float64,Address)"));
    throw e;
}

Value *
BaseExtension::ConvertTo(LOCATION, Builder *b, const Type *type, Value *value) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateConvertTo(PASSLOC, b, type, value))
            break;
    }

    Value *result = createValue(b, type);
    addOperation(b, new Op_ConvertTo(PASSLOC, this, b, aConvertTo, result, type, value));
    return result;
}


bool
BaseExtensionChecker::validateMul(LOCATION, Builder *b, Value *left, Value *right) {
    const Type *lType = left->type();
    const Type *rType = right->type();

    if (lType == _base->Address || rType == _base->Address)
        failValidateMul(PASSLOC, b, left, right);

    if (lType == _base->Int8
     || lType == _base->Int16
     || lType == _base->Int32
     || lType == _base->Int64
     || lType == _base->Float32
     || lType == _base->Float64) {
        if (rType != lType)
            failValidateMul(PASSLOC, b, left, right);
        return true;
    }

    // we defined this operation, so if we can't validate it we have to fail it
    failValidateMul(PASSLOC, b, left, right);
    return true;
}

void
BaseExtensionChecker::failValidateMul(LOCATION, Builder *b, Value *left, Value *right) {
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_BadInputTypes_Mul);
    const Type *lType = left->type();
    const Type *rType = right->type();
    e.setMessageLine(std::string("Mul: invalid input types"))
     .appendMessageLine(std::string("    left ").append(lType->to_string()))
     .appendMessageLine(std::string("   right ").append(rType->to_string()))
     .appendMessageLine(std::string("Left and right types are expected to be the same for integer types (Int8,Int16,Int32,Int64,Float32,Float64)"))
     .appendMessageLine(std::string("Address types cannot be used"));
    throw e;
}

Value *
BaseExtension::Mul(LOCATION, Builder *b, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateMul(PASSLOC, b, left, right))
            break;
    }

    Value *result = createValue(b, left->type());
    addOperation(b, new Op_Mul(PASSLOC, this, b, aMul, result, left, right));
    return result;
}

bool
BaseExtensionChecker::validateSub(LOCATION, Builder *b, Value *left, Value *right) {
    const Type *lType = left->type();
    const Type *rType = right->type();

    const Type *Address = _base->Address;
    if (lType == Address) {
        if (rType != Address && rType != _base->Word)
            failValidateSub(PASSLOC, b, left, right);
        return true;
    }

    if (rType == Address) // lType cannot be Address
        failValidateSub(PASSLOC, b, left, right);

    if (lType == _base->Int8
     || lType == _base->Int16
     || lType == _base->Int32
     || lType == _base->Int64
     || lType == _base->Float32
     || lType == _base->Float64) {
        if (rType != lType)
            failValidateSub(PASSLOC, b, left, right);
        return true;
    }

    // we defined this operation, so if we can't validate it we have to fail it
    failValidateSub(PASSLOC, b, left, right);
    return true;
}

void
BaseExtensionChecker::failValidateSub(LOCATION, Builder *b, Value *left, Value *right) {
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_BadInputTypes_Sub);
    const Type *lType = left->type();
    const Type *rType = right->type();
    e.setMessageLine(std::string("Sub: invalid input types"))
     .appendMessageLine(std::string("    left ").append(lType->to_string()))
     .appendMessageLine(std::string("   right ").append(rType->to_string()))
     .appendMessageLine(std::string("Left and right types are expected to be the same for integer types (Int8,Int16,Int32,Int64,Float32,Float64)"))
     .appendMessageLine(std::string("If left type is Address then the right type must be either Address or Word"));
    throw e;
}

Value *
BaseExtension::Sub(LOCATION, Builder *b, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateSub(PASSLOC, b, left, right))
            break;
    }

    Value *result = createValue(b, left->type());
    addOperation(b, new Op_Sub(PASSLOC, this, b, aSub, result, left, right));
    return result;
}

//
// Control operations
//

bool
BaseExtensionChecker::validateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args) {
    const FunctionType *tgtType = target->functionType();
    for (auto a=0;a < tgtType->numParms();a++) {
        Value *arg = va_arg(args, Value *);
        if (arg->type() != tgtType->parmTypes()[a]) // should be more like "can be stored to"
            failValidateCall(PASSLOC, b, target, args);
    }
    va_end(args);
    return true;
}

void
BaseExtensionChecker::failValidateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args) {
    const FunctionType *tgtType = target->functionType();
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_MismatchedArgumentTypes_Call);
    e.setMessageLine(std::string("Call: mismatched argument types"));
    for (auto a=0;a < tgtType->numParms();a++) {
        Value *arg = va_arg(args, Value *);
        if (arg->type() != tgtType->parmTypes()[a])
            e.appendMessageLine(std::string("  X  "));
        else
            e.appendMessageLine(std::string("     "));
        e.appendMessage(std::string(" p").append(std::to_string(a)).append(std::string(" ")).append(tgtType->parmTypes()[a]->to_string())
         .append(std::string(" : a")).append(std::to_string(a))
         .append(std::string(" v")).append(std::to_string(arg->id()))
         .append(std::string(" ")).append(arg->type()->to_string()));
    }
    e.appendMessageLine(std::string("Argument types must match corresponding parameter types (currently exact, should be assignable to)"));
    throw e;
}

Value *
BaseExtension::Call(LOCATION, Builder *b, FunctionSymbol *target, ...) {
    const FunctionType *tgtType = target->functionType();
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        std::va_list args;
        va_start(args, target);
        if (checker->validateCall(PASSLOC, b, target, args))
            break;
        va_end(args);
    }

    std::va_list args;
    va_start(args, target);

    Value *result = NULL;
    if (target->functionType()->returnType() != NULL) {
        result = createValue(b, target->functionType()->returnType());
        addOperation(b, new Op_Call(PASSLOC, this, b, aCall, result, target, args));
    } else {
        addOperation(b, new Op_Call(PASSLOC, this, b, aCall, target, args));
    }
    return result;
}


bool
BaseExtensionChecker::validateForLoopUp(LOCATION, Builder *b, LocalSymbol *loopVariable, Value *initial, Value *final, Value *bump) {
    const Type *counterType = loopVariable->type();
    if (counterType != _base->Int8
     && counterType != _base->Int16
     && counterType != _base->Int32
     && counterType != _base->Int64)
        failValidateForLoopUp(PASSLOC, b, loopVariable, initial, final, bump);

    if (initial->type() != loopVariable->type())
        failValidateForLoopUp(PASSLOC, b, loopVariable, initial, final, bump);

    if (final->type() != loopVariable->type())
        failValidateForLoopUp(PASSLOC, b, loopVariable, initial, final, bump);

    if (bump->type() != loopVariable->type())
        failValidateForLoopUp(PASSLOC, b, loopVariable, initial, final, bump);

    return true;
}

void
BaseExtensionChecker::failValidateForLoopUp(LOCATION, Builder *b, LocalSymbol *loopVariable, Value *initial, Value *final, Value *bump) {
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_BadInputTypes_ForLoopUp);
    e.setMessageLine(std::string("ForLoopUp: invalid input types"))
     .appendMessageLine(std::string("  loop var s").append(std::to_string(loopVariable->id())).append(" ").append(loopVariable->name()).append(" ").append(loopVariable->type()->to_string()))
     .appendMessageLine(std::string("   initial v").append(std::to_string(initial->id())).append(" ").append(initial->type()->to_string()))
     .appendMessageLine(std::string("     final v").append(std::to_string(final->id())).append(" ").append(final->type()->to_string()))
     .appendMessageLine(std::string("      bump v").append(std::to_string(final->id())).append(" ").append(bump->type()->to_string()))
     .appendMessageLine(std::string("Loop variable must be one of Int8, Int16, Int32, or Int64, and the types of initial, final, and bump must be same as the loop variable's type"));
    throw e;
}

ForLoopBuilder *
BaseExtension::ForLoopUp(LOCATION, Builder *b, LocalSymbol *loopVariable,
                         Value *initial, Value *final, Value *bump) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateForLoopUp(PASSLOC, b, loopVariable, initial, final, bump))
            break;
    }

    ForLoopBuilder *loopBuilder = new ForLoopBuilder();
    loopBuilder->setLoopVariable(loopVariable)
               ->setInitialValue(initial)
               ->setFinalValue(final)
               ->setBumpValue(bump);
    addOperation(b, new Op_ForLoopUp(PASSLOC, this, b, this->aForLoopUp, loopBuilder));
    return loopBuilder;
}


void
BaseExtension::Goto(LOCATION, Builder *b, Builder *target) {
    addOperation(b, new Op_Goto(PASSLOC, this, b, this->aGoto, target));
}


bool
BaseExtensionChecker::validateIfCmp(LOCATION, Builder *b, Builder *target, Value *left, Value *right, CompilerReturnCode failCode, std::string opCodeName) {
    const Type *lType = left->type();
    const Type *rType = right->type();

    if (lType == _base->Int8
     || lType == _base->Int16
     || lType == _base->Int32
     || lType == _base->Int64
     || lType == _base->Float32
     || lType == _base->Float64
     || lType == _base->Address) {
        if (rType != lType)
            failValidateIfCmp(PASSLOC, b, target, left, right, failCode, opCodeName);
        return true;
    }

    // operation is declared by this extension, so if we can't validate it we have to fail it
    failValidateIfCmp(PASSLOC, b, target, left, right, failCode, opCodeName);
    return true;
}

void
BaseExtensionChecker::failValidateIfCmp(LOCATION, Builder *b, Builder *target, Value *left, Value *right, CompilerReturnCode failCode, std::string opCodeName) {
    CompilationException e(PASSLOC, _base->compiler(), failCode);
    const Type *lType = left->type();
    const Type *rType = right->type();
    e.setMessageLine(opCodeName.append(std::string(": invalid input types")))
     .appendMessageLine(std::string("    left ").append(lType->to_string()))
     .appendMessageLine(std::string("   right ").append(rType->to_string()))
     .appendMessageLine(std::string("  target ").append(target->to_string()))
     .appendMessageLine(std::string("Left and right types are expected to be the same type (Int8,Int16,Int32,Int64,Float32,Float64,Address)"));
    throw e;
}

bool
BaseExtensionChecker::validateIfCmpZero(LOCATION, Builder *b, Builder *target, Value *value, CompilerReturnCode failCode, std::string opCodeName) {
    const Type *type = value->type();

    if (type == _base->Int8
     || type == _base->Int16
     || type == _base->Int32
     || type == _base->Int64
     || type == _base->Float32
     || type == _base->Float64
     || type == _base->Address) {
        return true;
    }

    // operation is declared by this extension, so if we can't validate it we have to fail it
    failValidateIfCmpZero(PASSLOC, b, target, value, failCode, opCodeName);
    return true;
}

void
BaseExtensionChecker::failValidateIfCmpZero(LOCATION, Builder *b, Builder *target, Value *value, CompilerReturnCode failCode, std::string opCodeName) {
    CompilationException e(PASSLOC, _base->compiler(), failCode);
    const Type *type = value->type();
    e.setMessageLine(opCodeName.append(std::string(": invalid input types")))
     .appendMessageLine(std::string("   value ").append(type->to_string()))
     .appendMessageLine(std::string("  target ").append(target->to_string()))
     .appendMessageLine(std::string("Value type is expected to be a primitive type (Int8,Int16,Int32,Int64,Float32,Float64,Address)"));
    throw e;
}

void
BaseExtension::IfCmpEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpEqual, "IfCmpEqual"))
            break;
    }

    addOperation(b, new Op_IfCmpEqual(PASSLOC, this, b, aIfCmpEqual, target, left, right));
}

void
BaseExtension::IfCmpEqualZero(LOCATION, Builder *b, Builder *target, Value *value) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmpZero(PASSLOC, b, target, value, CompileFail_BadInputTypes_IfCmpEqualZero, "IfCmpEqualZero"))
            break;
    }

    addOperation(b, new Op_IfCmpEqualZero(PASSLOC, this, b, aIfCmpEqualZero, target, value));
}

void
BaseExtension::IfCmpGreaterThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpGreaterThan, "IfCmpGreaterThan"))
            break;
    }

    addOperation(b, new Op_IfCmpGreaterThan(PASSLOC, this, b, aIfCmpGreaterThan, target, left, right));
}

void
BaseExtension::IfCmpGreaterOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpGreaterOrEqual, "IfCmpGreaterOrEqual"))
            break;
    }

    addOperation(b, new Op_IfCmpGreaterOrEqual(PASSLOC, this, b, aIfCmpGreaterOrEqual, target, left, right));
}

void
BaseExtension::IfCmpLessThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpLessThan, "IfCmpLessThan"))
            break;
    }

    addOperation(b, new Op_IfCmpLessThan(PASSLOC, this, b, aIfCmpLessThan, target, left, right));
}

void
BaseExtension::IfCmpLessOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpLessOrEqual, "IfCmpLessOrEqual"))
            break;
    }

    addOperation(b, new Op_IfCmpLessOrEqual(PASSLOC, this, b, aIfCmpLessOrEqual, target, left, right));
}

void
BaseExtension::IfCmpNotEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpNotEqual, "IfCmpNotEqual"))
            break;
    }

    addOperation(b, new Op_IfCmpNotEqual(PASSLOC, this, b, aIfCmpNotEqual, target, left, right));
}

void
BaseExtension::IfCmpNotEqualZero(LOCATION, Builder *b, Builder *target, Value *value) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmpZero(PASSLOC, b, target, value, CompileFail_BadInputTypes_IfCmpNotEqualZero, "IfCmpNotEqualZero"))
            break;
    }

    addOperation(b, new Op_IfCmpNotEqualZero(PASSLOC, this, b, aIfCmpNotEqualZero, target, value));
}

void
BaseExtension::IfCmpUnsignedGreaterThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedGreaterThan, "IfCmpUnsignedGreaterThan"))
            break;
    }

    addOperation(b, new Op_IfCmpUnsignedGreaterThan(PASSLOC, this, b, aIfCmpUnsignedGreaterThan, target, left, right));
}

void
BaseExtension::IfCmpUnsignedGreaterOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedGreaterOrEqual, "IfCmpUnsignedGreaterOrEqual"))
            break;
    }

    addOperation(b, new Op_IfCmpUnsignedGreaterOrEqual(PASSLOC, this, b, aIfCmpUnsignedGreaterOrEqual, target, left, right));
}

void
BaseExtension::IfCmpUnsignedLessThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedLessThan, "IfCmpUnsignedLessThan"))
            break;
    }

    addOperation(b, new Op_IfCmpUnsignedLessThan(PASSLOC, this, b, aIfCmpUnsignedLessThan, target, left, right));
}

void
BaseExtension::IfCmpUnsignedLessOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        BaseExtensionChecker *checker = *it;
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedLessOrEqual, "IfCmpUnsignedLessOrEqual"))
            break;
    }

    addOperation(b, new Op_IfCmpUnsignedLessOrEqual(PASSLOC, this, b, aIfCmpUnsignedLessOrEqual, target, left, right));
}

void
BaseExtension::Return(LOCATION, Builder *b) {
    addOperation(b, new Op_Return(PASSLOC, this, b, this->aReturn));
}

void
BaseExtension::Return(LOCATION, Builder *b, Value *v) {
    addOperation(b, new Op_Return(PASSLOC, this, b, this->aReturn, v));
}

//
// Memory operations
//

Value *
BaseExtension::Load(LOCATION, Builder *b, Symbol * sym) {
    Value * result = createValue(b, sym->type());
    addOperation(b, new Op_Load(PASSLOC, this, b, this->aLoad, result, sym));
    return result;
}

void
BaseExtension::Store(LOCATION, Builder *b, Symbol * sym, Value *value) {
    addOperation(b, new Op_Store(PASSLOC, this, b, this->aStore, sym, value));
}

Value *
BaseExtension::LoadAt(LOCATION, Builder *b, Value *ptrValue) {
    assert(ptrValue->type()->isKind<PointerType>());
    const Type *baseType = ptrValue->type()->refine<PointerType>()->baseType();
    Value * result = createValue(b, baseType);
    addOperation(b, new Op_LoadAt(PASSLOC, this, b, this->aLoadAt, result, ptrValue));
    return result;
}

void
BaseExtension::StoreAt(LOCATION, Builder *b, Value *ptrValue, Value *value) {
    assert(ptrValue->type()->isKind<PointerType>());
    const Type *baseType = ptrValue->type()->refine<PointerType>()->baseType();
    assert(baseType == value->type());
    addOperation(b, new Op_StoreAt(PASSLOC, this, b, this->aStoreAt, ptrValue, value));
}

Value *
BaseExtension::LoadField(LOCATION, Builder *b, const FieldType *fieldType, Value *structValue) {
    assert(structValue->type()->isKind<StructType>());
    assert(fieldType->owningStruct() == structValue->type());
    Value * result = createValue(b, fieldType->type());
    addOperation(b, new Op_LoadField(PASSLOC, this, b, this->aLoadField, result, fieldType, structValue));
    return result;
}

void
BaseExtension::StoreField(LOCATION, Builder *b, const FieldType *fieldType, Value *structValue, Value *value) {
    assert(structValue->type()->isKind<StructType>());
    assert(fieldType->owningStruct() == structValue->type());
    addOperation(b, new Op_StoreField(PASSLOC, this, b, this->aStoreField, fieldType, structValue, value));
}

Value *
BaseExtension::LoadFieldAt(LOCATION, Builder *b, const FieldType *fieldType, Value *pStruct) {
    assert(pStruct->type()->isKind<PointerType>());
    const Type *structType = pStruct->type()->refine<PointerType>()->baseType();
    assert(fieldType->owningStruct() == structType);
    Value * result = createValue(b, fieldType->type());
    addOperation(b, new Op_LoadFieldAt(PASSLOC, this, b, this->aLoadFieldAt, result, fieldType, pStruct));
    return result;
}

void
BaseExtension::StoreFieldAt(LOCATION, Builder *b, const FieldType *fieldType, Value *pStruct, Value *value) {
    assert(pStruct->type()->isKind<PointerType>());
    const Type *structType = pStruct->type()->refine<PointerType>()->baseType();
    assert(fieldType->owningStruct() == structType);
    addOperation(b, new Op_StoreFieldAt(PASSLOC, this, b, this->aStoreFieldAt, fieldType, pStruct, value));
}

Value *
BaseExtension::CreateLocalArray(LOCATION, Builder *b, Literal *numElements, const PointerType *pElementType) {
   assert(numElements->type()->isKind<IntegerType>());
   Value * result = createValue(b, pElementType);
   const Type *elementType = pElementType->baseType();
   // assert concrete type
   addOperation(b, new Op_CreateLocalArray(PASSLOC, this, b, this->aCreateLocalArray, result, numElements, pElementType));
   return result;
}

Value *
BaseExtension::CreateLocalStruct(LOCATION, Builder *b, const PointerType *pStructType) {
    const Type *baseType = pStructType->baseType();
    assert(baseType->isKind<StructType>());
    const StructType *structType = baseType->refine<StructType>();
    Value * result = createValue(b, pStructType);
    addOperation(b, new Op_CreateLocalStruct(PASSLOC, this, b, this->aCreateLocalStruct, result, structType));
    return result;
}

Value *
BaseExtension::IndexAt(LOCATION, Builder *b, Value *base, Value *index) {
    const Type *pElementType = base->type();
    assert(pElementType->isKind<PointerType>());
    Value *result = createValue(b, pElementType);
    addOperation(b, new Op_IndexAt(PASSLOC, this, b, aIndexAt, result, base, index));
    return result;
}

//
// Pseudo operations
//
Location *
BaseExtension::SourceLocation(LOCATION, Builder *b, std::string func) {
    Location *loc = new Location(b->comp(), func, "");
    b->setLocation(loc);
    return loc;
}

Location *
BaseExtension::SourceLocation(LOCATION, Builder *b, std::string func, std::string lineNumber) {
    Location *loc = new Location(b->comp(), func, lineNumber);
    b->setLocation(loc);
    return loc;
}

Location *
BaseExtension::SourceLocation(LOCATION, Builder *b, std::string func, std::string lineNumber, int32_t bcIndex) {
    Location *loc = new Location(b->comp(), func, lineNumber, bcIndex);
    b->setLocation(loc);
    return loc;
}

Value *
BaseExtension::ConstInt8(LOCATION, Builder *b, int8_t v) {
    Literal *lv = this->Int8->literal(PASSLOC, b->comp(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstInt16(LOCATION, Builder *b, int16_t v) {
    Literal *lv = this->Int16->literal(PASSLOC, b->comp(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstInt32(LOCATION, Builder *b, int32_t v) {
    Literal *lv = this->Int32->literal(PASSLOC, b->comp(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstInt64(LOCATION, Builder *b, int64_t v) {
    Literal *lv = this->Int64->literal(PASSLOC, b->comp(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstFloat32(LOCATION, Builder *b, float v) {
    Literal *lv = this->Float32->literal(PASSLOC, b->comp(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstFloat64(LOCATION, Builder *b, double v) {
    Literal *lv = this->Float64->literal(PASSLOC, b->comp(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstAddress(LOCATION, Builder *b, void * v) {
    Literal *lv = this->Address->literal(PASSLOC, b->comp(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstPointer(LOCATION, Builder *b, const PointerType *type, void * v) {
    Literal *lv = type->literal(PASSLOC, b->comp(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::Zero(LOCATION, Builder *b, const Type *type) {
    Literal *zero = type->zero(PASSLOC, b->comp());
    assert(zero);
    return Const(PASSLOC, b, zero);
}

Value *
BaseExtension::One(LOCATION, Builder *b, const Type *type) {
    Literal *one = type->identity(PASSLOC, b->comp());
    assert(one);
    return Const(PASSLOC, b, one);
}

void
BaseExtension::Increment(LOCATION, Builder *b, Symbol *sym, Value *bump) {
    Value *oldValue = Load(PASSLOC, b, sym);
    Value *newValue = Add(PASSLOC, b, oldValue, bump);
    Store(PASSLOC, b, sym, newValue);
}

void
BaseExtension::Increment(LOCATION, Builder *b, LocalSymbol *sym) {
   Value *oldValue = Load(PASSLOC, b, sym);
   Value *newValue = Add(PASSLOC, b, oldValue, One(PASSLOC, b, sym->type()));
   Store(PASSLOC, b, sym, newValue);
}

void
BaseExtension::failValidateOffsetAt(LOCATION, Builder *b, Value *array) {
    CompilationException e(PASSLOC, _compiler, CompileFail_BadInputArray_OffsetAt);
    const Type *arrayType = array->type();
    e.setMessageLine(std::string("OffsetAt: invalid array type"))
     .appendMessageLine(std::string("   array ").append(arrayType->to_string()))
     .appendMessageLine(std::string("Array type must be a PointerType"));
    throw e;
}

Value *
BaseExtension::OffsetAt(LOCATION, Builder *b, Value *array, size_t elementIndex) {
    const Type *pElement = array->type();
    if (!pElement->isKind<Base::PointerType>())
        failValidateOffsetAt(PASSLOC, b, array);

    const Type *Element = pElement->refine<PointerType>()->baseType();
    size_t offset = elementIndex; // * Element->size()/8;
    Literal *elementOffset = this->Word->literal(PASSLOC, b->comp(), reinterpret_cast<LiteralBytes *>(&offset));
    return IndexAt(PASSLOC, b, array, Const(PASSLOC, b, elementOffset));
}

Value *
BaseExtension::LoadArray(LOCATION, Builder *b, Value *array, size_t elementIndex) {
    Value *pElement = OffsetAt(PASSLOC, b, array, elementIndex);
    return this->LoadAt(PASSLOC, b, pElement);
}

void
BaseExtension::StoreArray(LOCATION, Builder *b, Value *array, size_t elementIndex, Value *value) {
    Value *pElement = this->OffsetAt(PASSLOC, b, array, elementIndex);
    StoreAt(PASSLOC, b, pElement, value);
}

Value *
BaseExtension::OffsetAt(LOCATION, Builder *b, Value *array, Value * indexValue) {
    const Type *pElement = array->type();
    if (!pElement->isKind<Base::PointerType>())
        failValidateOffsetAt(PASSLOC, b, array);

    //const Type *Element = pElement->refine<Base::PointerType>()->baseType();
    //size_t size = Element->size() / 8;
    //Value *elementSize = this->Const(PASSLOC, b, this->Word->literal(PASSLOC, b->comp(), reinterpret_cast<LiteralBytes *>(&size)));
    //Value *offsetValue = this->Mul(PASSLOC, b, indexValue, elementSize);
    return this->IndexAt(PASSLOC, b, array, indexValue); //offsetValue);
}

Value *
BaseExtension::LoadArray(LOCATION, Builder *b, Value *array, Value *indexValue) {
    Value *pElement = this->OffsetAt(PASSLOC, b, array, indexValue);
    return LoadAt(PASSLOC, b, pElement);
}

void
BaseExtension::StoreArray(LOCATION, Builder *b, Value *array, Value *indexValue, Value *value) {
    Value *pElement = OffsetAt(PASSLOC, b, array, indexValue);
    StoreAt(PASSLOC, b, pElement, value);
}


const PointerType *
BaseExtension::PointerTo(LOCATION, FunctionCompilation *comp, const Type *baseType) {
    PointerTypeBuilder pb(this, comp);
    pb.setBaseType(baseType);
    return pb.create(PASSLOC);
}

const FunctionType *
BaseExtension::DefineFunctionType(LOCATION, FunctionCompilation *comp, const Type *returnType, int32_t numParms, const Type **parmTypes) {
    const FunctionType *fType = comp->lookupFunctionType(returnType, numParms, parmTypes);
    if (fType)
        return fType;

    const FunctionType *f = new FunctionType(PASSLOC, this, comp->dict(), returnType, numParms, parmTypes);
    comp->registerFunctionType(f);
    return f;
}

CompilerReturnCode
BaseExtension::jb1cgCompile(Compilation *comp) {
    return _compiler->compile(comp, _jb1cgStrategyID);
}

#if 0
// keep handy during migration
Value *
BuilderBase::CoercePointer(Type * t, Value * v)
   {
   if (!(v->type()->isPointer() || v->type() == Address) || !t->isPointer())
      creationError(aCoercePointer, "type", t, "value", v);

   Value * result = Value::create(self(), t);
   add(OMR::JitBuilder::CoercePointer::create(self(), result, t, v));
   return result;
   }

Value *
BuilderBase::Add (Value * left, Value * right)
   {
   Type *returnType = dict()->producedType(aAdd, left, right);
   if (!returnType)
      creationError(aAdd, "left", left, "right", right);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::Add::create(self(), result, left, right));
   return result;
   }

Value *
BuilderBase::Sub (Value * left, Value * right)
   {
   Type *returnType = dict()->producedType(aSub, left, right);
   if (!returnType)
      creationError(aSub, "left", left, "right", right);

   Value * result = Value::create(self(), returnType);

   add(OMR::JitBuilder::Sub::create(self(), result, left, right));
   return result;
   }

Value *
BuilderBase::Mul (Value * left, Value * right)
   {
   Type *returnType = dict()->producedType(aMul, left, right);
   if (!returnType)
      creationError(aMul, "left", left, "right", right);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::Mul::create(self(), result, left, right));
   return result;
   }

Value *
BuilderBase::IndexAt (Type * type, Value * base, Value * index)
   {
   Type *returnType = dict()->producedType(aIndexAt, base, index);
   if (!returnType)
      creationError(aIndexAt, "type", type, "base", base, "index", index);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::IndexAt::create(self(), result, type, base, index));
   return result;
   }

Value *
BuilderBase::Load(Symbol *local)
   {
   Type *returnType = local->type();
   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::Load::create(self(), result, local));
   return result;
   }

Value *
BuilderBase::Load (std::string name)
   {
   Symbol *local = _fb->getSymbol(name);
   if (!local)
      creationError(aLoad, "localName", name);
   return Load(local);
   }

Value *
BuilderBase::LoadAt (Type * type, Value * address)
   {
   Type *returnType = dict()->producedType(aLoadAt, address);
   if (!returnType)
      creationError(aLoadAt, "type", type, "address", address);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::LoadAt::create(self(), result, type, address));
   return result;
   }

Value *
BuilderBase::LoadField (std::string structName, std::string fieldName, Value * structBase)
   {
   StructType *structType = dict()->LookupStruct(structName);
   FieldType *fieldType = structType->LookupField(fieldName);
   assert(fieldType);
   return LoadField(fieldType, structBase);
   }

Value *
BuilderBase::LoadField (FieldType *fieldType, Value *structBase)
   {
   StructType *structType = fieldType->owningStruct();
   Type *returnType = dict()->producedType(aLoadField, fieldType, structBase);
   if (!returnType)
      creationError(aLoadField, "struct", structType->name(), "field", fieldType->name(), "base", structBase);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::LoadField::create(self(), result, fieldType, structBase));
   return result;
   }

Value *
BuilderBase::LoadIndirect (std::string structName, std::string fieldName, Value * pStructBase)
   {
   StructType *structType = dict()->LookupStruct(structName);
   FieldType *fieldType = structType->LookupField(fieldName);
   assert(fieldType);
   return LoadIndirect(fieldType, pStructBase);
   }

Value *
BuilderBase::LoadIndirect (FieldType *fieldType, Value *pStructBase)
   {
   StructType *structType = fieldType->owningStruct();
   Type *returnType = dict()->producedType(aLoadIndirect, fieldType, pStructBase);
   if (!returnType)
      creationError(aLoadIndirect, "struct", structType->name(), "field", fieldType->name(), "basePtr", pStructBase);

   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::LoadIndirect::create(self(), result, fieldType, pStructBase));
   return result;
   }

void
BuilderBase::Store(Symbol * local, Value * value)
   {
   add(OMR::JitBuilder::Store::create(self(), local, value));
   }
   
void
BuilderBase::Store (std::string name, Value * value)
   {
   Symbol *local = fb()->getSymbol(name);
   if (local == NULL)
      {
      fb()->DefineLocal(name, value->type());
      local = fb()->getSymbol(name);
      }
   add(OMR::JitBuilder::Store::create(self(), local, value));
   }

void
BuilderBase::StoreAt (Value * address, Value * value)
   {
   Type *returnType = dict()->producedType(aStoreAt, address, value);
   if (returnType != NoType)
      creationError(aStoreAt, "address", address, "value", value);

   add(OMR::JitBuilder::StoreAt::create(self(), address, value));
   }

void
BuilderBase::StoreField (std::string structName, std::string fieldName, Value * structBase, Value *value)
   {
   StructType *structType = dict()->LookupStruct(structName);
   FieldType *fieldType = structType->LookupField(fieldName);
   assert(fieldType);
   return StoreField(fieldType, structBase, value);
   }

void
BuilderBase::StoreField (FieldType *fieldType, Value *structBase, Value *value)
   {
   StructType *structType = fieldType->owningStruct();
   Type *returnType = dict()->producedType(aStoreField, fieldType, structBase, value);
   if (structType != fieldType->owningStruct() || returnType != NoType)
      creationError(aStoreField, "struct", structType->name(), "field", fieldType->name(), "base", structBase, "value", value);

   add(OMR::JitBuilder::StoreField::create(self(), fieldType, structBase, value));
   }

void
BuilderBase::StoreIndirect (std::string structName, std::string fieldName, Value * pStructBase, Value *value)
   {
   StructType *structType = dict()->LookupStruct(structName);
   FieldType *fieldType = structType->LookupField(fieldName);
   assert(fieldType);
   return StoreIndirect(fieldType, pStructBase, value);
   }

void
BuilderBase::StoreIndirect (FieldType *fieldType, Value *pStructBase, Value *value)
   {
   StructType *structType = fieldType->owningStruct();
   Type *returnType = dict()->producedType(aStoreIndirect, fieldType, pStructBase, value);
   if (structType != fieldType->owningStruct() || returnType != NoType)
      creationError(aStoreIndirect, "struct", structType->name(), "field", fieldType->name(), "basePtr", pStructBase, "value", value);

   add(OMR::JitBuilder::StoreIndirect::create(self(), fieldType, pStructBase, value));
   }

void
BuilderBase::AppendBuilder(Builder * b)
   {
   Operation * appendBuilderOp = OMR::JitBuilder::AppendBuilder::create(self(), b);
   add(appendBuilderOp);
   b->setBoundness(May)
    ->setBound(appendBuilderOp)
    ->setBoundness(Must);
   _controlReachesEnd = true; // AppendBuilder establishes a label so control can now reach the end of this builder even if earlier it could not
   }

Value *
BuilderBase::Call(Value *func, int32_t numArgs, ...)
   {
   assert(func->type()->isKind<FunctionType>());

   va_list args ;
   va_start(args, numArgs);
   FunctionType *function = func->type()->refine<FunctionType>();
   Type *returnType = dict()->producedType(function, numArgs, args);
   if (returnType == NULL)
      creationError(aCall, "functionType", func, numArgs, args);
   va_end(args);

   Value * result = NULL;
   if (returnType != NoType)
      result = Value::create(self(), returnType);
   va_list args2 ;
   va_start(args2, numArgs);
   Operation * callOp = OMR::JitBuilder::Call::create(self(), result, func, numArgs, args2);
   add(callOp);
   va_end(args2);
   return result;
   }

Value *
BuilderBase::Call(Value *func, int32_t numArgs, Value **args)
   {
   assert(func->type()->isKind<FunctionType>());
   FunctionType *function = func->type()->refine<FunctionType>();
   Type *returnType = dict()->producedType(function, numArgs, args);
   if (returnType == NULL)
      creationError(aCall, "functionType", func, numArgs, args);

   Value * result = NULL;
   if (returnType != NoType)
      result = Value::create(self(), returnType);
   Operation * callOp = OMR::JitBuilder::Call::create(self(), result, func, numArgs, args);
   add(callOp);
   return result;
   }

void
BuilderBase::Goto(Builder * b)
   {
   Operation * gotoOp = OMR::JitBuilder::Goto::create(self(), b);
   add(gotoOp);
   _controlReachesEnd = false; // Goto definitely leaves this builder object
   }

void
BuilderBase::IfCmpGreaterThan(Builder * gtBuilder, Value * left, Value * right)
   {
   Type *returnType = dict()->producedType(aIfCmpGreaterThan, left, right);
   if (!returnType || returnType != NoType)
      creationError(aIfCmpGreaterThan, "left", left, "right", right);
   add(OMR::JitBuilder::IfCmpGreaterThan::create(self(), gtBuilder, left, right));
   gtBuilder->setTarget()
            ->setBoundness(Cant);
   }

void
BuilderBase::IfCmpLessThan(Builder * ltBuilder, Value * left, Value * right)
   {
   Type *returnType = dict()->producedType(aIfCmpLessThan, left, right);
   if (!returnType || returnType != NoType)
      creationError(aIfCmpLessThan, "left", left, "right", right);
   add(OMR::JitBuilder::IfCmpLessThan::create(self(), ltBuilder, left, right));
   ltBuilder->setTarget()
            ->setBoundness(Cant);
   }

void
BuilderBase::IfCmpGreaterOrEqual(Builder * goeBuilder, Value * left, Value * right)
   {
   Type *returnType = dict()->producedType(aIfCmpGreaterOrEqual, left, right);
   if (!returnType || returnType != NoType)
      creationError(aIfCmpGreaterOrEqual, "left", left, "right", right);
   add(OMR::JitBuilder::IfCmpGreaterOrEqual::create(self(), goeBuilder, left, right));
   goeBuilder->setTarget()
             ->setBoundness(Cant);
   }

void
BuilderBase::IfCmpLessOrEqual(Builder * loeBuilder, Value * left, Value * right)
   {
   Type *returnType = dict()->producedType(aIfCmpLessOrEqual, left, right);
   if (!returnType || returnType != NoType)
      creationError(aIfCmpLessOrEqual, "left", left, "right", right);
   add(OMR::JitBuilder::IfCmpLessOrEqual::create(self(), loeBuilder, left, right));
   loeBuilder->setTarget()
             ->setBoundness(Cant);
   }

void
BuilderBase::IfThenElse(Builder * thenB, Builder * elseB, Value * cond)
   {
   if (thenB && thenB->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because thenB builder cannot be bound");
   if (elseB && elseB->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because elseB builder cannot be bound");

   Operation *ifThenElseOp = OMR::JitBuilder::IfThenElse::create(self(), thenB, elseB, cond);
   add(ifThenElseOp);
   if (thenB)
      thenB->setTarget()
           ->setBoundness(May)
           ->setBound(ifThenElseOp)
           ->setBoundness(Must);
   if (elseB)
      elseB->setTarget()
           ->setBoundness(May)
           ->setBound(ifThenElseOp)
           ->setBoundness(Must);
   }

void
BuilderBase::ForLoopUp(std::string loopVar, Builder * body, Value * initial, Value * end, Value * bump)
   {
   LocalSymbol *loopSym = NULL;
   Symbol *sym = fb()->getSymbol(loopVar);
   if (sym && sym->isKind<LocalSymbol>())
      loopSym = sym->refine<LocalSymbol>();
   else
      loopSym = fb()->DefineLocal(loopVar, initial->type());
   ForLoopUp(loopSym, body, initial, end, bump);
   }

void
BuilderBase::ForLoopUp(LocalSymbol *loopSym, Builder * body, Value * initial, Value * end, Value * bump)
   {
   Type *returnType = dict()->producedType(aForLoop, initial, end, bump);
   if (!returnType || returnType != NoType)
      creationError(aForLoop, "initial", initial, "end", end, "bump", bump);
   if (body->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because body builder cannot be bound");

   Operation * forLoopOp = OMR::JitBuilder::ForLoop::create(self(), true, loopSym, body, initial, end, bump);
   add(forLoopOp);
   }

void
BuilderBase::ForLoop(bool countsUp, std::string loopVar, Builder * loopBody, Builder * loopContinue, Builder * loopBreak, Value * initial, Value * end, Value * bump)
   {
   LocalSymbol *loopSym = NULL;
   Symbol *sym = fb()->getSymbol(loopVar);
   if (sym && sym->isKind<LocalSymbol>())
      loopSym = sym->refine<LocalSymbol>();
   else
      loopSym = fb()->DefineLocal(loopVar, initial->type());
   ForLoop(countsUp, loopSym, loopBody, loopContinue, loopBreak, initial, end, bump);
   }

void
BuilderBase::ForLoop(bool countsUp, LocalSymbol *loopSym, Builder * loopBody, Builder * loopContinue, Builder * loopBreak, Value * initial, Value * end, Value * bump)
   {
   Type *returnType = dict()->producedType(aForLoop, initial, end, bump);
   if (!returnType || returnType != NoType)
      creationError(aForLoop, "initial", initial, "end", end, "bump", bump);
   if (loopBody->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because loopBody builder cannot be bound");
   if (loopContinue && loopContinue->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because loopContinue builder cannot be bound");
   if (loopBreak && loopBreak->boundness() == Cant)
      creationError(aIfThenElse, "Operation invalid because loopBreak builder cannot be bound");

   Operation * forLoopOp = (OMR::JitBuilder::ForLoop::create(self(), countsUp, loopSym, loopBody, loopContinue, loopBreak, initial, end, bump));
   add(forLoopOp);
   loopBody->setTarget()
           ->setBoundness(May)
           ->setBound(forLoopOp)
           ->setBoundness(Must);
   if (loopContinue)
      loopContinue->setBoundness(May)
                  ->setBound(forLoopOp)
                  ->setBoundness(Must);
   if (loopBreak)
      loopBreak->setBoundness(May)
               ->setBound(forLoopOp)
               ->setBoundness(Must);
   }

void
BuilderBase::Switch(Value * selector, Builder * defaultBuilder, int numCases, Case ** cases)
   {
   Type *returnType = dict()->producedType(aSwitch, selector);
   if (!returnType || returnType != NoType)
      creationError(aSwitch, "selector", selector);
   Operation *switchOp = OMR::JitBuilder::Switch::create(self(), selector, defaultBuilder, numCases, cases);
   add(switchOp);
   if (defaultBuilder)
      defaultBuilder->setTarget()
                    ->setBoundness(May)
                    ->setBound(switchOp)
                    ->setBoundness(Must);
   for (auto c=0;c < numCases;c++)
      {
      Case *thisCase = cases[c];
      Builder *b = thisCase->builder();
      b->setTarget()
       ->setBoundness(May)
       ->setBound(switchOp)
       ->setBoundness(Must);
      }
   }

void
BuilderBase::Return()
   {
   if (_fb->getReturnType() != NoType)
      creationError(aReturn, "expected type", _fb->getReturnType(), "returned type", NoType);

   add(OMR::JitBuilder::Return::create(self()));
   if (boundness() == Must)
      creationError(aReturn, "Operation invalid because target builder is bound");
   setBoundness(Cant);
   }

void
BuilderBase::Return(Value *v)
   {
   Type *returnType = dict()->producedType(aReturn, v);
   if (v->type() != _fb->getReturnType() || !returnType || returnType != NoType)
      creationError(aReturn, "expected type", _fb->getReturnType(), "returned type", v->type());

   add(OMR::JitBuilder::Return::create(self(), v));
   if (boundness() == Must)
      creationError(aReturn, "Operation invalid because target builder is bound");
   setBoundness(Cant);
   }

Value *
BuilderBase::CreateLocalArray(int32_t numElements, Type *elementType)
   {
   Type *returnType = dict()->PointerTo(elementType);
   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::CreateLocalArray::create(self(), result, numElements, elementType));
   return result;
   }

Value *
BuilderBase::CreateLocalStruct(Type *structType)
   {
   Type *returnType = dict()->PointerTo(structType);
   Value * result = Value::create(self(), returnType);
   add(OMR::JitBuilder::CreateLocalStruct::create(self(), result, structType));
   return result;
   }
#endif

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

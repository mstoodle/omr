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

#include <cstdarg>

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/ArithmeticOperations.hpp"
#include "Base/BaseIRAddon.hpp"
#include "Base/BaseExtension.hpp"
#include "Base/BaseFunctionExtensionAddon.hpp"
#include "Base/BaseIRClonerAddon.hpp"
#include "Base/BaseSymbol.hpp"
#include "Base/BaseTypes.hpp"
#include "Base/ConstOperation.hpp"
#include "Base/ControlOperations.hpp"
#include "Base/MemoryOperations.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

INIT_JBALLOC_REUSECAT(BaseExtension, Extension)
SUBCLASS_KINDSERVICE_IMPL(BaseExtension,"BaseExtension",Extension,Extensible);

const SemanticVersion BaseExtension::version(BASEEXT_MAJOR,BASEEXT_MINOR,BASEEXT_PATCH);
const String BaseExtension::NAME("jb2base");

static const MajorID NEEDED_FUNCEXT_MAJOR=0;
static const MajorID NEEDED_FUNCEXT_MINOR=1;
static const MajorID NEEDED_FUNCEXT_PATCH=0;
const static SemanticVersion neededFuncVersion(NEEDED_FUNCEXT_MAJOR,NEEDED_FUNCEXT_MINOR,NEEDED_FUNCEXT_PATCH);

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        Allocator *mem = compiler->mem();
        return new (mem) BaseExtension(MEM_PASSLOC(mem), compiler);
    }
}

BaseExtension::BaseExtension(MEM_LOCATION(a), Compiler *compiler, bool extended, String extensionName)
    : Extension(MEM_PASSLOC(a), CLASSKIND(BaseExtension,Extensible), compiler, (extended ? extensionName : NAME))
    , _cx(compiler->coreExt()) 
    , Int8(new (a) Int8Type(MEM_PASSLOC(a), this))
    , Int16(new (a) Int16Type(MEM_PASSLOC(a), this))
    , Int32(new (a) Int32Type(MEM_PASSLOC(a), this))
    , Int64(new (a) Int64Type(MEM_PASSLOC(a), this))
    , Float32(new (a) Float32Type(MEM_PASSLOC(a), this))
    , Float64(new (a) Float64Type(MEM_PASSLOC(a), this))
    , Address(new (a) AddressType(MEM_PASSLOC(a), this))
    , Word(compiler->platformWordSize() == 64 ? static_cast<const Type *>(this->Int64) : static_cast<const Type *>(this->Int32))
    , aConst(registerAction(String("Const")))
    , aAdd(registerAction(String("Add")))
    , aConvertTo(registerAction(String("ConvertTo")))
    , aMul(registerAction(String("Mul")))
    , aSub(registerAction(String("Sub")))
    , aLoadAt(registerAction(String("LoadAt")))
    , aStoreAt(registerAction(String("StoreAt")))
    , aLoadField(registerAction(String("LoadField")))
    , aStoreField(registerAction(String("StoreField")))
    , aLoadFieldAt(registerAction(String("LoadFieldAt")))
    , aStoreFieldAt(registerAction(String("StoreFieldAt")))
    , aCreateLocalArray(registerAction(String("CreateLocalArray")))
    , aCreateLocalStruct(registerAction(String("CreateLocalStruct")))
    , aIndexAt(registerAction(String("IndexAt")))
    , aCall(registerAction(String("Call")))
    , aCallVoid(registerAction(String("CallVoid")))
    , aForLoopUp(registerAction(String("ForLoopUp")))
    , aGoto(registerAction(String("Goto")))
    , aIfCmpEqual(registerAction(String("IfCmpEqual")))
    , aIfCmpEqualZero(registerAction(String("IfCmpEqualZero")))
    , aIfCmpGreaterThan(registerAction(String("IfCmpGreaterThan")))
    , aIfCmpGreaterOrEqual(registerAction(String("IfCmpGreaterOrEqual")))
    , aIfCmpLessThan(registerAction(String("IfCmpLessThan")))
    , aIfCmpLessOrEqual(registerAction(String("IfCmpLessOrEqual")))
    , aIfCmpNotEqual(registerAction(String("IfCmpNotEqual")))
    , aIfCmpNotEqualZero(registerAction(String("IfCmpNotEqualZero")))
    , aIfCmpUnsignedGreaterThan(registerAction(String("IfCmpUnsignedGreaterThan")))
    , aIfCmpUnsignedGreaterOrEqual(registerAction(String("IfCmpUnsignedGreaterOrEqual")))
    , aIfCmpUnsignedLessThan(registerAction(String("IfCmpUnsignedLessThan")))
    , aIfCmpUnsignedLessOrEqual(registerAction(String("IfCmpUnsignedLessOrEqual")))
    , aIfThenElse(registerAction(String("IfThenElse")))
    , aSwitch(registerAction(String("Switch")))
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
    , CompileFail_BadInputTypes_IfThenElse(registerReturnCode("CompileFail_BadInputTypes_IfThenElse"))
    , CompileFail_BadInputTypes_Switch(registerReturnCode("CompileFail_BadInputTypes_Switch"))
    , CompileFail_BadInputArray_OffsetAt(registerReturnCode("CompileFail_BadInputArray_OffsetAt"))
    , CompileFail_MismatchedArgumentTypes_Call(registerReturnCode("CompileFail_MismatchedArgumentTypes_Call"))
    , _checkers(NULL, a) {

    if (!extended) {
        _checkers.push_back(new (a) BaseExtensionChecker(a, this));
    }

    registerForExtensible(CLASSKIND(IR,Extensible), this);
    registerForExtensible(CLASSKIND(IRCloner,Extensible), this);
    
    Func::FunctionExtension *fx = compiler->lookupExtension<Func::FunctionExtension>();
    if (fx != NULL)
        createAddon(fx);
    else
        registerForExtensible(CLASSKIND(Func::FunctionExtension,Extensible), this);

    // if more are added, need to update createAddon to decode
}

BaseExtension::~BaseExtension() {
    for (auto it=_checkers.iterator();it.hasItem();it++) {
        BaseExtensionChecker *checker = it.item();
        delete checker;
    }
}

void
BaseExtension::registerChecker(BaseExtensionChecker *checker) {
    _checkers.push_front(checker);
}

void
BaseExtension::createAddon(Extensible *e) {
    Allocator *mem = e->allocator();

    if (e->isKind<IR>()) {
        BaseIRAddon *bc = new (mem) BaseIRAddon(mem, this, e->refine<IR>());
        e->attach(bc);
    } else if (e->isKind<Func::FunctionExtension>()) {
        BaseFunctionExtensionAddon *bfe = new (mem) BaseFunctionExtensionAddon(mem, e->refine<Func::FunctionExtension>(), this);
        e->attach(bfe);
    } else if (e->isKind<IRCloner>()) {
        BaseIRClonerAddon *bc = new (mem) BaseIRClonerAddon(mem, this, e->refine<IRCloner>());
        e->attach(bc);
    }
}


//
// Const Operations
//

Value *
BaseExtension::Const(LOCATION, Builder *b, Literal * lv) {
    Allocator *mem = b->ir()->mem();
    Value * result = createValue(b, lv->type());
    addOperation(b, new (mem) Op_Const(MEM_PASSLOC(mem), this, b, this->aConst, result, lv));
    return result;
}


BaseExtensionChecker::~BaseExtensionChecker() {
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
    e.setMessageLine(String("Add: invalid input types"))
     .appendMessageLine(String("    left ").append(lType->to_string()))
     .appendMessageLine(String("   right ").append(rType->to_string()))
     .appendMessageLine(String("Left and right types are expected to be the same for integer types (Int8,Int16,Int32,Int64,Float32,Float64)"))
     .appendMessageLine(String("If left/right type is Address then the right/left (respectively) type must be Word"));
    throw e;
}

Value *
BaseExtension::Add(LOCATION, Builder *b, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateAdd(PASSLOC, b, left, right))
            break;
    }

    if (right->type() == Address) { // reverse left and right
        Value *save = left;
        left = right;
        right = save;
    }

    Allocator *mem = b->ir()->mem();
    Value *result = createValue(b, left->type());
    addOperation(b, new (mem) Op_Add(MEM_PASSLOC(mem), this, b, aAdd, result, left, right));
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
    e.setMessageLine(String("ConvertTo: invalid input types"))
     .appendMessageLine(String("    type ").append(type->to_string()))
     .appendMessageLine(String("   value ").append(vType->to_string()))
     .appendMessageLine(String("Source value and destination types must be a primitive type (Int8,Int16,Int32,Int64,Float32,Float64,Address)"));
    throw e;
}

Value *
BaseExtension::ConvertTo(LOCATION, Builder *b, const Type *type, Value *value) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateConvertTo(PASSLOC, b, type, value))
            break;
    }

    Allocator *mem = b->ir()->mem();
    Value *result = createValue(b, type);
    addOperation(b, new (mem) Op_ConvertTo(MEM_PASSLOC(mem), this, b, aConvertTo, result, type, value));
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
    e.setMessageLine(String("Mul: invalid input types"))
     .appendMessageLine(String("    left ").append(lType->to_string()))
     .appendMessageLine(String("   right ").append(rType->to_string()))
     .appendMessageLine(String("Left and right types are expected to be the same for integer types (Int8,Int16,Int32,Int64,Float32,Float64)"))
     .appendMessageLine(String("Address types cannot be used"));
    throw e;
}

Value *
BaseExtension::Mul(LOCATION, Builder *b, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateMul(PASSLOC, b, left, right))
            break;
    }

    Allocator *mem = b->ir()->mem();
    Value *result = createValue(b, left->type());
    addOperation(b, new (mem) Op_Mul(MEM_PASSLOC(mem), this, b, aMul, result, left, right));
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
    e.setMessageLine(String("Sub: invalid input types"))
     .appendMessageLine(String("    left ").append(lType->to_string()))
     .appendMessageLine(String("   right ").append(rType->to_string()))
     .appendMessageLine(String("Left and right types are expected to be the same for integer types (Int8,Int16,Int32,Int64,Float32,Float64)"))
     .appendMessageLine(String("If left type is Address then the right type must be either Address or Word"));
    throw e;
}

Value *
BaseExtension::Sub(LOCATION, Builder *b, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateSub(PASSLOC, b, left, right))
            break;
    }

    Allocator *mem = b->ir()->mem();
    Value *result = createValue(b, left->type());
    addOperation(b, new (mem) Op_Sub(MEM_PASSLOC(mem), this, b, aSub, result, left, right));
    return result;
}

//
// Control operations
//

bool
BaseExtensionChecker::validateForLoopUp(LOCATION, Builder *b, Symbol *loopVariable, Value *initial, Value *final, Value *bump) {
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
BaseExtensionChecker::failValidateForLoopUp(LOCATION, Builder *b, Symbol *loopVariable, Value *initial, Value *final, Value *bump) {
    CompilationException e(PASSLOC, _base->compiler(), _base->CompileFail_BadInputTypes_ForLoopUp);
    e.setMessageLine(String("ForLoopUp: invalid input types"))
     .appendMessageLine(String("  loop var s").append(String::to_string(loopVariable->id())).append(" ").append(loopVariable->name()).append(" ").append(loopVariable->type()->to_string()))
     .appendMessageLine(String("   initial v").append(String::to_string(initial->id())).append(" ").append(initial->type()->to_string()))
     .appendMessageLine(String("     final v").append(String::to_string(final->id())).append(" ").append(final->type()->to_string()))
     .appendMessageLine(String("      bump v").append(String::to_string(final->id())).append(" ").append(bump->type()->to_string()))
     .appendMessageLine(String("Loop variable must be one of Int8, Int16, Int32, or Int64, and the types of initial, final, and bump must be same as the loop variable's type"));
    throw e;
}

ForLoopBuilder
BaseExtension::ForLoopUp(LOCATION, Builder *b, Symbol *loopVariable,
                         Value *initial, Value *final, Value *bump) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateForLoopUp(PASSLOC, b, loopVariable, initial, final, bump))
            break;
    }

    ForLoopBuilder loopBuilder;
    loopBuilder.setLoopVariable(loopVariable)
               .setInitialValue(initial)
               .setFinalValue(final)
               .setBumpValue(bump);
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_ForLoopUp(MEM_PASSLOC(mem), this, b, this->aForLoopUp, &loopBuilder));
    return loopBuilder;
}


void
BaseExtension::Goto(LOCATION, Builder *b, Builder *target) {
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_Goto(MEM_PASSLOC(mem), this, b, this->aGoto, target));
}


bool
BaseExtensionChecker::validateIfCmp(LOCATION, Builder *b, Builder *target, Value *left, Value *right, CompilerReturnCode failCode, String opCodeName) {
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
BaseExtensionChecker::failValidateIfCmp(LOCATION, Builder *b, Builder *target, Value *left, Value *right, CompilerReturnCode failCode, String opCodeName) {
    CompilationException e(PASSLOC, _base->compiler(), failCode);
    const Type *lType = left->type();
    const Type *rType = right->type();
    e.setMessageLine(opCodeName.append(String(": invalid input types")))
     .appendMessageLine(String("    left ").append(lType->to_string()))
     .appendMessageLine(String("   right ").append(rType->to_string()))
     .appendMessageLine(String("  target ").append(target->to_string()))
     .appendMessageLine(String("Left and right types are expected to be the same type (Int8,Int16,Int32,Int64,Float32,Float64,Address)"));
    throw e;
}

bool
BaseExtensionChecker::validateIfCmpZero(LOCATION, Builder *b, Builder *target, Value *value, CompilerReturnCode failCode, String opCodeName) {
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
BaseExtensionChecker::failValidateIfCmpZero(LOCATION, Builder *b, Builder *target, Value *value, CompilerReturnCode failCode, String opCodeName) {
    CompilationException e(PASSLOC, _base->compiler(), failCode);
    const Type *type = value->type();
    e.setMessageLine(opCodeName.append(String(": invalid input types")))
     .appendMessageLine(String("   value ").append(type->to_string()))
     .appendMessageLine(String("  target ").append(target->to_string()))
     .appendMessageLine(String("Value type is expected to be a primitive type (Int8,Int16,Int32,Int64,Float32,Float64,Address)"));
    throw e;
}

void
BaseExtension::IfCmpEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpEqual, "IfCmpEqual"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpEqual(MEM_PASSLOC(mem), this, b, aIfCmpEqual, target, left, right));
}

void
BaseExtension::IfCmpEqualZero(LOCATION, Builder *b, Builder *target, Value *value) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmpZero(PASSLOC, b, target, value, CompileFail_BadInputTypes_IfCmpEqualZero, "IfCmpEqualZero"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpEqualZero(MEM_PASSLOC(mem), this, b, aIfCmpEqualZero, target, value));
}

void
BaseExtension::IfCmpGreaterThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpGreaterThan, "IfCmpGreaterThan"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpGreaterThan(MEM_PASSLOC(mem), this, b, aIfCmpGreaterThan, target, left, right));
}

void
BaseExtension::IfCmpGreaterOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpGreaterOrEqual, "IfCmpGreaterOrEqual"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpGreaterOrEqual(MEM_PASSLOC(mem), this, b, aIfCmpGreaterOrEqual, target, left, right));
}

void
BaseExtension::IfCmpLessThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpLessThan, "IfCmpLessThan"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpLessThan(MEM_PASSLOC(mem), this, b, aIfCmpLessThan, target, left, right));
}

void
BaseExtension::IfCmpLessOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpLessOrEqual, "IfCmpLessOrEqual"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpLessOrEqual(MEM_PASSLOC(mem), this, b, aIfCmpLessOrEqual, target, left, right));
}

void
BaseExtension::IfCmpNotEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpNotEqual, "IfCmpNotEqual"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpNotEqual(MEM_PASSLOC(mem), this, b, aIfCmpNotEqual, target, left, right));
}

void
BaseExtension::IfCmpNotEqualZero(LOCATION, Builder *b, Builder *target, Value *value) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmpZero(PASSLOC, b, target, value, CompileFail_BadInputTypes_IfCmpNotEqualZero, "IfCmpNotEqualZero"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpNotEqualZero(MEM_PASSLOC(mem), this, b, aIfCmpNotEqualZero, target, value));
}

void
BaseExtension::IfCmpUnsignedGreaterThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedGreaterThan, "IfCmpUnsignedGreaterThan"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpUnsignedGreaterThan(MEM_PASSLOC(mem), this, b, aIfCmpUnsignedGreaterThan, target, left, right));
}

void
BaseExtension::IfCmpUnsignedGreaterOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedGreaterOrEqual, "IfCmpUnsignedGreaterOrEqual"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpUnsignedGreaterOrEqual(MEM_PASSLOC(mem), this, b, aIfCmpUnsignedGreaterOrEqual, target, left, right));
}

void
BaseExtension::IfCmpUnsignedLessThan(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedLessThan, "IfCmpUnsignedLessThan"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpUnsignedLessThan(MEM_PASSLOC(mem), this, b, aIfCmpUnsignedLessThan, target, left, right));
}

void
BaseExtension::IfCmpUnsignedLessOrEqual(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedLessOrEqual, "IfCmpUnsignedLessOrEqual"))
            break;
    }

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfCmpUnsignedLessOrEqual(MEM_PASSLOC(mem), this, b, aIfCmpUnsignedLessOrEqual, target, left, right));
}

IfThenElseBuilder
BaseExtension::IfThenElse(LOCATION, Builder *b, Value *selector) {
    #if 0
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedLessOrEqual, "IfCmpUnsignedLessOrEqual"))
            break;
    }
    #endif

    IfThenElseBuilder ifThenElseBuilder;
    ifThenElseBuilder.setSelector(selector);
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_IfThenElse(MEM_PASSLOC(mem), this, b, this->aIfThenElse, &ifThenElseBuilder));
    return ifThenElseBuilder;
}


void
BaseExtension::Switch(LOCATION, Builder *b, SwitchBuilder *bldr) {
    #if 0
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        BaseExtensionChecker *checker = it.item();
        if (checker->validateIfCmp(PASSLOC, b, target, left, right, CompileFail_BadInputTypes_IfCmpUnsignedLessOrEqual, "IfCmpUnsignedLessOrEqual"))
            break;
    }
    #endif

    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_Switch(MEM_PASSLOC(mem), this, b, aSwitch, bldr->selector(), bldr->defaultBuilder(), bldr->casesArray()));
}



//
// Memory operations
//

Value *
BaseExtension::LoadAt(LOCATION, Builder *b, Value *ptrValue) {
    assert(ptrValue->type()->isKind<PointerType>());
    const Type *baseType = ptrValue->type()->refine<PointerType>()->baseType();
    Allocator *mem = b->ir()->mem();
    Value * result = createValue(b, baseType);
    addOperation(b, new (mem) Op_LoadAt(MEM_PASSLOC(mem), this, b, this->aLoadAt, result, ptrValue));
    return result;
}

void
BaseExtension::StoreAt(LOCATION, Builder *b, Value *ptrValue, Value *value) {
    assert(ptrValue->type()->isKind<PointerType>());
    const Type *baseType = ptrValue->type()->refine<PointerType>()->baseType();
    assert(baseType == value->type());
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_StoreAt(MEM_PASSLOC(mem), this, b, this->aStoreAt, ptrValue, value));
}

Value *
BaseExtension::LoadField(LOCATION, Builder *b, const FieldType *fieldType, Value *structValue) {
    assert(structValue->type()->isKind<StructType>());
    assert(fieldType->owningStruct() == structValue->type());
    Allocator *mem = b->ir()->mem();
    Value * result = createValue(b, fieldType->type());
    addOperation(b, new (mem) Op_LoadField(MEM_PASSLOC(mem), this, b, this->aLoadField, result, fieldType, structValue));
    return result;
}

void
BaseExtension::StoreField(LOCATION, Builder *b, const FieldType *fieldType, Value *structValue, Value *value) {
    assert(structValue->type()->isKind<StructType>());
    assert(fieldType->owningStruct() == structValue->type());
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_StoreField(MEM_PASSLOC(mem), this, b, this->aStoreField, fieldType, structValue, value));
}

Value *
BaseExtension::LoadFieldAt(LOCATION, Builder *b, const FieldType *fieldType, Value *pStruct) {
    assert(pStruct->type()->isKind<PointerType>());
    const Type *structType = pStruct->type()->refine<PointerType>()->baseType();
    assert(fieldType->owningStruct() == structType);
    Allocator *mem = b->ir()->mem();
    Value * result = createValue(b, fieldType->type());
    addOperation(b, new (mem) Op_LoadFieldAt(MEM_PASSLOC(mem), this, b, this->aLoadFieldAt, result, fieldType, pStruct));
    return result;
}

void
BaseExtension::StoreFieldAt(LOCATION, Builder *b, const FieldType *fieldType, Value *pStruct, Value *value) {
    assert(pStruct->type()->isKind<PointerType>());
    const Type *structType = pStruct->type()->refine<PointerType>()->baseType();
    assert(fieldType->owningStruct() == structType);
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_StoreFieldAt(MEM_PASSLOC(mem), this, b, this->aStoreFieldAt, fieldType, pStruct, value));
}

Value *
BaseExtension::CreateLocalArray(LOCATION, Builder *b, Literal *numElements, const PointerType *pElementType) {
    assert(numElements->type()->isKind<IntegerType>());
    const Type *elementType = pElementType->baseType();
    Allocator *mem = b->ir()->mem();
    Value * result = createValue(b, pElementType);
    // assert concrete type
    addOperation(b, new (mem) Op_CreateLocalArray(MEM_PASSLOC(mem), this, b, this->aCreateLocalArray, result, numElements, pElementType));
    return result;
}

Value *
BaseExtension::CreateLocalStruct(LOCATION, Builder *b, const PointerType *pStructType) {
    const Type *baseType = pStructType->baseType();
    assert(baseType->isKind<StructType>());
    const StructType *structType = baseType->refine<StructType>();
    Allocator *mem = b->ir()->mem();
    Value * result = createValue(b, pStructType);
    addOperation(b, new (mem) Op_CreateLocalStruct(MEM_PASSLOC(mem), this, b, this->aCreateLocalStruct, result, structType));
    return result;
}

Value *
BaseExtension::IndexAt(LOCATION, Builder *b, Value *base, Value *index) {
    const Type *pElementType = base->type();
    assert(pElementType->isKind<PointerType>());
    Allocator *mem = b->ir()->mem();
    Value *result = createValue(b, pElementType);
    addOperation(b, new (mem) Op_IndexAt(MEM_PASSLOC(mem), this, b, aIndexAt, result, base, index));
    return result;
}

//
// Pseudo operations
//
Value *
BaseExtension::ConstInt8(LOCATION, Builder *b, int8_t v) {
    Literal *lv = this->Int8->literal(PASSLOC, b->ir(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstInt16(LOCATION, Builder *b, int16_t v) {
    Literal *lv = this->Int16->literal(PASSLOC, b->ir(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstInt32(LOCATION, Builder *b, int32_t v) {
    Literal *lv = this->Int32->literal(PASSLOC, b->ir(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstInt64(LOCATION, Builder *b, int64_t v) {
    Literal *lv = this->Int64->literal(PASSLOC, b->ir(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstFloat32(LOCATION, Builder *b, float v) {
    Literal *lv = this->Float32->literal(PASSLOC, b->ir(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstFloat64(LOCATION, Builder *b, double v) {
    Literal *lv = this->Float64->literal(PASSLOC, b->ir(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstAddress(LOCATION, Builder *b, void * v) {
    Literal *lv = this->Address->literal(PASSLOC, b->ir(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::ConstPointer(LOCATION, Builder *b, const PointerType *type, void * v) {
    Literal *lv = type->literal(PASSLOC, b->ir(), v);
    return Const(PASSLOC, b, lv);
}

Value *
BaseExtension::Zero(LOCATION, Builder *b, const Type *type) {
    Literal *zero = type->zero(PASSLOC, b->ir());
    assert(zero);
    return Const(PASSLOC, b, zero);
}

Value *
BaseExtension::One(LOCATION, Builder *b, const Type *type) {
    Literal *one = type->identity(PASSLOC, b->ir());
    assert(one);
    return Const(PASSLOC, b, one);
}

#if 0
void
BaseExtension::Increment(LOCATION, Builder *b, Symbol *sym, Value *bump) {
    Value *oldValue = _fx->Load(PASSLOC, b, sym);
    Value *newValue = Add(PASSLOC, b, oldValue, bump);
    _fx->Store(PASSLOC, b, sym, newValue);
}

void
BaseExtension::Increment(LOCATION, Builder *b, Func::LocalSymbol *sym) {
   Value *oldValue = _fx->Load(PASSLOC, b, sym);
   Value *newValue = Add(PASSLOC, b, oldValue, One(PASSLOC, b, sym->type()));
   _fx->Store(PASSLOC, b, sym, newValue);
}
#endif

void
BaseExtension::failValidateOffsetAt(LOCATION, Builder *b, Value *array) {
    CompilationException e(PASSLOC, _compiler, CompileFail_BadInputArray_OffsetAt);
    const Type *arrayType = array->type();
    e.setMessageLine(String("OffsetAt: invalid array type"))
     .appendMessageLine(String("   array ").append(arrayType->to_string()))
     .appendMessageLine(String("Array type must be a PointerType"));
    throw e;
}

Value *
BaseExtension::OffsetAt(LOCATION, Builder *b, Value *array, size_t elementIndex) {
    const Type *pElement = array->type();
    if (!pElement->isKind<Base::PointerType>())
        failValidateOffsetAt(PASSLOC, b, array);

    const Type *Element = pElement->refine<PointerType>()->baseType();
    size_t offset = elementIndex; // * Element->size()/8;
    Literal *elementOffset = this->Word->literal(PASSLOC, b->ir(), reinterpret_cast<LiteralBytes *>(&offset));
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
    //Value *elementSize = this->Const(PASSLOC, b, this->Word->literal(PASSLOC, b->ir(), reinterpret_cast<LiteralBytes *>(&size)));
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
BaseExtension::PointerTo(LOCATION, IR *ir, const Type *baseType) {
    PointerTypeBuilder pb(this, ir);
    pb.setBaseType(baseType);
    return pb.create(PASSLOC);
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
BuilderBase::Load (String name)
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
BuilderBase::LoadField (String structName, String fieldName, Value * structBase)
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
BuilderBase::LoadIndirect (String structName, String fieldName, Value * pStructBase)
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
BuilderBase::Store (String name, Value * value)
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
BuilderBase::StoreField (String structName, String fieldName, Value * structBase, Value *value)
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
BuilderBase::StoreIndirect (String structName, String fieldName, Value * pStructBase, Value *value)
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
   assert(func->type()->isKind<Func::FunctionType>());

   va_list args ;
   va_start(args, numArgs);
   Func::FunctionType *function = func->type()->refine<Func::FunctionType>();
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
   assert(func->type()->isKind<Func::FunctionType>());
   Func::FunctionType *function = func->type()->refine<Func::FunctionType>();
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
BuilderBase::ForLoopUp(String loopVar, Builder * body, Value * initial, Value * end, Value * bump)
   {
   Func::LocalSymbol *loopSym = NULL;
   Symbol *sym = fb()->getSymbol(loopVar);
   if (sym && sym->isKind<Func::LocalSymbol>())
      loopSym = sym->refine<Func::LocalSymbol>();
   else
      loopSym = fb()->DefineLocal(loopVar, initial->type());
   ForLoopUp(loopSym, body, initial, end, bump);
   }

void
BuilderBase::ForLoopUp(Func::LocalSymbol *loopSym, Builder * body, Value * initial, Value * end, Value * bump)
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
BuilderBase::ForLoop(bool countsUp, String loopVar, Builder * loopBody, Builder * loopContinue, Builder * loopBreak, Value * initial, Value * end, Value * bump)
   {
   Func::LocalSymbol *loopSym = NULL;
   Symbol *sym = fb()->getSymbol(loopVar);
   if (sym && sym->isKind<Func::LocalSymbol>())
      loopSym = sym->refine<Func::LocalSymbol>();
   else
      loopSym = fb()->DefineLocal(loopVar, initial->type());
   ForLoop(countsUp, loopSym, loopBody, loopContinue, loopBreak, initial, end, bump);
   }

void
BuilderBase::ForLoop(bool countsUp, Func::LocalSymbol *loopSym, Builder * loopBody, Builder * loopContinue, Builder * loopBreak, Value * initial, Value * end, Value * bump)
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

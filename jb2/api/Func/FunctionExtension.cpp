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
#include "Func/Function.hpp"
#include "Func/FunctionCompilation.hpp"
#include "Func/FunctionContext.hpp"
#include "Func/FunctionExtension.hpp"
#include "Func/FunctionOperations.hpp"
#include "Func/FunctionScope.hpp"
#include "Func/FunctionSymbols.hpp"
#include "Func/FunctionType.hpp"

namespace OMR {
namespace JitBuilder {
namespace Func {

INIT_JBALLOC_REUSECAT(FunctionExtension, Extension);
SUBCLASS_KINDSERVICE_IMPL(FunctionExtension,"FunctionExtension",Extension,Extensible);

const SemanticVersion FunctionExtension::version(FUNCTIONEXT_MAJOR,FUNCTIONEXT_MINOR,FUNCTIONEXT_PATCH);
const String FunctionExtension::NAME("jb2func");

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        Allocator *mem = compiler->mem();
        return new (mem) FunctionExtension(MEM_PASSLOC(mem), compiler);
    }
}

FunctionExtension::FunctionExtension(MEM_LOCATION(a), Compiler *compiler, bool extended, String extensionName)
    : Extension(MEM_PASSLOC(a), CLASSKIND(FunctionExtension,Extensible), compiler, (extended ? extensionName : NAME))
    , aLoad(registerAction(String(a, "Load")))
    , aStore(registerAction(String(a, "Store")))
    , aCall(registerAction(String(a, "Call")))
    , aCallVoid(registerAction(String(a, "CallVoid")))
    , aReturn(registerAction(String(a, "Return")))
    , aReturnVoid(registerAction(String(a, "ReturnVoid")))
    , CompileFail_MismatchedArgumentTypes_Call(registerReturnCode(String(a, "CompileFail_MismatchedArgumentTypes_Call")))
    , _checkers(NULL, a) {

    if (!extended) {
        registerChecker(new (a) FunctionExtensionChecker(a, this));
    }

    registerForExtensible(CLASSKIND(Func::FunctionCompilation,Extensible), this);
}

FunctionExtension::~FunctionExtension() {
    // what about other types!?
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        FunctionExtensionChecker *checker = it.item();
        delete checker;
    }
    //_checkers.erase();
}

void
FunctionExtension::createAddon(Extensible *e) {
    Allocator *mem = e->allocator();

    if (e->isKind<FunctionCompilation>()) {
        FunctionCompilationAddon *fc = new (mem) FunctionCompilationAddon(mem, this, e->refine<FunctionCompilation>());
        e->attach(fc);
    }
}

void
FunctionExtension::registerChecker(FunctionExtensionChecker *checker) {
    _checkers.push_front(checker);
}


FunctionExtensionChecker::FunctionExtensionChecker(Allocator *a, FunctionExtension *func)
    : Allocatable(a)
    , _func(func) {

}

FunctionExtensionChecker::~FunctionExtensionChecker() {

}


//
// Operations
//

Value *
FunctionExtension::Load(LOCATION, Builder *b, Symbol * sym) {
    Allocator *mem = b->ir()->mem();
    Value * result = createValue(b, sym->type());
    addOperation(b, new (mem) Op_Load(MEM_PASSLOC(mem), this, b, this->aLoad, result, sym));
    return result;
}

void
FunctionExtension::Store(LOCATION, Builder *b, Symbol * sym, Value *value) {
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_Store(MEM_PASSLOC(mem), this, b, this->aStore, sym, value));
}


bool
FunctionExtensionChecker::validateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args) {
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
FunctionExtensionChecker::failValidateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args) {
    const FunctionType *tgtType = target->functionType();
    CompilationException e(PASSLOC, _func->compiler(), _func->CompileFail_MismatchedArgumentTypes_Call);
    Allocator *mem = _func->compiler()->mem();
    e.setMessageLine(String(mem, "Call: mismatched argument types"));
    for (int32_t a=0;a < tgtType->numParms();a++) {
        Value *arg = va_arg(args, Value *);
        if (arg->type() != tgtType->parmTypes()[a])
            e.appendMessageLine(String(mem, "  X  "));
        else
            e.appendMessageLine(String(mem, "     "));
        e.appendMessage(String(mem, " p").append(String::to_string(mem, a)).append(String(mem, " ")).append(tgtType->parmTypes()[a]->to_string(mem))
         .append(String(mem, " : a")).append(String::to_string(mem, a))
         .append(String(mem, " v")).append(String::to_string(mem, arg->id()))
         .append(String(mem, " ")).append(arg->type()->to_string(mem)));
    }
    e.appendMessageLine(String(mem, "Argument types must match corresponding parameter types (currently exact, should be assignable to)"));
    throw e;
}

Value *
FunctionExtension::Call(LOCATION, Builder *b, FunctionSymbol *target, ...) {
    Allocator *mem = b->ir()->mem();

    const FunctionType *tgtType = target->functionType();
    for (auto it = _checkers.iterator(); it.hasItem(); it++) {
        FunctionExtensionChecker *checker = it.item();
        std::va_list args;
        va_start(args, target);
        if (checker->validateCall(PASSLOC, b, target, args))
            break;
        va_end(args);
    }

    std::va_list args;
    va_start(args, target);

    Value *result = NULL;
    if (target->functionType()->returnType() == compiler()->coreExt()->NoType) {
        addOperation(b, new (mem) Op_CallVoid(MEM_PASSLOC(mem), this, b, aCallVoid, target, args));
    } else {
        result = createValue(b, target->functionType()->returnType());
        addOperation(b, new (mem) Op_Call(MEM_PASSLOC(mem), this, b, aCall, result, target, args));
    }
    return result;
}

void
FunctionExtension::Return(LOCATION, Builder *b) {
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_ReturnVoid(MEM_PASSLOC(mem), this, b, this->aReturnVoid));
}

void
FunctionExtension::Return(LOCATION, Builder *b, Value *v) {
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_Return(MEM_PASSLOC(mem), this, b, this->aReturn, v));
}

// DefineFunctionType takes ownership of parmTypes array and must ensure it will always be freed
const FunctionType *
FunctionExtension::DefineFunctionType(LOCATION, FunctionCompilation *comp, const Type *returnType, int32_t numParms, const Type **parmTypes) {
    const FunctionType *fType = comp->addon<FunctionCompilationAddon>()->lookupFunctionType(returnType, numParms, parmTypes);
    if (fType) {
        delete[] parmTypes;
        return fType;
    }

    Allocator *mem = comp->ir()->mem();
    const FunctionType *f = new (mem) FunctionType(MEM_PASSLOC(mem), this, comp->ir()->typedict(), returnType, numParms, parmTypes);
    comp->addon<FunctionCompilationAddon>()->registerFunctionType(f);
    return f;
}

CompiledBody *
FunctionExtension::compile(LOCATION, Function *func, StrategyID strategy, TextLogger *lgr) {

    if (strategy == NoStrategy)
        strategy = _compiler->coreExt()->strategyCodegen;

    Allocator *mem = _compiler->mem();
    FunctionCompilation *comp = new (mem) FunctionCompilation(mem, this, func, strategy);
    comp->setLogger(lgr);

    CompiledBody *body = _compiler->compile(PASSLOC, comp, strategy);
    delete comp;
    return body;
}

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

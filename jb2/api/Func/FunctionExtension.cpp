/******************************************************************************* * Copyright (c) 2021, 2022 IBM Corp. and others *
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
#include "Func/FunctionSymbols.hpp"
#include "Func/FunctionType.hpp"

namespace OMR {
namespace JitBuilder {
namespace Func {

const SemanticVersion FunctionExtension::version(FUNCTIONEXT_MAJOR,FUNCTIONEXT_MINOR,FUNCTIONEXT_PATCH);
const std::string FunctionExtension::NAME("jb2func");

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        return new FunctionExtension(PASSLOC, compiler);
    }
}

FunctionExtension::FunctionExtension(LOCATION, Compiler *compiler, bool extended, std::string extensionName)
    : Extension(PASSLOC, compiler, (extended ? extensionName : NAME))
    , aLoad(registerAction(std::string("Load")))
    , aStore(registerAction(std::string("Store")))
    , aCall(registerAction(std::string("Call")))
    , aCallVoid(registerAction(std::string("CallVoid")))
    , aReturn(registerAction(std::string("Return")))
    , aReturnVoid(registerAction(std::string("ReturnVoid")))
    , CompileFail_MismatchedArgumentTypes_Call(registerReturnCode("CompileFail_MismatchedArgumentTypes_Call")) {

    if (!extended) {
        registerChecker(new FunctionExtensionChecker(this));
    }
}

FunctionExtension::~FunctionExtension() {
    // what about other types!?
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        FunctionExtensionChecker *checker = *it;
        delete checker;
    }
    _checkers.erase(_checkers.begin());
}

void
FunctionExtension::registerChecker(FunctionExtensionChecker *checker) {
    _checkers.push_front(checker);
}


//
// Operations
//

Value *
FunctionExtension::Load(LOCATION, Builder *b, Symbol * sym) {
    Value * result = createValue(b, sym->type());
    addOperation(b, new Op_Load(PASSLOC, this, b, this->aLoad, result, sym));
    return result;
}

void
FunctionExtension::Store(LOCATION, Builder *b, Symbol * sym, Value *value) {
    addOperation(b, new Op_Store(PASSLOC, this, b, this->aStore, sym, value));
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
FunctionExtension::Call(LOCATION, Builder *b, FunctionSymbol *target, ...) {
    const FunctionType *tgtType = target->functionType();
    for (auto it = _checkers.begin(); it != _checkers.end(); it++) {
        FunctionExtensionChecker *checker = *it;
        std::va_list args;
        va_start(args, target);
        if (checker->validateCall(PASSLOC, b, target, args))
            break;
        va_end(args);
    }

    std::va_list args;
    va_start(args, target);

    Value *result = NULL;
    if (target->functionType()->returnType() == NoType) {
        addOperation(b, new Op_CallVoid(PASSLOC, this, b, aCall, target, args));
    } else {
        result = createValue(b, target->functionType()->returnType());
        addOperation(b, new Op_Call(PASSLOC, this, b, aCall, result, target, args));
    }
    return result;
}

void
FunctionExtension::Return(LOCATION, Builder *b) {
    addOperation(b, new Op_ReturnVoid(PASSLOC, this, b, this->aReturnVoid));
}

void
FunctionExtension::Return(LOCATION, Builder *b, Value *v) {
    addOperation(b, new Op_Return(PASSLOC, this, b, this->aReturn, v));
}

const FunctionType *
FunctionExtension::DefineFunctionType(LOCATION, FunctionCompilation *comp, const Type *returnType, int32_t numParms, const Type **parmTypes) {
    const FunctionType *fType = comp->lookupFunctionType(returnType, numParms, parmTypes);
    if (fType)
        return fType;

    const FunctionType *f = new FunctionType(PASSLOC, this, comp->typedict(), returnType, numParms, parmTypes);
    comp->registerFunctionType(f);
    return f;
}

CompilerReturnCode
FunctionExtension::compile(LOCATION, Function *func, StrategyID strategy, TextWriter *logger) {

    if (strategy == NoStrategy)
        strategy = _compiler->jb1cgStrategyID;

    FunctionCompilation comp(_compiler, func, strategy, _compiler->dict(), NULL);
    FunctionContext context(PASSLOC, &comp);
    comp.setContext(&context);
    comp.setLogger(logger);

    CompilerReturnCode rc = _compiler->compile(PASSLOC, &comp, strategy);
    if (rc != _compiler->CompileSuccessful) {
        return rc;
    }

    CompiledBody *body = new CompiledBody(func, &context, strategy);
    func->saveCompiledBody(body, strategy);

    return _compiler->CompileSuccessful;
}

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

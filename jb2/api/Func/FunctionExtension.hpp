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

#ifndef FUNCTIONEXTENSION_INCL
#define FUNCTIONEXTENSION_INCL

#include <cstdarg>
#include <map>
#include <stdint.h>
#include <vector>
#include "CreateLoc.hpp"
#include "Extension.hpp"
#include "IDs.hpp"
#include "SemanticVersion.hpp"
#include "typedefs.hpp"


namespace OMR {
namespace JitBuilder {

class Builder;
class Compilation;
class Location;
class Symbol;
class Type;
class Value;

namespace Func {

class Function;
class FunctionCompilation;
class FunctionExtensionChecker;
class FunctionSymbol;
class FunctionType;
class LocalSymbol;
class ParameterSymbol;

class FunctionExtension : public Extension {

public:
    FunctionExtension(LOCATION, Compiler *compiler, bool extended=false, std::string extensionName="");
    virtual ~FunctionExtension();

    static const std::string NAME;

    // 3 == LocalSymbol, ParameterSymbol, FunctionSymbol
    uint32_t numSymbolTypes() const { return 3; }

    virtual const SemanticVersion * semver() const {
        return &version;
    }

    //
    // Types
    //

    #if 0
    const FunctionType * DefineFunctionType(LOCATION, FunctionTypeBuilder *builder);
    #endif
    // deprecated
    const FunctionType * DefineFunctionType(LOCATION, FunctionCompilation *comp, const Type *returnType, int32_t numParms, const Type **parmTypes);

    //
    // Actions
    //

    // Control actions
    const ActionID aCall;
    const ActionID aCallVoid;
    const ActionID aReturn;
    const ActionID aReturnVoid;

    // Memory actions
    const ActionID aLoad;
    const ActionID aStore;


    //
    // CompilerReturnCodes
    //

    const CompilerReturnCode CompileFail_MismatchedArgumentTypes_Call;


    //
    // Operations
    //

    Value *Call(LOCATION, Builder *b, FunctionSymbol *funcSym, ...);
    Value *CallWithArgArray(LOCATION, Builder *b, FunctionSymbol *funcSym, int32_t numArgs, Value **args);
    void Return(LOCATION, Builder *b);
    void Return(LOCATION, Builder *b, Value *v);
    Value * Load(LOCATION, Builder *b, Symbol *sym);
    void Store(LOCATION, Builder *b, Symbol *sym, Value *value);

    void registerChecker(FunctionExtensionChecker *checker);

    CompilerReturnCode compile(LOCATION, Function *func, StrategyID strategy, TextWriter *logger);

protected:

    std::list<FunctionExtensionChecker *> _checkers;

    static const MajorID FUNCTIONEXT_MAJOR=0;
    static const MinorID FUNCTIONEXT_MINOR=1;
    static const PatchID FUNCTIONEXT_PATCH=0;
    static const SemanticVersion version;
};

class FunctionExtensionChecker {
public:
    FunctionExtensionChecker(FunctionExtension *func)
        : _func(func) {

    }

    virtual bool validateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args);
    //virtual bool validateCallWithArgArray(LOCATION, Builder *b, FunctionSymbol *target, int32_t numArgs, Value **args);

protected:
    virtual void failValidateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args);
    //virtual void failValidateCallWithArgArray(LOCATION, Builder *b, FunctionSymbol *target, int32_t numArgs, Value **args);

    FunctionExtension *_func;
};

typedef std::vector<FunctionSymbol *> FunctionSymbolVector;
typedef Iterator<FunctionSymbol>  FunctionSymbolIterator;

typedef std::vector<LocalSymbol *> LocalSymbolVector;
typedef Iterator<LocalSymbol> LocalSymbolIterator;

typedef std::vector<ParameterSymbol *> ParameterSymbolVector;
typedef Iterator<ParameterSymbol> ParameterSymbolIterator;

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

#endif // defined(FUNCTIONEXTENSION_INCL)
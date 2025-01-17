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

#ifndef FUNCTIONEXTENSION_INCL
#define FUNCTIONEXTENSION_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

class Function;
class FunctionCompilation;
class FunctionExtensionChecker;
class FunctionSymbol;
class FunctionType;
class FunctionTypeBuilder;
class LocalSymbol;
class ParameterSymbol;

class FunctionExtension : public Extension {
    JBALLOC_(FunctionExtension)

public:
    DYNAMIC_ALLOC_ONLY(FunctionExtension, LOCATION, Compiler *compiler, bool extended=false, String extensionName="");

    static const String NAME;

    // 3 == LocalSymbol, ParameterSymbol, FunctionSymbol
    uint32_t numSymbolTypes() const { return 3; }

    virtual const SemanticVersion * semver() const {
        return &version;
    }

    //
    // Types
    //
    const FunctionType * DefineFunctionType(LOCATION, FunctionCompilation *comp, FunctionTypeBuilder & ftb);

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

    void createAddon(Extensible *e);
    void registerChecker(FunctionExtensionChecker *checker);
    CompiledBody * compile(LOCATION, Function *func, StrategyID strategy, TextLogger *lgr);

protected:

    List<FunctionExtensionChecker *> _checkers;

    static const MajorID FUNCTIONEXT_MAJOR=0;
    static const MinorID FUNCTIONEXT_MINOR=1;
    static const PatchID FUNCTIONEXT_PATCH=0;
    static const SemanticVersion version;

    SUBCLASS_KINDSERVICE_DECL(Extensible, FunctionExtension);
};

class FunctionExtensionChecker : public Allocatable {
    JBALLOC(FunctionExtensionChecker, NoAllocationCategory)

public:
    DYNAMIC_ALLOC_ONLY(FunctionExtensionChecker, FunctionExtension *func);

    virtual bool validateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args);
    //virtual bool validateCallWithArgArray(LOCATION, Builder *b, FunctionSymbol *target, int32_t numArgs, Value **args);

protected:
    virtual void failValidateCall(LOCATION, Builder *b, FunctionSymbol *target, std::va_list & args);
    //virtual void failValidateCallWithArgArray(LOCATION, Builder *b, FunctionSymbol *target, int32_t numArgs, Value **args);

    FunctionExtension *_func;

    SUBCLASS_KINDSERVICE_DECL(Extensible, FunctionExtension);
};

typedef List<FunctionSymbol *> FunctionSymbolList;
typedef FunctionSymbolList::Iterator FunctionSymbolIterator;

typedef List<LocalSymbol *> LocalSymbolList;
typedef LocalSymbolList::Iterator LocalSymbolIterator;

typedef List<ParameterSymbol *> ParameterSymbolList;
typedef ParameterSymbolList::Iterator ParameterSymbolIterator;

} // namespace Func
} // namespace JB2
} // namespace OMR

#endif // defined(FUNCTIONEXTENSION_INCL)

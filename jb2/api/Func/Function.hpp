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

#ifndef FUNCTION_INCL
#define FUNCTION_INCL

#include <exception>
#include <string>
#include <vector>
#include "JBCore.hpp"

namespace OMR {
namespace JitBuilder {
namespace Func {

class Debugger;
class FunctionCompilation;
class FunctionContext;
class FunctionSymbol;
class LocalSymbol;
class ParameterSymbol;

class Function : public CompileUnit {
    friend class FunctionCompilation;
    friend class FunctionExtension;

public:
    virtual ~Function();

    virtual std::string kindName() const { return "Function"; }


    void DefineName(std::string name);
    void DefineFile(std::string file);
    void DefineLine(std::string line);
#if 0
    ParameterSymbol * DefineParameter(std::string name, const Type * type);
    void DefineReturnType(const Type * type);
    LocalSymbol * DefineLocal(std::string name, const Type * type);
    FunctionSymbol * DefineFunction(LOCATION, std::string name, std::string fileName, std::string lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, ...);
    FunctionSymbol * DefineFunction(LOCATION, std::string name, std::string fileName, std::string lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, const Type **parmTypes);
    const PointerType * PointerTo(LOCATION, const Type *baseType);
#endif

    std::string name() const { return _givenName; }
    std::string fileName() const { return _fileName; }
    std::string lineNumber() const { return _lineNumber; }

#if 0
    ParameterSymbolIterator ParametersBegin() const;
    ParameterSymbolIterator ParametersEnd() const;
    ParameterSymbolVector ResetParameters();

    LocalSymbolIterator LocalsBegin() const;
    LocalSymbolIterator LocalsEnd() const;
    LocalSymbolVector ResetLocals();
    LocalSymbol * LookupLocal(std::string name);

    FunctionSymbolIterator FunctionsBegin() const { return FunctionSymbolIterator(_functions); }
    FunctionSymbolIterator FunctionsEnd() const { return endFunctionIterator; }
    FunctionSymbolVector ResetFunctions();
    FunctionSymbol *LookupFunction(std::string name) {
        Symbol *sym = getSymbol(name);
        if (sym == NULL || !sym->isKind<FunctionSymbol>())
            return NULL;
        return sym->refine<FunctionSymbol>();
    }

    int32_t numValues() const;
    int32_t numLocals() const;
    int32_t numReturnValues() const;
    const Type * returnType(unsigned i=0) const;

    void constructJB1Function(JB1MethodBuilder *j1mb);
    void jbgenProlog(JB1MethodBuilder *j1mb);

    Symbol * getSymbol(std::string name);
    void addLocation(Location *loc ) { _locations.push_back(loc); }
#endif

protected:
    Function(LOCATION, Compiler *compiler); // meant to be subclassed
    Function(LOCATION, Function *outerFunction);

    static FunctionCompilation *fcomp(Compilation *comp);
    static FunctionContext *fcontext(Compilation *comp);

    virtual bool initContext(LOCATION, Compilation *comp, Context *context);
    virtual bool buildIL(LOCATION, Compilation *comp, Context *context);

    // Next two are the API that drives user sub classes of Function
    virtual bool initContext(LOCATION, FunctionCompilation *comp, FunctionContext *fc) { return true; }
    virtual bool buildIL(LOCATION, FunctionCompilation *comp, FunctionContext *fc) { return true; }

    #if 0
    void DefineParameter(ParameterSymbol *parm);
    void DefineLocal(LocalSymbol *local);
    void DefineFunction(FunctionSymbol *function);
    FunctionSymbol * internalDefineFunction(LOCATION, std::string name, std::string fileName, std::string lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, const Type **parmTypes);
    #endif

    Function              * _outerFunction;

    std::string             _givenName;
    std::string             _fileName;
    std::string             _lineNumber;

    #if 0
    FunctionSymbolVector    _functions; // move to compiler?
    Debugger              * _debuggerObject;
    static FunctionSymbolIterator endFunctionIterator;
    #endif
};

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

#endif // defined(FUNCTION_INCL)

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

#ifndef FUNCTIONCONTEXT_INCL
#define FUNCTIONCONTEXT_INCL

#include <string>
#include <vector>
#include "Context.hpp"
#include "Func/FunctionExtension.hpp"

namespace OMR {
namespace JitBuilder {

class Operation;
class Symbol;
class TextWriter;
class Type;
class TypeDictionary;

namespace Func {

class LocalSymbol;
class FunctionCompilation;
class FunctionSymbol;
class ParameterSymbol;

class FunctionContext : public Context {
    friend class FunctionCompilation;
    friend class FunctionExtension;

public:
    FunctionContext(LOCATION, FunctionCompilation *comp, String name="");
    FunctionContext(LOCATION, FunctionCompilation *comp, FunctionContext *caller, String name="");

    ParameterSymbol * DefineParameter(String name, const Type * type);
    LocalSymbol * DefineLocal(String name, const Type * type);
    FunctionSymbol * DefineFunction(LOCATION, String name, String fileName, String lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, ...);
    FunctionSymbol * DefineFunction(LOCATION, String name, String fileName, String lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, const Type **parmTypes);
    void DefineReturnType(const Type * type) {
        _returnTypes.push_back(type);
    }

    LocalSymbolIterator LocalsBegin() const { return LocalSymbolIterator(this->_locals); }
    LocalSymbolIterator LocalsEnd() const { return endLocalSymbolIterator; }
    LocalSymbolVector ResetLocals() {
        LocalSymbolVector prev = this->_locals;
        this->_locals.clear();
        return prev;
    }
    LocalSymbol * LookupLocal(String name);

    ParameterSymbolIterator ParametersBegin() const { return ParameterSymbolIterator(this->_parameters); }
    ParameterSymbolIterator ParametersEnd() const { return endParameterSymbolIterator; }
    ParameterSymbolVector ResetParameters() {
        ParameterSymbolVector prev = this->_parameters;
        this->_parameters.clear();
        return prev;
    }

    FunctionSymbolIterator FunctionsBegin() const { return FunctionSymbolIterator(_functions); }
    FunctionSymbolIterator FunctionsEnd() const { return endFunctionSymbolIterator; }
    FunctionSymbolVector ResetFunctions();
    FunctionSymbol *LookupFunction(String name);
    Symbol * getSymbol(String name);

    int32_t numReturnTypes() const { return _returnTypes.size(); }
    const Type * returnType(int i=0) const {
        if (_returnTypes.size() > i)
            return this->_returnTypes[i];
        return NULL;
    }

protected:
    FunctionCompilation *fComp() const;

    void DefineParameter(ParameterSymbol *parm);
    void DefineLocal(LocalSymbol *local);
    void DefineFunction(FunctionSymbol *function);
    FunctionSymbol * internalDefineFunction(LOCATION, String name, String fileName, String lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, const Type **parmTypes);

    ParameterSymbolVector _parameters;
    LocalSymbolVector _locals;
    FunctionSymbolVector _functions;

    std::vector<const Type *> _returnTypes;

    static LocalSymbolIterator endLocalSymbolIterator;
    static ParameterSymbolIterator endParameterSymbolIterator;
    static FunctionSymbolIterator endFunctionSymbolIterator;
};

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

#endif // defined(FUNCTIONCONTEXT_INCL)

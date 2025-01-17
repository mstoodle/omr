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

#include "JBCore.hpp"
#include "Func/FunctionExtension.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

class LocalSymbol;
class FunctionCompilation;
class FunctionSymbol;
class ParameterSymbol;

class FunctionContext : public Context {
    JBALLOC_(FunctionContext)

    friend class FunctionCompilation;
    friend class FunctionExtension;
public:
    DYNAMIC_ALLOC_ONLY(FunctionContext, Extension *fx, IR *ir, String name="");
    DYNAMIC_ALLOC_ONLY(FunctionContext, FunctionContext *caller, String name="");

    ParameterSymbol * DefineParameter(String name, const Type * type);
    LocalSymbol * DefineLocal(String name, const Type * type);
    FunctionSymbol * DefineFunction(LOCATION, FunctionCompilation *comp, String name, String fileName, String lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, ...);
    FunctionSymbol * DefineFunction(LOCATION, FunctionCompilation *comp, String name, String fileName, String lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, const Type **parmTypes);
    void DefineReturnType(const Type * type);

    LocalSymbolIterator locals() const { return this->_locals.iterator(); }
    LocalSymbolList resetLocals();
    LocalSymbol * LookupLocal(String name);

    size_t numParameters() const { return this->_parameters.length(); }
    ParameterSymbolIterator parameters() const { return this->_parameters.iterator(); }
    ParameterSymbolList resetParameters();

    FunctionSymbolIterator functions() const { return this->_functions.iterator(); }
    FunctionSymbolList resetFunctions();
    FunctionSymbol *LookupFunction(String name);

    Symbol * getSymbol(String name);

    int32_t numReturnTypes() const { return _returnTypes.length(); }
    const Type * returnType(uint32_t i=0) const {
        if (_returnTypes.length() > i)
            return this->_returnTypes[i];
        return NULL;
    }

protected:
    DYNAMIC_ALLOC_ONLY(FunctionContext, Extension *ext, KINDTYPE(Extensible) kind, IR *ir, String name="");
    DYNAMIC_ALLOC_ONLY(FunctionContext, KINDTYPE(Extensible) kind, FunctionContext *caller, String name="");
    DYNAMIC_ALLOC_ONLY(FunctionContext, const FunctionContext *source, IRCloner *cloner);

    virtual Context *clone(Allocator *mem, IRCloner *cloner) const;
    virtual void logContents(TextLogger & lgr) const;

    FunctionCompilation *fComp() const;

    void DefineParameter(ParameterSymbol *parm);
    void DefineLocal(LocalSymbol *local);

    void DefineFunction(FunctionSymbol *function);
    FunctionSymbol * internalDefineFunction(LOCATION, FunctionCompilation *comp, String name, String fileName, String lineNumber, void *entryPoint, const Type *returnType, int32_t numParms, const Type **parmTypes);

    ParameterSymbolList _parameters;
    LocalSymbolList _locals;
    FunctionSymbolList _functions;

    TypeArray _returnTypes;

    SUBCLASS_KINDSERVICE_DECL(Extensible, FunctionContext);
};

} // namespace Func
} // namespace JB2
} // namespace OMR

#endif // defined(FUNCTIONCONTEXT_INCL)

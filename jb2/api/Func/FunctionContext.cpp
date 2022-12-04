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

#include "JBCore.hpp"
#include "Func/FunctionCompilation.hpp"
#include "Func/FunctionContext.hpp"
#include "Func/FunctionExtension.hpp"
#include "Func/FunctionSymbols.hpp"
#include "Func/FunctionType.hpp"


namespace OMR {
namespace JitBuilder {
namespace Func {

LocalSymbolIterator FunctionContext::endLocalSymbolIterator;
ParameterSymbolIterator FunctionContext::endParameterSymbolIterator;
FunctionSymbolIterator FunctionContext::endFunctionSymbolIterator;

FunctionContext::FunctionContext(LOCATION, FunctionCompilation *comp, std::string name)
    : Context(PASSLOC, comp, NULL, NULL, NULL, 1, 1, name) {

}

FunctionContext::FunctionContext(LOCATION, FunctionCompilation *comp, FunctionContext *caller, std::string name)
    : Context(PASSLOC, caller, NULL, NULL, NULL, 1, 1, name) {

}

FunctionCompilation *
FunctionContext::fComp() const {
    return static_cast<FunctionCompilation *>(_comp);
}

ParameterSymbol *
FunctionContext::DefineParameter(std::string name, const Type * type) {
    ParameterSymbol *parm = new ParameterSymbol(name, type, this->_parameters.size());
    this->_parameters.push_back(parm);
    addSymbol(parm);
    return parm;
}

void
FunctionContext::DefineParameter(ParameterSymbol *parm) {
    assert(parm->index() == this->_parameters.size());
    this->_parameters.push_back(parm);
    addSymbol(parm);
}

LocalSymbol *
FunctionContext::DefineLocal(std::string name, const Type * type) {
    Symbol *sym = this->lookupSymbol(name);
    if (sym && sym->isKind<LocalSymbol>())
       return sym->refine<LocalSymbol>();

    LocalSymbol *local = new LocalSymbol(name, type);
    this->_locals.push_back(local);
    addSymbol(local);
    return local;
}

void
FunctionContext::DefineLocal(LocalSymbol *local) {
    this->_locals.push_back(local);
    addSymbol(local);
}

LocalSymbol *
FunctionContext::LookupLocal(std::string name) {
    for (LocalSymbolIterator lIt = LocalsBegin(); lIt != LocalsEnd(); lIt++) {
        LocalSymbol * local = *lIt;
        if (local->name() == name)
            return local;
    }

    for (ParameterSymbolIterator pIt = ParametersBegin(); pIt != ParametersEnd(); pIt++) {
        ParameterSymbol * parameter = *pIt;
        if (parameter->name() == name)
            return parameter;
    }

    return NULL;
}

FunctionSymbol *
FunctionContext::DefineFunction(LOCATION,
                                std::string name,
                                std::string fileName,
                                std::string lineNumber,
                                void *entryPoint,
                                const Type *returnType,
                                int32_t numParms,
                                ...) {

    const Type **parmTypes = new const Type*[numParms];
    va_list parms;
    va_start(parms, numParms);
   
    for (int32_t p=0;p < numParms;p++)
        parmTypes[p] = (const Type *) va_arg(parms, const Type *);
    va_end(parms);

    return internalDefineFunction(PASSLOC, name, fileName, lineNumber, entryPoint, returnType, numParms, parmTypes);
}

FunctionSymbol *
FunctionContext::DefineFunction(LOCATION,
                                std::string name,
                                std::string fileName,
                                std::string lineNumber,
                                void *entryPoint,
                                const Type *returnType,
                                int32_t numParms,
                                const Type **parmTypes) {

    // copy parameter types so don't have to force caller to keep the parmTypes array alive
    const Type **copiedParmTypes = new const Type*[numParms];
    for (int32_t p=0;p < numParms;p++)
        copiedParmTypes[p] = parmTypes[p];

    return internalDefineFunction(PASSLOC, name, fileName, lineNumber, entryPoint, returnType, numParms, copiedParmTypes);
}

void
FunctionContext::DefineFunction(FunctionSymbol *function) {
    _functions.push_back(function);
}

// maybe move to Compilation?
FunctionSymbol *
FunctionContext::internalDefineFunction(LOCATION,
                                        std::string name,
                                        std::string fileName,
                                        std::string lineNumber,
                                        void *entryPoint,
                                        const Type *returnType,
                                        int32_t numParms,
                                        const Type **parmTypes) {

    FunctionExtension *fx = _comp->compiler()->lookupExtension<FunctionExtension>();
    const FunctionType *type = fx->DefineFunctionType(PASSLOC, fComp(), returnType, numParms, parmTypes);
    FunctionSymbol *sym = new FunctionSymbol(type, name, fileName, lineNumber, entryPoint);
    _functions.push_back(sym);
    return sym;
}

FunctionSymbol *
FunctionContext::LookupFunction(std::string name) {
    Symbol *sym = getSymbol(name);
    if (sym == NULL || !sym->isKind<FunctionSymbol>())
        return NULL;
    return sym->refine<FunctionSymbol>();
}

FunctionSymbolVector
FunctionContext::ResetFunctions() {
    FunctionSymbolVector prev = _functions;
    _functions.clear();
    return prev;
}

Symbol *
FunctionContext::getSymbol(std::string name) {
    LocalSymbol *localSym = LookupLocal(name);
    if (localSym)
        return localSym;

    for (FunctionSymbolIterator fIt = FunctionsBegin(); fIt != FunctionsEnd(); fIt++) {
        FunctionSymbol * function = *fIt;
        if (function->name() == name)
            return function;
    }

    return NULL;
}

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

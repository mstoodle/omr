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

INIT_JBALLOC_REUSECAT(FunctionContext, Context)
SUBCLASS_KINDSERVICE_IMPL(FunctionContext, "FunctionContext", Context, Context)

FunctionContext::FunctionContext(LOCATION, FunctionCompilation *comp, String name)
    : Context(PASSLOC, KIND(Context), comp->ext(), comp, name)
    , _parameters(NULL, comp->mem())
    , _locals(NULL, comp->mem())
    , _functions(NULL, comp->mem())
    , _returnTypes(NULL, comp->mem()) {

}

FunctionContext::FunctionContext(LOCATION, KINDTYPE(Context) kind, FunctionCompilation *comp, String name)
    : Context(PASSLOC, kind, comp->ext(), comp, name)
    , _parameters(NULL, comp->mem())
    , _locals(NULL, comp->mem())
    , _functions(NULL, comp->mem())
    , _returnTypes(NULL, comp->mem()) {

}

FunctionContext::FunctionContext(LOCATION, FunctionContext *caller, String name)
    : Context(PASSLOC, getContextClassKind(), caller->comp()->ext(), caller, name)
    , _parameters(NULL, caller->comp()->mem())
    , _locals(NULL, caller->comp()->mem())
    , _functions(NULL, caller->comp()->mem())
    , _returnTypes(NULL, caller->comp()->mem()) {

}

FunctionContext::FunctionContext(LOCATION, KINDTYPE(Context) kind, FunctionContext *caller, String name)
    : Context(PASSLOC, kind, caller->comp()->ext(), caller, name)
    , _parameters(NULL, caller->comp()->mem())
    , _locals(NULL, caller->comp()->mem())
    , _functions(NULL, caller->comp()->mem())
    , _returnTypes(NULL, caller->comp()->mem()) {

}

FunctionContext::~FunctionContext() {
    _functions.erase();
    _locals.erase();
    _parameters.erase();

    // FunctionContext doesn't create Type objects, but still need to erase the List
    _returnTypes.erase();
}

FunctionCompilation *
FunctionContext::fComp() const {
    return _comp->refine<FunctionCompilation>();
}

ParameterSymbol *
FunctionContext::DefineParameter(String name, const Type * type) {
    Allocator *mem = _comp->mem();
    ParameterSymbol *parm = new (mem) ParameterSymbol(mem, _ext, name, type, this->_parameters.length());
    this->_parameters.push_back(parm);
    addSymbol(parm);
    return parm;
}

void
FunctionContext::DefineParameter(ParameterSymbol *parm) {
    assert(parm->index() == this->_parameters.length());
    this->_parameters.push_back(parm);
    addSymbol(parm);
}

LocalSymbol *
FunctionContext::DefineLocal(String name, const Type * type) {
    Symbol *sym = this->lookupSymbol(name);
    if (sym && sym->isKind<LocalSymbol>())
       return sym->refine<LocalSymbol>();

    Allocator *mem = _comp->mem();
    LocalSymbol *local = new (mem) LocalSymbol(mem, _ext, name, type);
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
FunctionContext::LookupLocal(String name) {
    for (auto it = locals(); it.hasItem(); it++) {
        LocalSymbol * local = it.item();
        if (local->name() == name)
            return local;
    }

    for (auto it = parameters(); it.hasItem(); it++) {
        ParameterSymbol * parameter = it.item();
        if (parameter->name() == name)
            return parameter;
    }

    return NULL;
}

void
FunctionContext::DefineReturnType(const Type * type) {
    size_t index = _returnTypes.length();
    _returnTypes.assign(index, type);
}

LocalSymbolList
FunctionContext::resetLocals() {
    LocalSymbolList prev = this->_locals;
    this->_locals.erase();
    return prev;
}

ParameterSymbolList
FunctionContext::resetParameters() {
    ParameterSymbolList prev = this->_parameters;
    this->_parameters.erase();
    return prev;
}

FunctionSymbolList
FunctionContext::resetFunctions() {
    FunctionSymbolList prev = _functions;
    _functions.erase();
    return prev;
}

FunctionSymbol *
FunctionContext::DefineFunction(LOCATION,
                                String name,
                                String fileName,
                                String lineNumber,
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
                                String name,
                                String fileName,
                                String lineNumber,
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
                                        String name,
                                        String fileName,
                                        String lineNumber,
                                        void *entryPoint,
                                        const Type *returnType,
                                        int32_t numParms,
                                        const Type **parmTypes) {

    FunctionExtension *fx = _comp->compiler()->lookupExtension<FunctionExtension>();
    const FunctionType *type = fx->DefineFunctionType(PASSLOC, fComp(), returnType, numParms, parmTypes);
    Allocator *mem = _comp->mem();
    FunctionSymbol *sym = new (mem) FunctionSymbol(mem, _ext, type, name, fileName, lineNumber, entryPoint);
    _functions.push_back(sym);
    addSymbol(sym);
    return sym;
}

FunctionSymbol *
FunctionContext::LookupFunction(String name) {
    Symbol *sym = getSymbol(name);
    if (sym == NULL || !sym->isKind<FunctionSymbol>())
        return NULL;
    return sym->refine<FunctionSymbol>();
}

Symbol *
FunctionContext::getSymbol(String name) {
    LocalSymbol *localSym = LookupLocal(name);
    if (localSym)
        return localSym;

    for (auto fIt = functions(); fIt.hasItem(); fIt++) {
        FunctionSymbol * function = fIt.item();
        if (function->name() == name)
            return function;
    }

    return NULL;
}

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

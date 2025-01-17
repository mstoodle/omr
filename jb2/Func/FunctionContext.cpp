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
namespace JB2 {
namespace Func {

INIT_JBALLOC_REUSECAT(FunctionContext, Context)
SUBCLASS_KINDSERVICE_IMPL(FunctionContext, "FunctionContext", Context, Extensible)

FunctionContext::FunctionContext(Allocator *a, Extension *ext, IR *ir, String name)
    : Context(a, ext, KIND(Extensible), ir, name)
    , _parameters(NULL, ir->mem())
    , _locals(NULL, ir->mem())
    , _functions(NULL, ir->mem())
    , _returnTypes(NULL, ir->mem()) {

}

FunctionContext::FunctionContext(Allocator *a, Extension *ext, KINDTYPE(Extensible) kind, IR *ir, String name)
    : Context(a, ext, kind, ir, name)
    , _parameters(NULL, ir->mem())
    , _locals(NULL, ir->mem())
    , _functions(NULL, ir->mem())
    , _returnTypes(NULL, ir->mem()) {

}

FunctionContext::FunctionContext(Allocator *a, FunctionContext *caller, String name)
    : Context(a, caller->ext(), KIND(Extensible), caller, name)
    , _parameters(NULL, caller->ir()->mem())
    , _locals(NULL, caller->ir()->mem())
    , _functions(NULL, caller->ir()->mem())
    , _returnTypes(NULL, caller->ir()->mem()) {

}

FunctionContext::FunctionContext(Allocator *a, KINDTYPE(Extensible) kind, FunctionContext *caller, String name)
    : Context(a, caller->ext(), kind, caller, name)
    , _parameters(NULL, caller->ir()->mem())
    , _locals(NULL, caller->ir()->mem())
    , _functions(NULL, caller->ir()->mem())
    , _returnTypes(NULL, caller->ir()->mem()) {

}

FunctionContext::FunctionContext(Allocator *a, const FunctionContext *source, IRCloner *cloner)
    : Context(a, source, cloner)
    , _parameters(NULL, a)
    , _locals(NULL, a)
    , _functions(NULL, a)
    , _returnTypes(NULL, a) {

    for (auto it=source->_parameters.iterator();it.hasItem(); it++) {
        Symbol *sym = it.item();
        _parameters.push_back(cloner->clonedSymbol(sym)->refine<ParameterSymbol>());
    }
    for (auto it=source->_locals.iterator();it.hasItem(); it++) {
        Symbol *sym = it.item();
        _locals.push_back(cloner->clonedSymbol(sym)->refine<LocalSymbol>());
    }
    for (auto it=source->_functions.iterator();it.hasItem(); it++) {
        Symbol *sym = it.item();
        _functions.push_back(cloner->clonedSymbol(sym)->refine<FunctionSymbol>());
    }
    for (auto i=0;i < source->_returnTypes.length();i++) {
        const Type *type = source->_returnTypes[i];
        _returnTypes.assign(i, cloner->clonedType(type));
    }
}

FunctionContext::~FunctionContext() {
    _functions.erase();
    _locals.erase();
    _parameters.erase();

    // FunctionContext doesn't create Type objects, but still need to erase the List
    _returnTypes.erase();
}

Context *
FunctionContext::clone(Allocator *mem, IRCloner *cloner) const {
    return new (mem) FunctionContext(mem, this, cloner);
}

ParameterSymbol *
FunctionContext::DefineParameter(String name, const Type * type) {
    Allocator *mem = _ir->mem();
    ParameterSymbol *parm = new (mem) ParameterSymbol(mem, ext(), _ir, name, type, this->_parameters.length());
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

    Allocator *mem = _ir->mem();
    LocalSymbol *local = new (mem) LocalSymbol(mem, ext(), _ir, name, type);
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
                                FunctionCompilation *comp,
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

    return internalDefineFunction(PASSLOC, comp, name, fileName, lineNumber, entryPoint, returnType, numParms, parmTypes);
}

FunctionSymbol *
FunctionContext::DefineFunction(LOCATION,
                                FunctionCompilation *comp,
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

    return internalDefineFunction(PASSLOC, comp, name, fileName, lineNumber, entryPoint, returnType, numParms, copiedParmTypes);
}

void
FunctionContext::DefineFunction(FunctionSymbol *function) {
    _functions.push_back(function);
}

// maybe move to Compilation?
FunctionSymbol *
FunctionContext::internalDefineFunction(LOCATION,
                                        FunctionCompilation *comp,
                                        String name,
                                        String fileName,
                                        String lineNumber,
                                        void *entryPoint,
                                        const Type *returnType,
                                        int32_t numParms,
                                        const Type **parmTypes) {

    FunctionExtension *fx = ext()->refine<FunctionExtension>();
    FunctionTypeBuilder ftb(comp);
    ftb.setReturnType(returnType);
    for (auto p=0;p < numParms;p++)
        ftb.addParameterType(parmTypes[p]);
    const FunctionType *type = fx->DefineFunctionType(PASSLOC, comp, ftb);
    Allocator *mem = _ir->mem();
    FunctionSymbol *sym = new (mem) FunctionSymbol(mem, ext(), _ir, type, name, fileName, lineNumber, entryPoint);
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

void
FunctionContext::logContents(TextLogger & lgr) const {
    lgr.irListBegin("locals", _locals.length());
    for (auto it=_locals.iterator(); it.hasItem(); it++) {
        const LocalSymbol *sym = it.item();
        sym->log(lgr, true);
    }
    lgr.irListEnd(_locals.length());
    lgr.irListBegin("parameters", _parameters.length());
    for (auto it=_parameters.iterator(); it.hasItem(); it++) {
        const ParameterSymbol *sym = it.item();
        sym->log(lgr, true);
    }
    lgr.irListEnd(_parameters.length());
    lgr.irListBegin("returnTypes", _returnTypes.length());
    for (auto it=_returnTypes.constIterator(); it.hasItem(); it++) {
        const Type *rt = it.item();
        rt->log(lgr, true);
    }
    lgr.irListEnd(_returnTypes.length());
}

} // namespace Func
} // namespace JB2
} // namespace OMR

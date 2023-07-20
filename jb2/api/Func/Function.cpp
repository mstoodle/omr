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
#include "Func/FunctionExtension.hpp"
#include "Func/Function.hpp"
#include "Func/FunctionCompilation.hpp"
#include "Func/FunctionContext.hpp"
#include "Func/FunctionScope.hpp"


namespace OMR {
namespace JitBuilder {
namespace Func {


Function::Function(MEM_LOCATION(a), Compiler *compiler)
    : CompileUnit(MEM_PASSLOC(a), compiler)
    , _cx(compiler->coreExt()) {

}

Function::Function(LOCATION, Compiler *compiler)
    : CompileUnit(PASSLOC, compiler) 
    , _cx(compiler->coreExt()){

}

Function::Function(MEM_LOCATION(a), Function *outerFunc)
    : CompileUnit(MEM_PASSLOC(a), outerFunc) 
    , _cx(_compiler->coreExt()){

}

Function::Function(LOCATION, Function *outerFunc)
    : CompileUnit(PASSLOC, outerFunc) 
    , _cx(_compiler->coreExt()){

}

Function::~Function() {
}

bool
Function::buildContext(LOCATION, Compilation *comp, Scope *scope, Context *ctx) {
    FunctionCompilation *fcomp = comp->refine<FunctionCompilation>();
    FunctionScope *fscope = scope->refine<FunctionScope>();
    FunctionContext *fctx = ctx->refine<FunctionContext>();
    return buildContext(PASSLOC, fcomp, fscope, fctx);
}

bool
Function::buildIL(LOCATION, Compilation *comp, Scope *scope, Context *ctx) {
    Builder *entryBuilder = comp->unit()->EntryBuilder(PASSLOC, comp, scope);
    BuilderEntry *entry = new (comp->mem()) BuilderEntry(comp->mem(), comp, 0, entryBuilder);
    scope->addEntryPoint(entry);

    FunctionCompilation *fcomp = comp->refine<FunctionCompilation>();
    FunctionScope *fscope = scope->refine<FunctionScope>();
    FunctionContext *fctx = ctx->refine<FunctionContext>();
    return buildIL(PASSLOC, fcomp, fscope, fctx);
}

FunctionCompilation *
Function::fcomp(Compilation *comp) {
    return comp->refine<FunctionCompilation>();
}

FunctionScope *
Function::fscope(Compilation *comp) {
    return comp->scope<FunctionScope>();
}

FunctionContext *
Function::fctx(Compilation *comp) {
    return comp->context<FunctionContext>();
}

void
Function::DefineName(String name) {
    _givenName = name;
}

void
Function::DefineFile(String file) {
    _createLocation.overrideFileName(file.c_str());
}

void
Function::DefineLine(String line) {
    _createLocation.overrideLineNumber(static_cast<uint32_t>(std::stoul(line)));
}

#if 0
int32_t
Function::numLocals() const {
    return comp()->nativeContext()->_locals.size() + _nativeContext->_parameters.size();
}

ParameterSymbol *
Function::DefineParameter(String name, const Type * type) {
    return comp()->nativeContext()->DefineParameter(name, type);
}

void
Function::DefineParameter(ParameterSymbol * parm) {
    comp()->nativeContext()->DefineParameter(parm);
}

void
Function::DefineReturnType(const Type * type) {
    comp()->nativeContext()->DefineReturnType(type);
}

LocalSymbol *
Function::DefineLocal(String name, const Type * type) {
    return comp()->nativeContext()->DefineLocal(name, type);
}

void
Function::DefineLocal(LocalSymbol *local) {
    comp()->nativeContext()->DefineLocal(local);
}

FunctionSymbol *
Function::DefineFunction(LOCATION,
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
Function::DefineFunction(LOCATION,
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
Function::DefineFunction(FunctionSymbol *function) {
    _functions.push_back(function);
}

// maybe move to Compilation?
FunctionSymbol *
Function::internalDefineFunction(LOCATION,
                                 String name,
                                 String fileName,
                                 String lineNumber,
                                 void *entryPoint,
                                 const Type *returnType,
                                 int32_t numParms,
                                 const Type **parmTypes) {

    const FunctionType *type = _base->DefineFunctionType(PASSLOC, _comp, returnType, numParms, parmTypes);
    FunctionSymbol *sym = new FunctionSymbol(type, name, fileName, lineNumber, entryPoint);
    _functions.push_back(sym);
    return sym;
}

const PointerType *
Function::PointerTo(LOCATION, const Type *baseType) {
    return this->_base->PointerTo(PASSLOC, _comp, baseType);
}

LocalSymbolIterator
Function::LocalsBegin() const {
    return comp()->nativeContext()->LocalsBegin();
}

LocalSymbolIterator
Function::LocalsEnd() const {
    return comp()->nativeContext()->LocalsEnd();
}

ParameterSymbolIterator
Function::ParametersBegin() const {
    return comp()->nativeContext()->ParametersBegin();
}

ParameterSymbolIterator
Function::ParametersEnd() const {
    return comp()->nativeContext()->ParametersEnd();
}

ParameterSymbolList
Function::ResetParameters() {
    return comp()->nativeContext()->ResetParameters();
}

LocalSymbolList
Function::ResetLocals() {
    return comp()->nativeContext()->ResetLocals();
}

LocalSymbol *
Function::LookupLocal(String name) {
    for (LocalSymbolIterator lIt = comp()->nativeContext()->LocalsBegin(); lIt != comp()->nativeContext()->LocalsEnd(); lIt++) {
        LocalSymbol * local = *lIt;
        if (local->name() == name)
            return local;
    }

    for (ParameterSymbolIterator pIt = comp()->nativeContext()->ParametersBegin(); pIt != comp()->nativeContext()->ParametersEnd(); pIt++) {
        ParameterSymbol * parameter = *pIt;
        if (parameter->name() == name)
            return parameter;
    }

    return NULL;
}

FunctionSymbolList
Function::ResetFunctions() {
    FunctionSymbolList prev = _functions;
    _functions.clear();
    return prev;
}

Symbol *
Function::getSymbol(String name) {
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

const Type *
Function::returnType() const {
    return comp()->nativeContext()->returnType();
}

int32_t
Function::numReturnValues() const {
    return (comp()->nativeContext()->returnType() == _base->NoType) ? 0 : 1; /* 1 for now */
}

void
Function::addInitialBuildersToWorklist(BuilderWorklist & worklist) {
    for (int i=0;i < this->_numEntryPoints;i++)
       worklist.push_back(this->_entryPoints[i]);
}

#endif

#if 0
void *
Function::internalDebugger(int32_t *returnCode)
   {
   _debuggerObject = new OMR::JitBuilder::Debugger(this);
   _debugEntryPoint = _debuggerObject->createDebugger(returnCode);
   return _debugEntryPoint;
   }
#endif

} // namespace Function
} // namespace JitBuilder
} // namespace OMR


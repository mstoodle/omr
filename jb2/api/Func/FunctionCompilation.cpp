/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
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

#include <assert.h>
#include <list>
#include <stdint.h>
#include <string>
#include "JBCore.hpp"
#include "Func/Function.hpp"
#include "Func/FunctionCompilation.hpp"
#include "Func/FunctionContext.hpp"
#include "Func/FunctionSymbols.hpp"
#include "Func/FunctionType.hpp"

namespace OMR {
namespace JitBuilder {
namespace Func {

FunctionCompilation::FunctionCompilation(Compiler *compiler, Function *func, StrategyID strategy, TypeDictionary *dict, Config *localConfig)
    : Compilation(compiler, func, strategy, dict, localConfig) {

}

Function *
FunctionCompilation::func() const {
    return static_cast<Function *>(_unit);
}

FunctionContext *
FunctionCompilation::funcContext() const {
    return static_cast<FunctionContext *>(_context);
}

void
FunctionCompilation::addInitialBuildersToWorklist(BuilderWorklist & worklist) {
    #if 0
    func()->addInitialBuildersToWorklist(worklist);
    #endif
    for (int i=0;i < context()->numEntryPoints();i++)
       worklist.push_back(funcContext()->builderEntryPoint(i));
}

const FunctionType *
FunctionCompilation::lookupFunctionType(const Type *returnType, int32_t numParms, const Type **parmTypes) {
    String name = FunctionType::typeName(returnType, numParms, parmTypes);
    auto it = _functionTypesFromName.find(name);
    if (it != _functionTypesFromName.end()) {
        const FunctionType *fType = it->second;
        return fType;
    }
    return NULL;
}

void
FunctionCompilation::registerFunctionType(const FunctionType *fType) {
    _functionTypesFromName.insert({fType->name(), fType});
}

void
FunctionCompilation::write(TextWriter &w) const {
    w << "Function" << w.endl();

    TypeDictionary *td = typedict();
    td->write(w);

    SymbolDictionary *sd = symdict();
    sd->write(w);

    LiteralDictionary *ld = litdict();
    ld->write(w);

    w.indent() << "[ Function" /*<< _id*/ << w.endl();
    w.indentIn();

    FunctionContext *fc = funcContext();
    w.indent() << "[ name " << func()->name() << " ]" << w.endl();
    w.indent() << "[ origin " << func()->createLoc()->to_string() << " ]" << w.endl();
    w.indent() << "[ returnType " << fc->returnType() << "]" << w.endl();
    for (ParameterSymbolIterator paramIt = fc->ParametersBegin();paramIt != fc->ParametersEnd(); paramIt++) {
        const ParameterSymbol *parameter = *paramIt;
        w.indent() << "[ parameter " << parameter << " ]" << w.endl();
    }
    for (LocalSymbolIterator localIt = fc->LocalsBegin();localIt != fc->LocalsEnd();localIt++) {
        const LocalSymbol *local = *localIt;
        w.indent() << "[ local " << local << " ]" << w.endl();
    }
    for (FunctionSymbolIterator functionIt = fc->FunctionsBegin();functionIt != fc->FunctionsEnd();functionIt++) {
        const FunctionSymbol *function = *functionIt;
        w.indent() << "[ function " << function << " ]" << w.endl();
    }
    w.indent() << "[ entryPoint " << fc->builderEntryPoint() << " ]" << w.endl();
    w.indentOut();
    w.indent() << "]" << w.endl();
}

void
FunctionCompilation::constructJB1Function(JB1MethodBuilder *j1mb) {
    j1mb->FunctionName(func()->name());
    j1mb->FunctionFile(func()->fileName());
    j1mb->FunctionLine(func()->lineNumber());
    j1mb->FunctionReturnType(funcContext()->returnType());

    for (ParameterSymbolIterator paramIt = funcContext()->ParametersBegin();paramIt != funcContext()->ParametersEnd(); paramIt++) {
        const ParameterSymbol *parameter = *paramIt;
        j1mb->Parameter(parameter->name(), parameter->type());
    }
    for (LocalSymbolIterator localIt = funcContext()->LocalsBegin();localIt != funcContext()->LocalsEnd();localIt++) {
        const LocalSymbol *symbol = *localIt;
        j1mb->Local(symbol->name(), symbol->type());
    }
    for (FunctionSymbolIterator fnIt = funcContext()->FunctionsBegin();fnIt != funcContext()->FunctionsEnd();fnIt++) {
        const FunctionSymbol *fSym = *fnIt;
        const FunctionType *fType = fSym->functionType();
        j1mb->DefineFunction(fSym->name(),
                             fSym->fileName(),
                             fSym->lineNumber(),
                             fSym->entryPoint(),
                             fType->returnType(),
                             fType->numParms(),
                             fType->parmTypes());
     }
}

void
FunctionCompilation::jbgenProlog(JB1MethodBuilder *j1mb) {
    j1mb->EntryPoint(context()->builderEntryPoint());
}

void
FunctionCompilation::setNativeEntryPoint(void *entry, unsigned i) {
    context()->setNativeEntryPoint(entry, i);
}

void
FunctionCompilation::replaceTypes(TypeReplacer *repl) {
    FunctionContext *fc = funcContext();
    TextWriter *log = logger(repl->traceEnabled());

    // replace return type if needed
    const Type *returnType = fc->returnType();
    const Type *newReturnType = repl->singleMappedType(returnType);
    if (newReturnType != returnType) {
        fc->DefineReturnType(newReturnType);
        if (log) log->indent() << "Return type t" << returnType->id() << " -> t" << newReturnType->id() << log->endl();
    }

    // replace parameters if needed, creating new Symbols if needed
    bool changeSomeParm = false;
    for (auto pIt = fc->ParametersBegin(); pIt != fc->ParametersEnd(); pIt++) {
        ParameterSymbol *parm = *pIt;
        if (repl->isModified(parm->type())) {
            changeSomeParm = true;
            break;
        }
    }

    if (changeSomeParm) {
        ParameterSymbolVector prevParameters = fc->ResetParameters();
        int32_t parmIndex = 0;
        for (auto pIt = prevParameters.begin(); pIt != prevParameters.end(); pIt++) {
            ParameterSymbol *parm = *pIt;
            const Type *type = parm->type();
            SymbolMapper *parmSymMapper = new SymbolMapper();
            if (repl->isModified(type)) {
                TypeMapper *parmTypeMapper = repl->mapperForType(type);
                String baseName("");
                if (parmTypeMapper->size() > 1)
                    baseName = parm->name() + ".";

                LOG_INDENT_REGION(log) {
                    for (int i=0;i < parmTypeMapper->size();i++) {
                        String newName(baseName + parmTypeMapper->name());
                        const Type *newType = parmTypeMapper->next();
                        ParameterSymbol *newSym = fc->DefineParameter(newName, newType);
                        parmIndex++;
                        parmSymMapper->add(newSym);
                        repl->recordSymbolMapper(newSym, new SymbolMapper(newSym));
                        if (log) log->indent() << "now DefineParameter " << newName << " (" << newType->name() << " t" << newType->id() << ")" << log->endl();
                    }
                } LOG_OUTDENT
            }
            else if (parmIndex > parm->index()) {
                // no type change but recreate because parameter index needs to change due to early parameter expansion
                ParameterSymbol *newSym = fc->DefineParameter(parm->name(), parm->type());
                parmSymMapper->add(newSym);
                parmIndex++;
            }
            else {
                // no change in parameter at all, so reuse existing parameter symbol
                fc->DefineParameter(parm);
                parmSymMapper->add(parm);
                parmIndex++;
            }
            repl->recordSymbolMapper(parm, parmSymMapper);
        }
    }

    // replace locals if needed, creating new Symbols if needed
    bool changeSomeLocal = false;
    for (auto lIt = fc->LocalsBegin(); lIt != fc->LocalsEnd(); lIt++) {
        Symbol *local = *lIt;
        if (repl->isModified(local->type())) {
            changeSomeLocal = true;
            break;
        }
    }

    if (changeSomeLocal) {
        LocalSymbolVector locals = fc->ResetLocals();
        for (auto lIt = locals.begin(); lIt != locals.end(); lIt++) {
            LocalSymbol *local = *lIt;
            const Type *type = local->type();
            if (log) log->indent() << "Local " << local->name() << " (" << type->name() << " t" << type->id() << "):" << log->endl();

            SymbolMapper *symMapper = new SymbolMapper();
            if (repl->isModified(type)) {
                TypeMapper *typeMapper = repl->mapperForType(type);
                String baseName("");
                if (typeMapper->size() > 1)
                    baseName = local->name() + ".";

                LOG_INDENT_REGION(log) {
                    for (int i=0;i < typeMapper->size();i++) {
                        String newName = baseName + typeMapper->name();
                        const Type *newType = typeMapper->next();
                        Symbol *newSym = fc->DefineLocal(newName, newType);
                        symMapper->add(newSym);
                        repl->recordSymbolMapper(newSym, new SymbolMapper(newSym));
                        if (log) log->indent() << "now DefineLocal " << newName << " (" << newType->name() << " t" << newType->id() << ")" << log->endl();
                    }
                } LOG_OUTDENT
            }
            else  {
                // no type change so can reuse existing local symbol
                fc->DefineLocal(local);
                symMapper->add(local);
            }
            repl->recordSymbolMapper(local, symMapper);
        }
    }

    // replace functions if needed, creating new Symbols if needed
    bool changeSomeFunction = false;
    for (auto fnIt = fc->FunctionsBegin(); fnIt != fc->FunctionsEnd(); fnIt++) {
        FunctionSymbol *function = *fnIt;
        if (repl->isModified(function->functionType())) {
            changeSomeFunction = true;
            break;
        }
    }
 
    if (changeSomeFunction) {
        FunctionSymbolVector functions = fc->ResetFunctions();
        for (auto fnIt = functions.begin(); fnIt != functions.end(); fnIt++) {
            FunctionSymbol *function = *fnIt;
            const FunctionType *type = function->functionType();
            if (log) log->indent() << "Function " << function->name() << " (" << type->name() << " t" << type->id() << "):" << log->endl();

            SymbolMapper *symMapper = new SymbolMapper();
            if (repl->isModified(type)) {
                TypeMapper *typeMapper = repl->mapperForType(type);
                assert(typeMapper->size() == 1); // shouldn't be multiple FunctionTypes

                const Type *newType = typeMapper->next();
                assert(newType->isKind<FunctionType>() && newType != type);
                const FunctionType *newFnType = newType->refine<FunctionType>();
                FunctionSymbol *newSym = fc->DefineFunction(LOC,
                                                            function->name(), // maybe not right
                                                            function->fileName(), // not quite right
                                                            function->lineNumber(), // not quite right
                                                            function->entryPoint(), // unlikely to be right
                                                            newFnType->returnType(),
                                                            newFnType->numParms(),
                                                            newFnType->parmTypes());
                repl->recordSymbolMapper(newSym, new SymbolMapper(newSym));
                symMapper->add(newSym);
                LOG_INDENT_REGION(log) {
                    if (log) log->indent() << "now DefineFunction " << function->name() << " (" << newType->name() << " t" << newType->id() << ")" << log->endl();
                } LOG_OUTDENT
            }
            else {
                // type not modified, so just reuse existing symbol
                fc->DefineFunction(function);
                symMapper->add(function);
            }
            repl->recordSymbolMapper(function, symMapper);
        }
    }
}

} // namespace Func
} // namespace JitBuilder
} // namespace OMR

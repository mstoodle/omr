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
#include <stdint.h>
#include "JBCore.hpp"
#include "Func/Function.hpp"
#include "Func/FunctionCompilation.hpp"
#include "Func/FunctionContext.hpp"
#include "Func/FunctionSymbols.hpp"
#include "Func/FunctionType.hpp"

namespace OMR {
namespace JitBuilder {
namespace Func {

FunctionCompilation::FunctionCompilation(Compiler *compiler, Function *func, StrategyID strategy, LiteralDictionary *litdict, SymbolDictionary *symdict, TypeDictionary *typedict, Config *localConfig)
    : Compilation(compiler, func, strategy, litdict, symdict, typedict, localConfig) {

}
FunctionCompilation::~FunctionCompilation() {

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
FunctionCompilation::addInitialBuildersToWorklist(BuilderList & worklist) {
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
FunctionCompilation::log(TextLogger &lgr) const {
    lgr << "Function" << lgr.endl();

    TypeDictionary *td = typedict();
    td->log(lgr);

    SymbolDictionary *sd = symdict();
    sd->log(lgr);

    LiteralDictionary *ld = litdict();
    ld->log(lgr);

    lgr.indent() << "[ Function" /*<< _id*/ << lgr.endl();
    lgr.indentIn();

    FunctionContext *fc = funcContext();
    lgr.indent() << "[ name " << func()->name() << " ]" << lgr.endl();
    lgr.indent() << "[ origin " << func()->createLoc()->to_string() << " ]" << lgr.endl();
    lgr.indent() << "[ returnType " << fc->returnType() << "]" << lgr.endl();
    for (auto paramIt = fc->parameters();paramIt.hasItem(); paramIt++) {
        const ParameterSymbol *parameter = paramIt.item();
        lgr.indent() << "[ parameter " << parameter << " ]" << lgr.endl();
    }
    for (auto localIt = fc->locals();localIt.hasItem();localIt++) {
        const LocalSymbol *local = localIt.item();
        lgr.indent() << "[ local " << local << " ]" << lgr.endl();
    }
    for (auto functionIt = fc->functions();functionIt.hasItem();functionIt++) {
        const FunctionSymbol *function = functionIt.item();
        lgr.indent() << "[ function " << function << " ]" << lgr.endl();
    }
    lgr.indent() << "[ entryPoint " << fc->builderEntryPoint() << " ]" << lgr.endl();
    lgr.indentOut();
    lgr.indent() << "]" << lgr.endl();
}

void
FunctionCompilation::constructJB1Function(JB1MethodBuilder *j1mb) {
    j1mb->FunctionName(func()->name());
    j1mb->FunctionFile(func()->fileName());
    j1mb->FunctionLine(func()->lineNumber());
    j1mb->FunctionReturnType(funcContext()->returnType());

    for (auto paramIt = funcContext()->parameters();paramIt.hasItem(); paramIt++) {
        const ParameterSymbol *parameter = paramIt.item();
        j1mb->Parameter(parameter->name(), parameter->type());
    }
    for (auto localIt = funcContext()->locals();localIt.hasItem();localIt++) {
        const LocalSymbol *symbol = localIt.item();
        j1mb->Local(symbol->name(), symbol->type());
    }
    for (auto fnIt = funcContext()->functions();fnIt.hasItem();fnIt++) {
        const FunctionSymbol *fSym = fnIt.item();
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
    TextLogger *lgr = logger(repl->traceEnabled());

    // replace return type if needed
    const Type *returnType = fc->returnType();
    const Type *newReturnType = repl->singleMappedType(returnType);
    if (newReturnType != returnType) {
        fc->DefineReturnType(newReturnType);
        if (lgr) lgr->indent() << "Return type t" << returnType->id() << " -> t" << newReturnType->id() << lgr->endl();
    }

    // replace parameters if needed, creating new Symbols if needed
    bool changeSomeParm = false;
    for (auto pIt = fc->parameters(); pIt.hasItem(); pIt++) {
        ParameterSymbol *parm = pIt.item();
        if (repl->isModified(parm->type())) {
            changeSomeParm = true;
            break;
        }
    }

    if (changeSomeParm) {
        ParameterSymbolList prevParameters = fc->resetParameters();
        int32_t parmIndex = 0;
        for (auto pIt = prevParameters.iterator(); pIt.hasItem(); pIt++) {
            ParameterSymbol *parm = pIt.item();
            const Type *type = parm->type();
            SymbolMapper *parmSymMapper = new (mem()) SymbolMapper(mem());
            if (repl->isModified(type)) {
                TypeMapper *parmTypeMapper = repl->mapperForType(type);
                String baseName("");
                if (parmTypeMapper->size() > 1)
                    baseName = parm->name() + ".";

                LOG_INDENT_REGION(lgr) {
                    for (int i=0;i < parmTypeMapper->size();i++) {
                        String newName(baseName + parmTypeMapper->name());
                        const Type *newType = parmTypeMapper->next();
                        ParameterSymbol *newSym = fc->DefineParameter(newName, newType);
                        parmIndex++;
                        parmSymMapper->add(newSym);
                        repl->recordSymbolMapper(newSym, new (mem()) SymbolMapper(mem(), newSym));
                        if (lgr) lgr->indent() << "now DefineParameter " << newName << " (" << newType->name() << " t" << newType->id() << ")" << lgr->endl();
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
    for (auto lIt = fc->locals(); lIt.hasItem(); lIt++) {
        Symbol *local = lIt.item();
        if (repl->isModified(local->type())) {
            changeSomeLocal = true;
            break;
        }
    }

    if (changeSomeLocal) {
        LocalSymbolList locals = fc->resetLocals();
        for (auto lIt = locals.iterator(); lIt.hasItem(); lIt++) {
            LocalSymbol *local = lIt.item();
            const Type *type = local->type();
            if (lgr) lgr->indent() << "Local " << local->name() << " (" << type->name() << " t" << type->id() << "):" << lgr->endl();

            SymbolMapper *symMapper = new (mem()) SymbolMapper(mem());
            if (repl->isModified(type)) {
                TypeMapper *typeMapper = repl->mapperForType(type);
                String baseName("");
                if (typeMapper->size() > 1)
                    baseName = local->name() + ".";

                LOG_INDENT_REGION(lgr) {
                    for (int i=0;i < typeMapper->size();i++) {
                        String newName = baseName + typeMapper->name();
                        const Type *newType = typeMapper->next();
                        Symbol *newSym = fc->DefineLocal(newName, newType);
                        symMapper->add(newSym);
                        repl->recordSymbolMapper(newSym, new (mem()) SymbolMapper(mem(), newSym));
                        if (lgr) lgr->indent() << "now DefineLocal " << newName << " (" << newType->name() << " t" << newType->id() << ")" << lgr->endl();
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
    for (auto fnIt = fc->functions(); fnIt.hasItem(); fnIt++) {
        FunctionSymbol *function = fnIt.item();
        if (repl->isModified(function->functionType())) {
            changeSomeFunction = true;
            break;
        }
    }
 
    if (changeSomeFunction) {
        FunctionSymbolList functions = fc->resetFunctions();
        for (auto fnIt = functions.iterator(); fnIt.hasItem(); fnIt++) {
            FunctionSymbol *function = fnIt.item();
            const FunctionType *type = function->functionType();
            if (lgr) lgr->indent() << "Function " << function->name() << " (" << type->name() << " t" << type->id() << "):" << lgr->endl();

            SymbolMapper *symMapper = new (mem()) SymbolMapper(mem());
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
                repl->recordSymbolMapper(newSym, new (mem()) SymbolMapper(mem(), newSym));
                symMapper->add(newSym);
                LOG_INDENT_REGION(lgr) {
                    if (lgr) lgr->indent() << "now DefineFunction " << function->name() << " (" << newType->name() << " t" << newType->id() << ")" << lgr->endl();
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

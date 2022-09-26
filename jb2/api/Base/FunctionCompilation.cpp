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

#include <list>
#include <stdint.h>
#include <string>
#include "Function.hpp"
#include "FunctionCompilation.hpp"
#include "JB1MethodBuilder.hpp"
#include "LiteralDictionary.hpp"
#include "SymbolDictionary.hpp"
#include "TextWriter.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

void
FunctionCompilation::addInitialBuildersToWorklist(BuilderWorklist & worklist) {
    this->_func->addInitialBuildersToWorklist(worklist);
}

const PointerType *
FunctionCompilation::pointerTypeFromBaseType(const Type *baseType) {
    auto found = _pointerTypeFromBaseType.find(baseType);
    if (found != _pointerTypeFromBaseType.end()) {
        const PointerType *t = found->second;
        return t;
    }
    return NULL;
}

void
FunctionCompilation::registerPointerType(const PointerType *pType) {
    const Type *baseType = pType->baseType();
    _pointerTypeFromBaseType.insert({baseType, pType});
}

const StructType *
FunctionCompilation::structTypeFromName(std::string name) {
    auto found = _structTypeFromName.find(name);
    if (found != _structTypeFromName.end()) {
        const StructType *t = found->second;
        return t;
    }
    return NULL;
}

void
FunctionCompilation::registerStructType(const StructType *sType) {
    _structTypeFromName.insert({sType->name(), sType});
}

const FunctionType *
FunctionCompilation::lookupFunctionType(const Type *returnType, int32_t numParms, const Type **parmTypes) {
    std::string name = FunctionType::typeName(returnType, numParms, parmTypes);
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

    TypeDictionary *td = dict();
    td->write(w);

    SymbolDictionary *sd = symdict();
    sd->write(w);

    LiteralDictionary *ld = litdict();
    ld->write(w);

    _func->write(w);
}

void
FunctionCompilation::constructJB1Function(JB1MethodBuilder *j1mb) {
    _func->constructJB1Function(j1mb);
}

void
FunctionCompilation::jbgenProlog(JB1MethodBuilder *j1mb) {
    _func->jbgenProlog(j1mb);
}

void
FunctionCompilation::setNativeEntryPoint(void *entry, int i) {
    _func->setNativeEntryPoint(entry, i);
}

bool
FunctionCompilation::buildIL() {
    bool success = _func->buildIL();
    if (success)
        _ilBuilt = true;
    return success;
}

void
FunctionCompilation::replaceTypes(TypeReplacer *repl) {
    _func->replaceTypes(repl);
}

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

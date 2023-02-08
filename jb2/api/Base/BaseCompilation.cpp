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

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/BaseCompilation.hpp"
#include "Base/BaseTypes.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

INIT_JBALLOC_REUSECAT(BaseCompilation, Compilation)

BaseCompilation::BaseCompilation(Compiler *compiler, Func::Function *func, StrategyID strategy, LiteralDictionary *litDict, SymbolDictionary *symDict, TypeDictionary *typeDict, Config *localConfig)
    : Func::FunctionCompilation(compiler, func, strategy, litDict, symDict, typeDict, localConfig) {

}

const PointerType *
BaseCompilation::pointerTypeFromBaseType(const Type *baseType) {
    auto found = _pointerTypeFromBaseType.find(baseType);
    if (found != _pointerTypeFromBaseType.end()) {
        const PointerType *t = found->second;
        return t;
    }
    return NULL;
}

void
BaseCompilation::registerPointerType(const PointerType *pType) {
    const Type *baseType = pType->baseType();
    _pointerTypeFromBaseType.insert({baseType, pType});
}

const StructType *
BaseCompilation::structTypeFromName(String name) {
    auto found = _structTypeFromName.find(name);
    if (found != _structTypeFromName.end()) {
        const StructType *t = found->second;
        return t;
    }
    return NULL;
}

void
BaseCompilation::registerStructType(const StructType *sType) {
    _structTypeFromName.insert({sType->name(), sType});
}

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

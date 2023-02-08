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

#ifndef BASECOMPILATION_INCL
#define BASECOMPILATION_INCL

#include <map>
#include "JBCore.hpp"
#include "Func/Func.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

class PointerType;
class StructType;

class BaseCompilation : public Func::FunctionCompilation {
    JBALLOC_NO_DESTRUCTOR_(BaseCompilation)

    friend class BaseExtension;

public:
    BaseCompilation(Compiler *compiler, Func::Function *func, StrategyID strategy=NoStrategy, LiteralDictionary *litDict=NULL, SymbolDictionary *symDict=NULL, TypeDictionary *typeDict=NULL, Config *localConfig=NULL);

    const PointerType * pointerTypeFromBaseType(const Type * baseType);
    void registerPointerType(const PointerType * pType);
    const StructType * structTypeFromName(String name);
    void registerStructType(const StructType * sType);

protected:
    void setContext(Func::FunctionContext *context) { this->Compilation::setContext(context); }

    std::map<const Type *,const PointerType *> _pointerTypeFromBaseType;
    std::map<String,const StructType *> _structTypeFromName;
};

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // !defined(BASECOMPILATION_INCL)


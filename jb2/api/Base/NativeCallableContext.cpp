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

#include "BaseExtension.hpp"
#include "BaseSymbols.hpp"
#include "BaseTypes.hpp"
#include "FunctionCompilation.hpp"
#include "NativeCallableContext.hpp"
#include "Operation.hpp"
#include "TextWriter.hpp"


namespace OMR {
namespace JitBuilder {
namespace Base {

LocalSymbolIterator NativeCallableContext::endLocalSymbolIterator;
ParameterSymbolIterator NativeCallableContext::endParameterSymbolIterator;

ParameterSymbol *
NativeCallableContext::DefineParameter(std::string name, const Type * type) {
    ParameterSymbol *parm = new ParameterSymbol(name, type, this->_parameters.size());
    this->_parameters.push_back(parm);
    addSymbol(parm);
    return parm;
}

void
NativeCallableContext::DefineParameter(ParameterSymbol *parm) {
    assert(parm->index() == this->_parameters.size());
    this->_parameters.push_back(parm);
    addSymbol(parm);
}

LocalSymbol *
NativeCallableContext::DefineLocal(std::string name, const Type * type) {
    Symbol *sym = this->lookupSymbol(name);
    if (sym && sym->isKind<LocalSymbol>())
       return sym->refine<LocalSymbol>();

    LocalSymbol *local = new LocalSymbol(name, type);
    this->_locals.push_back(local);
    addSymbol(local);
    return local;
}

void
NativeCallableContext::DefineLocal(LocalSymbol *local) {
    this->_locals.push_back(local);
    addSymbol(local);
}

} // namespace Base
} // namespace JitBuilder
} // namespace OMR


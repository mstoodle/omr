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

#include "Compilation.hpp"
#include "Context.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"


namespace OMR {
namespace JitBuilder {


SymbolIterator Context::symbolEndIterator;

Context::Context(Compilation *comp, Context *parent, std::string name)
    : _comp(comp)
    , _name(name)
    , _parent(parent) {

}

void
Context::addSymbol(Symbol *symbol) {
    this->_comp->symdict()->registerSymbol(symbol);
    this->_symbolByName.insert({symbol->name(), symbol});
    this->_symbols.push_back(symbol);
}

Symbol *
Context::lookupSymbol(std::string name, bool includeParents) {
    auto it = this->_symbolByName.find(name);
    if (it != this->_symbolByName.end())
        return it->second;
    if (includeParents && this->_parent)
        return this->_parent->lookupSymbol(name, includeParents);
    return NULL;
}

} // namespace JitBuilder
} // namespace OMR


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

#include <assert.h>
#include "IRCloner.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(Symbol, SymbolDictionary)

BASECLASS_KINDSERVICE_IMPL(Symbol);

// only used by clone()
Symbol::Symbol(Allocator *mem, const Symbol *source, IRCloner *cloner)
    : Allocatable(mem)
    , _ext(source->_ext)
    , _id(NoSymbol)
    , _name(source->_name)
    , _type(cloner->clonedType(source->_type))
    , BASECLASS_KINDINIT(KIND(Symbol)) {

}

Symbol *
Symbol::clone(Allocator *mem, IRCloner *cloner) {
    assert(_kind == KIND(Symbol));
    return new (mem) Symbol(mem, this, cloner);
}

Symbol::~Symbol() {

}

void
Symbol::log(TextLogger &lgr) const {
    lgr << "[ " << this << " ";
    logDetails(lgr);
    lgr << " ]" << lgr.endl();
}

} // namespace JitBuilder
} // namespace OMR

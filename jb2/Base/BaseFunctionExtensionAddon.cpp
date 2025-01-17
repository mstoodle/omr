/******************************************************************************* * Copyright (c) 2021, 2022 IBM Corp. and others
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

#include <cstdarg>

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/BaseExtension.hpp"
#include "Base/BaseFunctionExtensionAddon.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

INIT_JBALLOC_REUSECAT(BaseFunctionExtensionAddon, Extension)
SUBCLASS_KINDSERVICE_IMPL(BaseFunctionExtensionAddon,"BaseFunctionExtensionAddon",BaseAddon,Extensible);

BaseFunctionExtensionAddon::BaseFunctionExtensionAddon(Allocator *a, Func::FunctionExtension *fx, Base::BaseExtension *bx)
    : BaseAddon(a, bx, this, KIND(Extensible))
    , _fx(fx) {

}

void
BaseFunctionExtensionAddon::Increment(LOCATION, Builder *b, Symbol *sym, Value *bump) {
    Value *oldValue = fx()->Load(PASSLOC, b, sym);
    Value *newValue = bx()->Add(PASSLOC, b, oldValue, bump);
    fx()->Store(PASSLOC, b, sym, newValue);
}

void
BaseFunctionExtensionAddon::Increment(LOCATION, Builder *b, Symbol *sym) {
   Value *oldValue = fx()->Load(PASSLOC, b, sym);
   Value *newValue = bx()->Add(PASSLOC, b, oldValue, bx()->One(PASSLOC, b, sym->type()));
   fx()->Store(PASSLOC, b, sym, newValue);
}

} // namespace Base
} // namespace JB2
} // namespace OMR

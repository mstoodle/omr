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

#include <stdint.h>
#include "Base/ConstOperation.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

INIT_JBALLOC_REUSECAT(Op_Const, Operation)

Op_Const::Op_Const(MEM_LOCATION(a), Extension *ext, Builder * parent, ActionID aConst, Value * result, Literal *lv)
    : OperationR1L1(MEM_PASSLOC(a), aConst, ext, parent, result, lv) {
}

Op_Const::~Op_Const() {

}

Operation *
Op_Const::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    Allocator *mem = b->ir()->mem();
    return new (mem) Op_Const(MEM_PASSLOC(mem), this->_ext, b, this->action(), cloner->result(), cloner->literal());
}

} // namespace Base
} // namespace JB2
} // namespace OMR

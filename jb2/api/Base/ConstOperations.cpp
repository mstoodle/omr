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
#include "BaseExtension.hpp"
#include "BaseSymbols.hpp"
#include "BaseTypes.hpp"
#include "Builder.hpp"
#include "ConstOperations.hpp"
#include "Function.hpp"
#include "Literal.hpp"
#include "JB1MethodBuilder.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "OperationCloner.hpp"
#include "TextWriter.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {


Op_Const::Op_Const(LOCATION, Extension *ext, Builder * parent, ActionID aConst, Value * result, Literal *lv)
    : OperationR1L1(PASSLOC, aConst, ext, parent, result, lv) {
}

Operation *
Op_Const::clone(LOCATION, Builder *b, OperationCloner *cloner) const {
    return new Op_Const(PASSLOC, this->_ext, b, this->action(), cloner->result(), cloner->literal());
}

void
Op_Const::jbgen(JB1MethodBuilder *j1mb) const {
    literal()->type()->createJB1ConstOp(location(), j1mb, parent(), result(), literal());
}


} // namespace Base
} // namespace JitBuilder
} // namespace OMR


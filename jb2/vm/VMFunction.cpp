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

#include "JBCore.hpp"
#include "vm/BytecodeBuilder.hpp"
#include "vm/VMExtension.hpp"
#include "vm/VMFunction.hpp"


namespace OMR {
namespace JB2 {
namespace VM {

INIT_JBALLOC_ON(VMFunction, Compiler)
SUBCLASS_KINDSERVICE_IMPL(VMFunction, "VMFunction", Func::Function, Extensible)

VMFunction::VMFunction(MEM_LOCATION(a), Compiler *compiler, VMExtension *vmx)
    : Func::Function(MEM_PASSLOC(a), compiler, getExtensibleClassKind())
    , _vmx(vmx) {

}

VMFunction::VMFunction(MEM_LOCATION(a), VMFunction *outerFunc)
    : Func::Function(MEM_PASSLOC(a), outerFunc, getExtensibleClassKind())
    , _vmx(outerFunc->vmx()) {

}

VMFunction::~VMFunction() {
}

Builder *
VMFunction::EntryBuilder(LOCATION, IR *ir, Scope *scope) {
    return _vmx->EntryBuilder(PASSLOC, ir, scope, "Entry");
}

} // namespace VM
} // namespace JB2
} // namespace OMR

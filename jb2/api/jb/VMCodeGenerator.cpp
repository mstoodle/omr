/*******************************************************************************
 * Copyright (c) 2023, 2023 IBM Corp. and others
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
#include "JBCore.hpp"
#include "jb/JBMethodBuilder.hpp"
#include "jb/VMCodeGenerator.hpp"
#include "vm/VM.hpp"


namespace OMR {
namespace JitBuilder {
namespace JB {

static const MajorID BASEDON_VMEXT_MAJOR=0;
static const MajorID BASEDON_VMEXT_MINOR=1;
static const MajorID BASEDON_VMEXT_PATCH=0;
const static SemanticVersion correctVMVersion(BASEDON_VMEXT_MAJOR,BASEDON_VMEXT_MINOR,BASEDON_VMEXT_PATCH);

INIT_JBALLOC_REUSECAT(VMJBCodeGenerator, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(VMJBCodeGenerator,"VMJBCodeGenerator",JBCodeGenerator,Extensible);

VMJBCodeGenerator::VMJBCodeGenerator(Allocator *a, VM::VMExtension *vmx)
    : JBCodeGenerator(a, vmx)
    , _vmx(vmx) {

    assert(vmx->semver()->isCompatibleWith(correctVMVersion));
}

VMJBCodeGenerator::~VMJBCodeGenerator() {
}


bool
VMJBCodeGenerator::registerBuilder(JBMethodBuilder *jbmb, Builder *b) {
    if (b->isKind<VM::BytecodeBuilder>()) { // maybe assert?
        VM::BytecodeBuilder *bcb = b->refine<VM::BytecodeBuilder>();
        jbmb->createBytecodeBuilder(b, bcb->bcIndex(), bcb->name());
        return true;
    }

    return false;
}

} // namespace JB
} // namespace JitBuilder
} // namespace OMR

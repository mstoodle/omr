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
#include "jb2/JBCore.hpp"
#include "jb2cg/OMRCodeGenerator.hpp"
#include "jb2cg/OMRCodeGeneratorForVM.hpp"
#include "jb2cg/OMRIlGen.hpp"


namespace OMR {
namespace JB2 {
namespace omrgen {

static const MajorID BASEDON_VMEXT_MAJOR=0;
static const MajorID BASEDON_VMEXT_MINOR=1;
static const MajorID BASEDON_VMEXT_PATCH=0;
const static SemanticVersion correctVMVersion(BASEDON_VMEXT_MAJOR,BASEDON_VMEXT_MINOR,BASEDON_VMEXT_PATCH);

INIT_JBALLOC_REUSECAT(OMRCodeGeneratorForVM, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(OMRCodeGeneratorForVM,"OMRCodeGeneratorForVM",CodeGeneratorForVM,Extensible);

OMRCodeGeneratorForVM::OMRCodeGeneratorForVM(Allocator *a, OMRCodeGenerator *omrcg, VM::VMExtension *vmx)
    : CodeGeneratorForVM(a, omrcg, vmx)
    , _vmx(vmx) {

    assert(vmx->semver()->isCompatibleWith(correctVMVersion));
}

OMRCodeGeneratorForVM::~OMRCodeGeneratorForVM() {
}


OMRCodeGenerator *
OMRCodeGeneratorForVM::omrcg() const {
    return cg()->refine<OMRCodeGenerator>();
}

OMRIlGen *
OMRCodeGeneratorForVM::ilgen() const {
    return omrcg()->ilgen();
}

bool
OMRCodeGeneratorForVM::registerBuilder(Builder *b) {
    VM::VMBuilderAddon *vmba = b->addon<VM::VMBuilderAddon>();
    ilgen()->registerBuilder(b); //, vmba->bcIndex());
    return true;
}

} // namespace omrgen
} // namespace JB2
} // namespace OMR

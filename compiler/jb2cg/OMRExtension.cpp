/******************************************************************************* * Copyright (c) 2021, 2022 IBM Corp. and others *
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

#include "jb2/JBCore.hpp"
#include "jb2/Base/Base.hpp"
#include "jb2/Func/Func.hpp"
#include "jb2/vm/VMExtension.hpp"
#include "jb2cg/omrgen.hpp"
#include "jb2cg/OMRCompiler.hpp"

namespace OMR {
namespace JB2 {
namespace omrgen {

INIT_JBALLOC_REUSECAT(OMRExtension, Extension)
SUBCLASS_KINDSERVICE_IMPL(OMRExtension,"OMRExtension",Extension,Extensible);

const SemanticVersion OMRExtension::version(OMREXT_MAJOR,OMREXT_MINOR,OMREXT_PATCH);
const String OMRExtension::NAME(OMRGEN_NAME);

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        Allocator *mem = compiler->mem();
        return new (mem) OMRExtension(MEM_PASSLOC(mem), compiler);
    }
}

OMRExtension::OMRExtension(MEM_LOCATION(a), Compiler *compiler, bool extended, String extensionName)
    : Extension(MEM_PASSLOC(a), CLASSKIND(OMRExtension,Extensible), compiler, (extended ? extensionName : NAME)) {

    _omr = OMRCompiler::instance();
    _omr->initialize();

    // register extended passes here
    Allocator *mem = compiler->mem();
    _omrcg = new (mem) OMRCodeGenerator(mem, this);
    compiler->registerExtensible(_omrcg, CLASSKIND(CodeGenerator, Extensible));
}

OMRExtension::~OMRExtension() {
    _omr->shutdown();
}

void
OMRExtension::notifyNewExtension(Extension *other) {
    Allocator *mem = other->allocator();
    if (other->isExactKind<Base::BaseExtension>()) {
        Base::BaseExtension *bx = other->refine<Base::BaseExtension>();
        OMRCodeGeneratorForBase *bcg = new (mem) OMRCodeGeneratorForBase(mem, _omrcg, bx);
        OMRCodeGeneratorExtensionAddon *cgea = new (mem) OMRCodeGeneratorExtensionAddon(mem, bx, bcg);
        bx->attach(cgea);
    } else if (other->isExactKind<CoreExtension>()) {
        CoreExtension *cx = other->refine<CoreExtension>();
        OMRCodeGeneratorForCore *ccg = new (mem) OMRCodeGeneratorForCore(mem, _omrcg, cx);
        OMRCodeGeneratorExtensionAddon *cgea = new (mem) OMRCodeGeneratorExtensionAddon(mem, cx, ccg);
        cx->attach(cgea);
    } else if (other->isExactKind<Func::FunctionExtension>()) {
        Func::FunctionExtension *fx = other->refine<Func::FunctionExtension>();
        OMRCodeGeneratorForFunc *fcg = new (mem) OMRCodeGeneratorForFunc(mem, _omrcg, fx);
        OMRCodeGeneratorExtensionAddon *cgea = new (mem) OMRCodeGeneratorExtensionAddon(mem, fx, fcg);
        fx->attach(cgea);
    } else if (other->isExactKind<VM::VMExtension>()) {
        VM::VMExtension *vmx = other->refine<VM::VMExtension>();
        OMRCodeGeneratorForVM *vmcg = new (mem) OMRCodeGeneratorForVM(mem, _omrcg, vmx);
        OMRCodeGeneratorExtensionAddon *cgea = new (mem) OMRCodeGeneratorExtensionAddon(mem, vmx, vmcg);
        vmx->attach(cgea);
    }
}

} // namespace omrgen
} // namespace JB2
} // namespace OMR

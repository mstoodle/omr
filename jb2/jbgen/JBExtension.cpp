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

#include "JBCore.hpp"
#include "jbgen/JBCodeGenerator.hpp"
#include "jbgen/JBCodeGeneratorExtensionAddon.hpp"
#include "jbgen/JBCodeGeneratorForBase.hpp"
#include "jbgen/JBCodeGeneratorForCore.hpp"
#include "jbgen/JBCodeGeneratorForFunc.hpp"
#include "jbgen/JBCodeGeneratorForVM.hpp"
#include "jbgen/JBExtension.hpp"
#include "jbgen/OMRJB.hpp"
#include "Base/Base.hpp"
#include "Func/Func.hpp"
#include "vm/VMExtension.hpp"

namespace OMR {
namespace JB2 {
namespace jbgen {

INIT_JBALLOC_REUSECAT(JBExtension, Extension)
SUBCLASS_KINDSERVICE_IMPL(JBExtension,"JBExtension",Extension,Extensible);

const SemanticVersion JBExtension::version(JBEXT_MAJOR,JBEXT_MINOR,JBEXT_PATCH);
const String JBExtension::NAME("jb2jbgen");

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        Allocator *mem = compiler->mem();
        return new (mem) JBExtension(MEM_PASSLOC(mem), compiler);
    }
}

JBExtension::JBExtension(MEM_LOCATION(a), Compiler *compiler, bool extended, String extensionName)
    : Extension(MEM_PASSLOC(a), CLASSKIND(JBExtension,Extensible), compiler, (extended ? extensionName : NAME)) {

    _jb = OMR_JB::instance();
    _jb->initialize();

    // register extended passes here
    Allocator *mem = compiler->mem();
    _jbcg = new (mem) JBCodeGenerator(mem, this);
    compiler->registerExtensible(_jbcg, CLASSKIND(CodeGenerator, Extensible));
}

JBExtension::~JBExtension() {
    _jb->shutdown();
}

void
JBExtension::notifyNewExtension(Extension *other) {
    Allocator *mem = other->allocator();
    if (other->isExactKind<Base::BaseExtension>()) {
        Base::BaseExtension *bx = other->refine<Base::BaseExtension>();
        JBCodeGeneratorForBase *bcg = new (mem) JBCodeGeneratorForBase(mem, _jbcg, bx);
        JBCodeGeneratorExtensionAddon *cgea = new (mem) JBCodeGeneratorExtensionAddon(mem, bx, bcg);
        bx->attach(cgea);
    } else if (other->isExactKind<CoreExtension>()) {
        CoreExtension *cx = other->refine<CoreExtension>();
        JBCodeGeneratorForCore *ccg = new (mem) JBCodeGeneratorForCore(mem, _jbcg, cx);
        JBCodeGeneratorExtensionAddon *cgea = new (mem) JBCodeGeneratorExtensionAddon(mem, cx, ccg);
        cx->attach(cgea);
    } else if (other->isExactKind<Func::FunctionExtension>()) {
        Func::FunctionExtension *fx = other->refine<Func::FunctionExtension>();
        JBCodeGeneratorForFunc *fcg = new (mem) JBCodeGeneratorForFunc(mem, _jbcg, fx);
        JBCodeGeneratorExtensionAddon *cgea = new (mem) JBCodeGeneratorExtensionAddon(mem, fx, fcg);
        fx->attach(cgea);
    } else if (other->isExactKind<VM::VMExtension>()) {
        VM::VMExtension *vmx = other->refine<VM::VMExtension>();
        JBCodeGeneratorForVM *vmcg = new (mem) JBCodeGeneratorForVM(mem, _jbcg, vmx);
        JBCodeGeneratorExtensionAddon *cgea = new (mem) JBCodeGeneratorExtensionAddon(mem, vmx, vmcg);
        vmx->attach(cgea);
    }
}

} // namespace jbgen
} // namespace JB2
} // namespace OMR

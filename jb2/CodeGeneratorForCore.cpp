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

#include "CodeGeneratorForCore.hpp"
#include "CoreExtension.hpp"
#include "Operation.hpp"


namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(CodeGeneratorForCore, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(CodeGeneratorForCore,"CodeGeneratorForCore",CodeGeneratorForExtension,Extensible);

CodeGeneratorForCore::CodeGeneratorForCore(Allocator *a, CodeGenerator *cg, CoreExtension *cx)
    : CodeGeneratorForExtension(a, cg, CLASSKIND(CodeGeneratorForCore,Extensible), cx, "CodeGeneratorForCore") {

    setTraceEnabled(false);
}

CodeGeneratorForCore::~CodeGeneratorForCore() {
}

CoreExtension *
CodeGeneratorForCore::cx() const {
    return ext()->refine<CoreExtension>();
}

Builder *
CodeGeneratorForCore::gencode(Operation *op) {
    CoreExtension *cx = ext()->refine<CoreExtension>();
    ActionID a = op->action();
    if (a == cx->aMergeDef)
        return this->gencodeMergeDef(op);
    else if (a == cx->aAppendBuilder)
        return this->gencodeAppendBuilder(op);
    return NULL;
}

MISSING_CG_OP_HANDLER(CodeGeneratorForCore, AppendBuilder)
MISSING_CG_OP_HANDLER(CodeGeneratorForCore, MergeDef);

} // namespace JB2
} // namespace OMR

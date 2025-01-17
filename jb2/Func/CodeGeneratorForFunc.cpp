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

#include "Func/CodeGeneratorForFunc.hpp"
#include "Func/FunctionExtension.hpp"


namespace OMR {
namespace JB2 {
namespace Func {

INIT_JBALLOC_REUSECAT(CodeGeneratorForFunc, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(CodeGeneratorForFunc,"CodeGeneratorForFunc",CodeGeneratorForExtension,Extensible);

CodeGeneratorForFunc::CodeGeneratorForFunc(Allocator *a, CodeGenerator *cg, FunctionExtension *fx)
    : CodeGeneratorForExtension(a, cg, CLASSKIND(CodeGeneratorForFunc,Extensible), fx, "CodeGeneratorForFunc")
    , INIT_CG_FUNC_VFT_FIELDS(a) {

    INIT_CG_FUNC_HANDLERS(CodeGeneratorForFunc);

    setTraceEnabled(false);
}

CodeGeneratorForFunc::~CodeGeneratorForFunc() {
}

DEFINE_CG_FUNC_HANDLER_DISPATCH(CodeGeneratorForFunc)

MISSING_CG_OP_HANDLER(CodeGeneratorForFunc,Call)
MISSING_CG_OP_HANDLER(CodeGeneratorForFunc,CallVoid)
MISSING_CG_OP_HANDLER(CodeGeneratorForFunc,Load)
MISSING_CG_OP_HANDLER(CodeGeneratorForFunc,Return)
MISSING_CG_OP_HANDLER(CodeGeneratorForFunc,ReturnVoid)
MISSING_CG_OP_HANDLER(CodeGeneratorForFunc,Store)

} // namespace Func
} // namespace JB2
} // namespace OMR

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

#include "Builder.hpp"
#include "CodeGenerator.hpp"
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "CoreExtension.hpp"
#include "Dispatcher.hpp"
#include "IR.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "SemanticVersion.hpp"
#include "Strategy.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_ON(CoreExtension, Compiler);
SUBCLASS_KINDSERVICE_IMPL(CoreExtension,"CoreExtension",Extension,Extensible);

const SemanticVersion CoreExtension::version((MajorID)0,(MinorID)1,(PatchID)0);
const String CoreExtension::NAME("core");

// used by Compiler to allocate an actual Extension object
CoreExtension::CoreExtension(Allocator *a, LOCATION, Compiler *compiler)
    : Extension(MEM_PASSLOC(a), CLASSKIND(CoreExtension,Extensible), compiler, NAME)
    , _codegenStrategy(new (a) Strategy(a, compiler, "CodeGen"))
    , tNoType(NoTypeID)
    , aAppendBuilder(registerAction(String(a, "AppendBuilder")))
    , aMergeDef(registerAction(String(a, "MergeDef")))
    , CompileFail_CodeGeneratorMissingOperationHandler(registerReturnCode(String(a, "CompileFail_CodeGeneratorMissingOperationHandler")))
    , strategyCodegen(_codegenStrategy->id()) {

    _dispatcher = new (a) Dispatcher<CodeGenerator>(a, this, "CodeGenDispatcher");
    _codegenStrategy->addPass(_dispatcher);
}

CoreExtension::~CoreExtension() {
    delete _dispatcher;
}

const NoTypeType *
CoreExtension::NoType(IR *ir) {
    return ir->NoType;
}

//
// Core Operations
//

void
CoreExtension::AppendBuilder(LOCATION, Builder *parent, Builder *b) {
    Allocator *mem = parent->ir()->mem();
    addOperation(b, new (mem) Op_AppendBuilder(MEM_PASSLOC(mem), this, b, this->aAppendBuilder, b));
}

void
CoreExtension::MergeDef(LOCATION, Builder *b, Value *existingDef, Value *newDef) {
    Allocator *mem = b->ir()->mem();
    addOperation(b, new (mem) Op_MergeDef(MEM_PASSLOC(mem), this, b, this->aMergeDef, existingDef, newDef));
}

} // namespace JB2
} // namespace OMR

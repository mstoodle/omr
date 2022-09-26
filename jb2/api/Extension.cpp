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
#include "Compiler.hpp"
#include "Extension.hpp"
#include "Operation.hpp"
#include "SemanticVersion.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {


const SemanticVersion Extension::version(0,0,0);

Extension::Extension(Compiler *compiler, std::string name)
    : _id(compiler->getExtensionID())
    , _name(name)
    , _compiler(compiler)
    , _types()
    , aMergeDef(registerAction(std::string("MergeDef"))) {
}

const std::string
Extension::actionName(ActionID id) const {
    return _compiler->actionName(id);
}

ActionID
Extension::registerAction(std::string name) {
    return _compiler->assignActionID(name);
}

CompilerReturnCode
Extension::registerReturnCode(std::string name) {
    return _compiler->assignReturnCode(name);
}

PassID
Extension::addPass(Pass *pass) {
    return _compiler->addPass(pass);
}

Value *
Extension::createValue(const Builder *parent, const Type *type) {
    return Value::create(parent, type);
}

void
Extension::addOperation(Builder *b, Operation *op) {
    b->add(op);
}


//
// Const Operations
//

void
Extension::MergeDef(LOCATION, Builder *b, Value *existingDef, Value *newDef) {
    addOperation(b, new Op_MergeDef(PASSLOC, this, b, this->aMergeDef, existingDef, newDef));
}


//
// Const Pseudo Operations
//

Builder *
Extension::BoundBuilder(LOCATION, Builder *parent, Operation *parentOp, std::string name) {
    return new Builder(parent, parentOp, name);
}

Builder *
Extension::OrphanBuilder(LOCATION, Builder *parent, Context *context, std::string name) {
    return new Builder(parent, context, name);
}

} // namespace JitBuilder
} // namespace OMR


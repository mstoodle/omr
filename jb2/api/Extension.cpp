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
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Extension.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "SemanticVersion.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JitBuilder {


const SemanticVersion Extension::version((MajorID)0,(MinorID)0,(PatchID)0);
const String Extension::NAME("core");

INIT_JBALLOC_ON(Extension, Compiler)

// used by Compiler to allocate an actual Extension object
Extension::Extension(Allocator *a, LOCATION, Compiler *compiler)
    : Allocatable(a)
    , _id(compiler->getExtensionID())
    , _name(NAME)
    , _compiler(compiler)
    , _createLoc(PASSLOC)
    , _types((Allocator *)NULL, a)
    , NoType(new (a) NoTypeType(MEM_LOC(a), this))
    , aMergeDef(registerAction(String("MergeDef"))) {
}

Extension::Extension(Allocator *a, LOCATION, Compiler *compiler, String name)
    : Allocatable(a)
    , _id(compiler->getExtensionID())
    , _name(name)
    , _compiler(compiler)
    , _createLoc(PASSLOC)
    , _types((Allocator *)NULL, a)
    , NoType(new (a) NoTypeType(MEM_LOC(a), this))
    , aMergeDef(registerAction(String("MergeDef"))) {
}

Extension::~Extension() {

}

const String
Extension::actionName(ActionID id) const {
    return _compiler->actionName(id);
}

ActionID
Extension::registerAction(String name) {
    return _compiler->assignActionID(name);
}

CompilerReturnCode
Extension::registerReturnCode(String name) {
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

Builder *
Extension::internalRegisterBuilder(Compilation *comp, Builder *b) {
    return comp->registerBuilder(b);
}


//
// Core Operations
//

void
Extension::MergeDef(LOCATION, Builder *b, Value *existingDef, Value *newDef) {
    Allocator *mem = b->comp()->mem();
    addOperation(b, new (mem) Op_MergeDef(MEM_PASSLOC(mem), this, b, this->aMergeDef, existingDef, newDef));
}


//
// Core Pseudo Operations
//

Builder *
Extension::BoundBuilder(LOCATION, Builder *parent, Operation *parentOp, String name) {
    Compilation *comp = parent->comp();
    Allocator *mem = comp->mem();
    return internalRegisterBuilder(comp, new (mem) Builder(mem, parent, parentOp, name));
}

Builder *
Extension::OrphanBuilder(LOCATION, Builder *parent, Context *context, String name) {
    Compilation *comp = parent->comp();
    Allocator *mem = comp->mem();
    return internalRegisterBuilder(comp, new (mem) Builder(mem, parent, context, name));
}

Builder *
Extension::EntryBuilder(LOCATION, Compilation *comp, Context *context, String name) {
    Allocator *mem = comp->mem();
    return internalRegisterBuilder(comp, new (mem) Builder(mem, comp, context, name));
}

Builder *
Extension::ExitBuilder(LOCATION, Compilation *comp, Context *context, String name) {
    Allocator *mem = comp->mem();
    return internalRegisterBuilder(comp, new (mem) Builder(mem, comp, context, name));
}

Location *
Extension::SourceLocation(LOCATION, Builder *b, String func) {
    Allocator *mem = b->comp()->mem();
    Location *loc = new (mem) Location(mem, b->comp(), func, "");
    b->setLocation(loc);
    return loc;
}

Location *
Extension::SourceLocation(LOCATION, Builder *b, String func, String lineNumber) {
    Allocator *mem = b->comp()->mem();
    Location *loc = new (mem) Location(mem, b->comp(), func, lineNumber);
    b->setLocation(loc);
    return loc;
}

Location *
Extension::SourceLocation(LOCATION, Builder *b, String func, String lineNumber, int32_t bcIndex) {
    Allocator *mem = b->comp()->mem();
    Location *loc = new (mem) Location(mem, b->comp(), func, lineNumber, bcIndex);
    b->setLocation(loc);
    return loc;
}

} // namespace JitBuilder
} // namespace OMR


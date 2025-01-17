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

#include <iostream>
#include "Builder.hpp"
#include "CodeGenerator.hpp"
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Config.hpp"
#include "Dispatcher.hpp"
#include "Extension.hpp"
#include "IDs.hpp"
#include "IR.hpp"
#include "Location.hpp"
#include "Operation.hpp"
#include "SemanticVersion.hpp"
#include "Strategy.hpp"
#include "TextLogger.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_ON(Extension, Compiler);
SUBCLASS_KINDSERVICE_IMPL(Extension,"Extension",Extensible,Extensible);

const SemanticVersion Extension::version((MajorID)0,(MinorID)0,(PatchID)0);
const String Extension::NAME("primordial");

Extension::Extension(MEM_LOCATION(a), KINDTYPE(Extensible) kind, Compiler *compiler, String name)
    : Extensible(a, this, kind)
    , _id(compiler->getExtensionID())
    , _name(name)
    , _compiler(compiler)
    , _createLoc(PASSLOC)
    , _types((Allocator *)NULL, a)
    , _codegenStrategy(NULL) {

    Config *cfg = compiler->config();
    TextLogger *lgr = cfg->logger();
    if (lgr)
        (*lgr) << "Extension loaded " << name << lgr->endl();
}

Extension::~Extension() {
    for (auto it = _extendedPasses.begin(); it != _extendedPasses.end(); it++) {
        Pass *extendedPass = it->second;
        delete extendedPass;
    }
    _extendedPasses.clear();
}

const String
Extension::actionName(ActionID id) const {
    return _compiler->actionName(id);
}

const ActionID
Extension::registerAction(String name) const {
    return _compiler->assignActionID(name);
}

const CompilerReturnCode
Extension::registerReturnCode(String name) const {
    return _compiler->assignReturnCode(name);
}

const TypeID
Extension::registerType() const {
    return _compiler->irPrototype()->getTypeID();
}

const PassID
Extension::addPass(Pass *pass) {
    return _compiler->addPass(pass);
}

Value *
Extension::createValue(Builder *parent, const Type *type) {
    return Value::create(parent, type);
}

void
Extension::addOperation(Builder *b, Operation *op) {
    b->add(op);
}

void
Extension::registerBuilder(IR *ir, Builder *b) {
    return ir->registerBuilder(b);
}

void
Extension::registerExtendedPass(Extension *ext, KINDTYPE(Extensible) kind, Pass *extendedPass) {
    ext->_extendedPasses.insert({kind, extendedPass});
}

void
Extension::registerForExtensible(ExtensibleKind kind, Extension *ext) {
    compiler()->registerForExtensible(kind, ext);
}

void
Extension::setContext(Compilation *comp, Context *context) {
    comp->ir()->setContext(context);
}

void
Extension::setScope(Compilation *comp, Scope *scope) {
    comp->ir()->setScope(scope);
}

void
Extension::setLogger(Compilation *comp, TextLogger *logger) {
    comp->setLogger(logger);
}

//
// Core Pseudo Operations
//

Builder *
Extension::BoundBuilder(LOCATION, Builder *parent, Operation *parentOp, String name) {
    IR *ir = parent->ir();
    Allocator *mem = ir->mem();
    Builder *b = new (mem) Builder(mem, this, parent, parentOp, name);
    this->registerBuilder(ir, b);
    return b;
}

Builder *
Extension::EntryBuilder(LOCATION, IR *ir, Scope *scope, String name) {
    Allocator *mem = ir->mem();
    Builder *b = new (mem) Builder(mem, this, ir, scope, name);
    this->registerBuilder(ir, b);
    return b;
}

Builder *
Extension::ExitBuilder(LOCATION, IR *ir, Scope *scope, String name) {
    Allocator *mem = ir->mem();
    Builder *b = new (mem) Builder(mem, this, ir, scope, name);
    this->registerBuilder(ir, b);
    return b;
}

Builder *
Extension::OrphanBuilder(LOCATION, Builder *parent, Scope *scope, String name) {
    if (scope == NULL)
        scope = parent->scope();
    Allocator *mem = parent->ir()->mem();
    Builder *b = new (mem) Builder(mem, this, parent, scope, name);
    this->registerBuilder(parent->ir(), b);
    return b;
}

Location *
Extension::SourceLocation(LOCATION, Builder *b, String func) {
    Allocator *mem = b->ir()->mem();
    Location *loc = new (mem) Location(mem, b->ir(), func, "");
    b->setLocation(loc);
    return loc;
}

Location *
Extension::SourceLocation(LOCATION, Builder *b, String func, String lineNumber) {
    Allocator *mem = b->ir()->mem();
    Location *loc = new (mem) Location(mem, b->ir(), func, lineNumber);
    b->setLocation(loc);
    return loc;
}

Location *
Extension::SourceLocation(LOCATION, Builder *b, String func, String lineNumber, int32_t bcIndex) {
    Allocator *mem = b->ir()->mem();
    Location *loc = new (mem) Location(mem, b->ir(), func, lineNumber, bcIndex);
    b->setLocation(loc);
    return loc;
}

} // namespace JB2
} // namespace OMR

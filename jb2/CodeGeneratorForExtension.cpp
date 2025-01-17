/*******************************************************************************
 * Copyright (c) 2024 IBM Corp. and others
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

#include "CodeGenerator.hpp"
#include "CodeGeneratorForExtension.hpp"
#include "Compiler.hpp"
#include "CoreExtension.hpp"
#include "Operation.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC(CodeGeneratorForExtension)

SUBCLASS_KINDSERVICE_IMPL(CodeGeneratorForExtension, "CodeGeneratorForExtension", Extensible, Extensible)


CodeGeneratorForExtension::CodeGeneratorForExtension(Allocator *a, CodeGenerator *cg, KINDTYPE(Extensible) kind, Extension *ext, String name)
    : Extensible(a, ext, kind)
    , _cg(cg) {

}

CodeGeneratorForExtension::~CodeGeneratorForExtension() {
}

CodeGenerator *
CodeGeneratorForExtension::cg() const {
    return _cg;
}

MISSING_CG_OP_HANDLER(CodeGeneratorForExtension,)

void
CodeGeneratorForExtension::setupbody(Compilation *comp) {
}

void
CodeGeneratorForExtension::genbody(Compilation *comp) {
}

void
CodeGeneratorForExtension::connectsuccessors(Builder *b) {

}

bool
CodeGeneratorForExtension::registerBuilder(Builder *b) {
    return true;
}

bool
CodeGeneratorForExtension::registerContext(Context *c) {
    return true;
}

bool
CodeGeneratorForExtension::registerLiteral(Literal *lv) {
    return true;
}

bool
CodeGeneratorForExtension::registerScope(Scope *s) {
    return true;
}

bool
CodeGeneratorForExtension::registerSymbol(Symbol *sym) {
    return true;
}

bool
CodeGeneratorForExtension::registerType(const Type *type) {
    return true;
}

bool
CodeGeneratorForExtension::registerValue(Value *value) {
    return true;
}

Builder *
CodeGeneratorForExtension::missingCodeGeneratorOperation(LOCATION, Operation *op) {
    CoreExtension *cx = compiler()->lookupExtension<CoreExtension>();
    CompilationException e(PASSLOC, compiler(), cx->CompileFail_CodeGeneratorMissingOperationHandler);
    Allocator *mem = compiler()->mem();
    e.setMessageLine(String(mem, "Extension lacks a CodeGenerator handler for an Operation"))
     .appendMessageLine(String(mem, "   Extension ").append(ext()->name()))
     .appendMessageLine(String(mem, "   CodeGenerator ").append(cg()->name()))
     .appendMessageLine(String(mem, "   Operation op").append(String::to_string(mem, op->id())))
     .appendMessageLine(String(mem, "The code generator tried could not find a handler to generate code for the operation."))
     .appendMessageLine(String(mem, "Usually means that <CodeGenerator name>CodeGeneratorFor<Extension name>::gencode() does not know how to handle this kind of Operation."));
    throw e;
}

} // namespace JB2
} // namespace OMR

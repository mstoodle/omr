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

#include "JBCore.hpp"
#include "Func/FunctionExtension.hpp"
#include "Func/Function.hpp"
#include "Func/FunctionCompilation.hpp"
#include "Func/FunctionContext.hpp"
#include "Func/FunctionScope.hpp"


namespace OMR {
namespace JB2 {
namespace Func {

INIT_JBALLOC_ON(Function, Compiler)
SUBCLASS_KINDSERVICE_IMPL(Function,"Function",CompileUnit,Extensible)

Function::Function(MEM_LOCATION(a), Compiler *compiler, String name)
    : CompileUnit (MEM_PASSLOC(a), compiler, getExtensibleClassKind(), name)
    , _cx(compiler->coreExt()) {

}

Function::Function(MEM_LOCATION(a), Function *outerFunc, String name)
    : CompileUnit(MEM_PASSLOC(a), outerFunc, getExtensibleClassKind(), name)
    , _cx(_compiler->coreExt()){

}

Function::Function(MEM_LOCATION(a), Compiler *compiler, ExtensibleKind kind, String name)
    : CompileUnit (MEM_PASSLOC(a), compiler, kind, name)
    , _cx(compiler->coreExt()) {

}

Function::Function(MEM_LOCATION(a), Function *outerFunc, ExtensibleKind kind, String name)
    : CompileUnit(MEM_PASSLOC(a), outerFunc, kind, name)
    , _cx(_compiler->coreExt()){

}

Function::~Function() {
}

FunctionCompilation *
Function::fcomp(Compilation *comp) {
    return comp->refine<FunctionCompilation>();
}

FunctionScope *
Function::fscope(Compilation *comp) {
    return comp->scope<FunctionScope>();
}

FunctionContext *
Function::fctx(Compilation *comp) {
    return comp->context<FunctionContext>();
}

void
Function::DefineName(String name) {
    _givenName = String(compiler()->mem(), name);
}

void
Function::DefineFile(String file) {
    _fileName = String(compiler()->mem(), file);
}

void
Function::DefineLine(String line) {
    _lineNumber = String(compiler()->mem(), line);
}

void
Function::logContents(TextLogger &lgr) const {
    lgr.indent() << lgr.irStart() << "name " << name() << lgr.irStop() << lgr.endl();
    lgr.indent() << lgr.irStart() << "fileName " << fileName() << lgr.irStop() << lgr.endl();
    lgr.indent() << lgr.irStart() << "lineNumber " << lineNumber() << lgr.irStop() << lgr.endl();
}

} // namespace Function
} // namespace JB2
} // namespace OMR


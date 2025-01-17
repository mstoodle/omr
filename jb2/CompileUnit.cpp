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
#include "CompiledBody.hpp"
#include "Compiler.hpp"
#include "CompileUnit.hpp"
#include "Context.hpp"
#include "CoreExtension.hpp"
#include "Operation.hpp"
#include "TextLogger.hpp"
#include "TypeDictionary.hpp"
#include "TypeReplacer.hpp"
#include "Value.hpp"


namespace OMR {
namespace JB2 {

INIT_JBALLOC_ON(CompileUnit, Compiler)
SUBCLASS_KINDSERVICE_IMPL(CompileUnit,"CompileUnit",ExtensibleIR,Extensible)

CompileUnit::CompileUnit(MEM_LOCATION(a), Compiler *compiler, ExtensibleKind kind, String name)
    : Extensible(a, compiler->coreExt(), kind)
    , _compiler(compiler)
    , _id(compiler->getCompileUnitID())
    , _createLocation(PASSLOC)
    , _name(compiler->mem(), name)
    , _outerUnit(NULL) {
}

CompileUnit::CompileUnit(MEM_LOCATION(a), CompileUnit *outerUnit, ExtensibleKind kind, String name)
    : Extensible(a, outerUnit->compiler()->coreExt(), kind)
    , _compiler(outerUnit->_compiler)
    , _id(outerUnit->_compiler->getCompileUnitID())
    , _createLocation(PASSLOC)
    , _name(_compiler->mem(), name)
    , _outerUnit(outerUnit) {
}

CompileUnit::~CompileUnit() {
    for (auto it=_bodies.begin(); it != _bodies.end(); it++) {
        CompiledBody *body = it->second;
        delete body;
    }
}

Builder *
CompileUnit::EntryBuilder(LOCATION, IR *ir, Scope *scope) {
    return ir->compiler()->coreExt()->EntryBuilder(PASSLOC, ir, scope, "Entry");
}

void
CompileUnit::log(TextLogger &lgr) const {
    lgr.irSectionBegin("unit", "u", id(), _kind, name());

    logContents(lgr);

    lgr.irSectionEnd();
}

#if 0
CompilerReturnCode
CompileUnit::compile(LOCATION, StrategyID strategy, TextLogger *lgr) {
    Compilation comp(_compiler->coreExt(), CLASSKIND(Compilation,Extensible), this);
    Context context(PASSLOC, Context::getContextClassKind(), _compiler->coreExt(), &comp);
    comp.setContext(&context);
    comp.setLogger(lgr);

    CompilerReturnCode rc = _compiler->compile(PASSLOC, &comp, strategy);
    if (rc != _compiler->CompileSuccessful) {
        return rc;
    }

    Allocator *mem = _compiler->mem();
    CompiledBody *body = new (mem) CompiledBody(mem, this, &context, strategy);
    auto it = _bodies.find(strategy);
    if (it != _bodies.end()) {
        notifyRecompile(it->second, body);
        _bodies.erase(it);
    }
    _bodies.insert({strategy, body});

    return rc;
}
#endif

CompiledBody *
CompileUnit::compiledBody(StrategyID strategy) const {
    auto it = _bodies.find(strategy);
    if (it == _bodies.end())
        return NULL;

    return it->second;
}

void
CompileUnit::saveCompiledBody(CompiledBody *body, StrategyID strategy) {
    auto it = _bodies.find(strategy);
    if (it != _bodies.end()) {
        _compiler->notifyRecompile(this, it->second, body, strategy);
        _bodies.erase(it);
    }
    _bodies.insert({strategy, body});
}

} // namespace JB2
} // namespace OMR

/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
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
#include "CompileUnit.hpp"
#include "Config.hpp"
#include "Context.hpp"
#include "Extension.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(Compilation, Compiler)
SUBCLASS_KINDSERVICE_IMPL(Compilation,"Compilation",Extensible,Extensible)

Compilation::Compilation(Allocator *a, Extension *ext, ExtensibleKind kind, CompileUnit *unit, StrategyID strategy, Config *config)
    : Extensible(a, ext, kind)
    , _id(ext->compiler()->getCompilationID())
    , _nextBuilderID(NoBuilder+1) // must precede anything that might create Builders
    , _nextContextID(NoContext+1) // must precede anything that might create Contexts
    , _nextLiteralID(NoLiteral+1) // must precede anything that might create Literal
    , _nextLocationID(NoLocation+1) // must precede anything that might create Locations
    , _nextOperationID(NoOperation+1) // must precede anything that might create Operations
    , _nextScopeID(NoScope+1) // must precede anything that might create Scopes
    , _nextTransformationID(NoTransformation) // must precede anything that might create Transformations
    , _nextValueID(NoValue) // must precede anything that might create Values
    , _compiler(ext->compiler())
    , _ext(ext)
    , _unit(unit)
    , _strategy(strategy)
    , _context(NULL) // expected to be created by subclass or by extension's ::compile
    , _scope(NULL) // expected to be created by subclass or by extension's ::compile
    , _myConfig(config == NULL)
    , _config((config == NULL) ? new (_compiler->mem()) Config(_compiler->mem(), _compiler->config()) : config)
    , _mem(_config->compilationAllocator(_compiler->mem()))
    , _passMem(NULL)
    , _literalDict(new (_mem) LiteralDictionary(_mem, _compiler, "Compilation Literal Dictionary", _compiler->litDict()))
    , _symbolDict(new (_mem) SymbolDictionary(_mem, _compiler, "Compilation Symbol Dictionary", _compiler->symDict()))
    , _typeDict(new (_mem) TypeDictionary(_mem, _compiler, "Compilation Type Dictionary", _compiler->typeDict()))
    , _logger(NULL)
    , _writer(NULL)
    , _builders(NULL, _mem) {

    notifyCreation(ext, KIND(Extensible));
}

Compilation::~Compilation() {
    delete _typeDict;
    delete _symbolDict;
    delete _literalDict;
    _config->destructCompilationAllocator(_mem);
    if (_myConfig && _config != NULL)
        delete _config;
    // scope and context are typically not dynamically allocated
    if (_scope->allocator())
        delete _scope;
    if (_context->allocator())
        delete _context;
}

void
Compilation::addInitialBuildersToWorklist(BuilderList & worklist) {
    for (auto it = _builders.iterator();it.hasItem(); it++) {
        Builder *b = it.item();
        worklist.push_back(b);
    }
}

Literal *
Compilation::registerLiteral(LOCATION, const Type *type, const LiteralBytes *value) {
    return _literalDict->registerLiteral(PASSLOC, type, value);
}

Builder *
Compilation::registerBuilder(Builder *b) {
    _builders.push_back(b);
    return b;
}

void
Compilation::log(TextLogger &lgr) const {
   lgr << lgr.endl();

   lgr.indentIn();
   TypeDictionary *td = typedict();
   td->log(lgr);

   SymbolDictionary *sd = symdict();
   sd->log(lgr);

   LiteralDictionary *ld = litdict();
   ld->log(lgr);
}

bool
Compilation::prepareIL(LOCATION) {
    if (unit()->buildContext(PASSLOC, this, _scope, _context) == false)
        return false;

    return unit()->buildIL(PASSLOC, this, _scope, _context);
}

void
Compilation::freeIL(LOCATION) {
    for (auto it=_builders.fwdIterator();it.hasItem(); it++) {
        Builder *b = it.item();
        delete b;
    }
}

} // namespace JitBuilder
} // namespace OMR

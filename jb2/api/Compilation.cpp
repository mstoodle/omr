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
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(Compilation, Compiler)

Compilation::Compilation(Compiler *compiler, CompileUnit *unit, StrategyID strategy, LiteralDictionary *litDict, SymbolDictionary *symDict, TypeDictionary *typeDict, Config *config)
    : Allocatable() // Compilation objects are allocated on the stack by Extension objects
    , _id(compiler->getCompilationID())
    , _nextBuilderID(NoBuilder+1) // must precede anything that might create Builders
    , _nextContextID(NoContext+1) // must precede anything that might create Contexts
    , _nextLiteralID(NoLiteral+1) // must precede anything that might create Literal
    , _nextLocationID(NoLocation+1) // must precede anything that might create Locations
    , _nextOperationID(NoOperation+1) // must precede anything that might create Operations
    , _nextTransformationID(NoTransformation) // must precede anything that might create Transformations
    , _nextValueID(NoValue) // must precede anything that might create Values
    , _compiler(compiler)
    , _unit(unit)
    , _strategy(strategy)
    , _myConfig(config == NULL)
    , _config((config == NULL) ? new (compiler->mem()) Config(compiler->mem(), compiler->config()) : config)
    , _context(NULL)
    , _mem(_config->compilationAllocator(_compiler->mem()))
    , _passMem(NULL)
    , _myLiteralDict(litDict == NULL)
    , _mySymbolDict(symDict == NULL)
    , _myTypeDict(typeDict == NULL)
    , _literalDict(_myLiteralDict ? new (_mem) LiteralDictionary(_mem, compiler, "Compilation Literal Dictionary", compiler->litDict()) : litDict)
    , _symbolDict(_mySymbolDict ? new (_mem) SymbolDictionary(_mem, compiler, "Compilation Symbol Dictionary", compiler->symDict()) : symDict)
    , _typeDict(_myTypeDict ? new (_mem) TypeDictionary(_mem, compiler, "Compilation Type Dictionary", compiler->typeDict()) : typeDict)
    , _logger(NULL)
    , _writer(NULL)
    , _builders(NULL, _mem) {

}

Compilation::~Compilation() {
    if (_myTypeDict && _typeDict != NULL)
        delete _typeDict;
    if (_mySymbolDict && _symbolDict != NULL)
        delete _symbolDict;
    if (_myLiteralDict && _literalDict != NULL)
        delete _literalDict;
    _config->destructCompilationAllocator(_mem);
    if (_myConfig && _config != NULL)
        delete _config;
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
    if (unit()->initContext(PASSLOC, this, _context) == false)
        return false;

    return unit()->buildIL(PASSLOC, this, _context);
}

void
Compilation::freeIL(LOCATION) {
    for (auto it=_builders.fwdIterator();it.hasItem(); it++) {
        Builder *b = it.item();
        delete b;
    }
}

void
Compilation::setNativeEntryPoint(void *entryPoint, int e) {
    _context->setNativeEntryPoint(entryPoint, e);
}

} // namespace JitBuilder
} // namespace OMR

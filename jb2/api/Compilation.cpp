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
#include "TextWriter.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JitBuilder {

BuilderIterator Compilation::endBuilderIterator;

Compilation::Compilation(Compiler *compiler, CompileUnit *unit, StrategyID strategy, TypeDictionary *typeDict, Config *config)
    : _id(compiler->getCompilationID())
    , _nextBuilderID(NoBuilder+1) // must precede anything that might create Builders
    , _nextContextID(NoContext+1) // must precede anything that might create Contexts
    , _nextLiteralDictionaryID(0) // must precede anything that might create LiteralDictionaries
    , _nextLiteralID(NoLiteral+1) // must precede anything that might create Literal
    , _nextLocationID(NoLocation+1) // must precede anything that might create Locations
    , _nextOperationID(NoOperation+1) // must precede anything that might create Operations
    , _nextSymbolDictionaryID(0) // must precede anything that might create SymbolDictionaries
    , _nextTransformationID(NoTransformation) // must precede anything that might create Transformations
    , _nextValueID(NoValue) // must precede anything that might create Values
    , _compiler(compiler)
    , _unit(unit)
    , _strategy(strategy)
    , _myConfig(config == NULL)
    , _config((config == NULL) ? new Config(compiler->config()) : config)
    , _context(NULL)
    , _literalDict(new LiteralDictionary(this))
    , _symbolDict(new SymbolDictionary(this))
    , _myTypeDict(typeDict == NULL)
    , _typeDict(_myTypeDict ? new TypeDictionary(compiler, "Compilation", compiler->dict()) : typeDict) {

}

Compilation::~Compilation() {
    if (_myTypeDict && _typeDict != NULL)
        delete _typeDict;
    if (_myConfig && _config != NULL)
        delete _config;
    delete _symbolDict;
    delete _literalDict;
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

void
Compilation::write(TextWriter &w) const {
   w << w.endl();

   w.indentIn();
   TypeDictionary *td = typedict();
   td->write(w);

   SymbolDictionary *sd = symdict();
   sd->write(w);

   LiteralDictionary *ld = litdict();
   ld->write(w);
}

bool
Compilation::prepareIL(LOCATION) {
    if (unit()->initContext(PASSLOC, this, _context) == false)
        return false;

    return unit()->buildIL(PASSLOC, this, _context);
}

void
Compilation::setNativeEntryPoint(void *entryPoint, int e) {
    _context->setNativeEntryPoint(entryPoint, e);
}

} // namespace JitBuilder
} // namespace OMR

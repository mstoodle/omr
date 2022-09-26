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
#include "Config.hpp"
#include "Context.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "SymbolDictionary.hpp"
#include "TextWriter.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JitBuilder {

CompilationID Compilation::nextCompilationID = 1; // 0 is reserved

BuilderIterator Compilation::endBuilderIterator;

Compilation::Compilation(Compiler *compiler, TypeDictionary *typeDict, Config *config)
    : _id(nextCompilationID++)
    , _compiler(compiler)
    , _config(config)
    , _myConfig(true)
    , _context(new Context(this, NULL, "root"))
    , _literalDict(new LiteralDictionary(this))
    , _symbolDict(new SymbolDictionary(this))
    , _typeDict(typeDict)
    , _nextBuilderID(NoBuilder+1)
    //, _nextCaseID(No)
    , _nextLiteralID(NoLiteral+1)
    , _nextLiteralDictionaryID(0)
    , _nextLocationID(NoLocation+1)
    , _nextOperationID(NoOperation+1)
    , _nextSymbolDictionaryID(0)
    , _nextTransformationID(NoTransformation+1)
    , _nextValueID(NoValue+1)
    , _ilBuilt(false) {

    if (_config == NULL) {
        _config = compiler->config();
        _myConfig = false;
    }
}

Compilation::~Compilation() {
    if (_myConfig && _config != NULL)
        delete _config;
    delete _symbolDict;
    delete _literalDict;
    delete _context;
}

void
Compilation::addInitialBuildersToWorklist(BuilderWorklist & worklist) {
    for (auto it = _builders.begin();it != _builders.end(); it++) {
        Builder *b = *it;
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
   TypeDictionary *td = dict();
   td->write(w);

   SymbolDictionary *sd = symdict();
   sd->write(w);

   LiteralDictionary *ld = litdict();
   ld->write(w);
}

CompilerReturnCode
Compilation::compile(std::string strategy) {
    // debatable but let's call this an error
    return _compiler->CompileFailed;
}

} // namespace JitBuilder
} // namespace OMR

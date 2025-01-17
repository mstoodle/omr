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
#include "CoreExtension.hpp"
#include "Extension.hpp"
#include "IR.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "Scope.hpp"
#include "String.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_ON(Compilation, Compiler)
SUBCLASS_KINDSERVICE_IMPL(Compilation,"Compilation",Extensible,Extensible)

Compilation::Compilation(Allocator *a, Extension *ext, ExtensibleKind kind, CompileUnit *unit, StrategyID strategy, Config *config)
    : Extensible(a, ext, kind)
    , _id(ext->compiler()->getCompilationID())
    , _nextTransformationID(NoTransformation) // must precede anything that might create Transformations
    , _compiler(ext->compiler())
    , _ext(ext)
    , _unit(unit)
    , _strategy(strategy)
    , _myConfig(config == NULL)
    , _config((config == NULL) ? new (_compiler->mem()) Config(_compiler->mem(), _compiler->config()) : config)
    , _mem(_config->compilationAllocator(_compiler->mem()))
    , _ir(NULL)
    , _passMem(NULL)
    , _logger(NULL)
    , _writer(NULL)
    , _builders(NULL, _mem) {

    notifyCreation(KIND(Extensible));

    _string = new (a) String(a, a, "[ compilation C");
    _string->append(String::to_string(a, _id))
            .append(" ]");
}

Compilation::~Compilation() {
    _config->destructCompilationAllocator(_mem);
    if (_myConfig && _config != NULL)
        delete _config;
}

#if 0
void
Compilation::addInitialBuildersToWorklist(BuilderList & worklist) {
    for (auto it = ir()->builders();it.hasItem(); it++) {
        Builder *b = it.item();
        worklist.push_back(b);
    }
}

Literal *
Compilation::registerLiteral(LOCATION, const Type *type, const LiteralBytes *value) {
    return _ir->registerLiteral(PASSLOC, type, value);
}

Builder *
Compilation::registerBuilder(Builder *b) {
    _builders.push_back(b);
    return b;
}
#endif

void
Compilation::log(TextLogger &lgr) {
   ir()->log(this, lgr);
}

bool
Compilation::prepareIL(LOCATION) {
    Allocator *irmem = mem();
    IR *ir = compiler()->irPrototype()->clone(irmem);

    // ownership of the Context and Scope objects are passed to ir during construction
    Context *context = new (irmem) Context(irmem, _compiler->coreExt(), ir, "Compilation Context");
    Scope *scope = new (irmem) Scope(irmem, _compiler->coreExt(), ir, "Compilation Scope");

    if (unit()->buildContext(PASSLOC, this, scope, context) == false)
        return false;

    bool rc = unit()->buildIL(PASSLOC, this, scope, context);

    delete ir;

    return rc;
}

void
Compilation::freeIL(LOCATION) {
    if (_ir != NULL)
        delete _ir;
}

} // namespace JB2
} // namespace OMR

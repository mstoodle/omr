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
#include "IRCloner.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "Scope.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextLogger.hpp"
#include "TextWriter.hpp"
#include "TypeDictionary.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(IR, Compilation)
SUBCLASS_KINDSERVICE_IMPL(IR,"IR",Extensible,Extensible)

IR::IR(Allocator *a, CompileUnit *unit)
    : ExtensibleIR(a, unit->compiler()->coreExt(), this, KIND(Extensible))
    , _id(unit->compiler()->getIRID())
    , _nextBuilderID(NoBuilder+1)
    , _nextContextID(NoContext+1)
    , _nextLiteralID(NoLiteral+1)
    , _nextLocationID(NoLocation+1)
    , _nextOperationID(NoOperation+1)
    , _nextScopeID(NoScope+1)
    , _nextTransformationID(NoTransformation)
    , _nextValueID(NoValue)
    , _unit(unit)
    , _mem(a)
    , _scope(NULL)
    , _context(NULL)
    , _litdict(new (a) LiteralDictionary(a, unit->compiler(), "CompileUnit Literal Dictionary", unit->compiler()->litdict()))
    , _symdict(new (a) SymbolDictionary(a, unit->compiler(), "CompileUnit Symbol Dictionary", unit->compiler()->symdict()))
    , _typedict(new (a) TypeDictionary(a, unit->compiler(), "CompileUnit Type Dictionary", unit->compiler()->typedict()))
    , _builders(NULL, a)
    , _locations(NULL, a) {

    notifyCreation(KIND(Extensible));
}

IR::IR(Allocator *a, const IR *source, IRCloner *cloner)
    : ExtensibleIR(a, source->ext(), this, KIND(Extensible))
    , _nextBuilderID(NoBuilder+1)
    , _nextContextID(NoContext+1)
    , _nextLiteralID(NoLiteral+1)
    , _nextLocationID(NoLocation+1)
    , _nextOperationID(NoOperation+1)
    , _nextScopeID(NoScope+1)
    , _nextTransformationID(NoTransformation)
    , _nextValueID(NoValue)
    , _unit(source->unit())
    , _mem(a)
    , _scope(NULL)
    , _context(NULL)
    , _litdict(NULL)
    , _symdict(NULL)
    , _typedict(NULL)
    , _builders(NULL, a)
    , _locations(NULL, a) {

    // make sure no IR is created before this is done
    cloner->setClonedIR(this);

    // TypeDictionary comes first because everyone references Types
    _typedict = cloner->clonedTypeDictionary(source->typedict());

    // next basic elements are Literals and Symbols
    _litdict = cloner->clonedLiteralDictionary(source->litdict());
    _symdict = cloner->clonedSymbolDictionary(source->symdict());

    // having cloned Symbol, we can clone the Context
    _context = cloner->clonedContext(source->context<Context>());

    // we can now clone Builders which clones Operations and Values which depend on Literals, Symbols, and Types
    _scope = cloner->clonedScope(source->scope<Scope>());


    // Literals, Symbols, TypeDictionaries, and Values reference Types
    // LiteralDictionaries reference Literals
    // Contexts and SymbolDictionaries reference Symbols
    // Builders and Operations reference each other as well as Literals, Symbols, Types, and Values
    // Scopes reference Builders
    // Cases reference Builders and Literals
    // IR references Builders, Contexts, and Scopes

    notifyCreation(KIND(Extensible));
}

IR::~IR() {
    for (auto it=_builders.fwdIterator();it.hasItem(); it++) {
        Builder *b = it.item();
        delete b;
    }

    delete _typedict;
    delete _symdict;
    delete _litdict;

    // scope and context objects may not be (but probably are) dynamically allocated
    if (_scope->allocator())
        delete _scope;
    if (_context->allocator())
        delete _context;
}

void
IR::addInitialBuildersToWorklist(BuilderList & worklist) {
    _scope->addInitialBuildersToWorklist(worklist);
}

Literal *
IR::registerLiteral(LOCATION, const Type *type, const LiteralBytes *value) {
    return _litdict->registerLiteral(PASSLOC, type, value);
}

void
IR::registerBuilder(Builder *b) {
    _builders.push_back(b);
}

void
IR::registerLocation(Location *location) {
    _locations.push_back(location);
}

bool
IR::prepare(LOCATION, Compilation *comp) {
    if (unit()->buildContext(PASSLOC, comp, _scope, _context) == false)
        return false;

    return unit()->buildIL(PASSLOC, comp, _scope, _context);
}

bool
IR::build(LOCATION, Compilation *comp) {
    return unit()->buildIL(PASSLOC, comp, _scope, _context);
}

IR *
IR::clone(Allocator *mem) const {
    IRCloner cloner(mem, compiler()->coreExt(), KIND(Extensible));
    return new (mem) IR(mem, this, &cloner);
}

void
IR::log(Compilation *comp, TextLogger &lgr) const {
   lgr.irSectionBegin("ir", "ir", id(), kind(), unit()->createLoc()->functionName(mem()));
   unit()->log(lgr);
   typedict()->log(lgr);
   symdict()->log(lgr);
   litdict()->log(lgr);

   _context->log(lgr);
   _scope->log(lgr);

   //TextWriter *wrtr = new (mem()) TextWriter(mem(), compiler(), lgr);
   //wrtr->print(comp);

   lgr.irSectionEnd();
}

} // namespace JitBuilder
} // namespace OMR

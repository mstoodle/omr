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
#include "Extension.hpp"
#include "Operation.hpp"
#include "TextWriter.hpp"
#include "TypeDictionary.hpp"
#include "TypeReplacer.hpp"
#include "Value.hpp"


namespace OMR {
namespace JitBuilder {

CompileUnit::CompileUnit(LOCATION, Compiler *compiler, String name)
    : _id(compiler->getCompileUnitID())
    , _createLocation(PASSLOC)
    , _name(name)
    , _compiler(compiler)
    , _outerUnit(NULL) {
    //, _myContext(context == NULL)
    //, _context(_myContext ? new Context(compiler) : context)
    //, _numEntryPoints(context->numEntryPoints())
    //, _nativeEntryPoints(new void *[_numEntryPoints])
    //, _debugEntryPoints(new void *[_numEntryPoints]) {

    //Move to Compilation
    //for (uint32_t e=0;e < _numEntryPoints;e++)
    //    _builderEntryPoints[e] = Builder::create(_comp, _context);
    //for (uint32_t x=0;x < _numExitPoints;x++)
    //    _builderExitPoints[x] = Builder::create(_comp, _context);
    //ext->SourceLocation(PASSLOC, _builderEntryPoints[0], ""); // make sure everything has a location; by default BCIndex is 0
}

CompileUnit::CompileUnit(LOCATION, CompileUnit *outerUnit, String name)
    : _id(_compiler->getCompileUnitID())
    , _createLocation(PASSLOC)
    , _name(name)
    , _compiler(outerUnit->_compiler)
    , _outerUnit(outerUnit) {
    //, _myContext(false)
    //, _context(outerUnit->_context)
    //, _numEntryPoints(_context->numEntryPoints())
    //, _nativeEntryPoints(new void *[_numEntryPoints])
    //, _debugEntryPoints(new void *[_numEntryPoints]) {

    //for (uint32_t e=0;e < _numEntryPoints;e++)
    //    _builderEntryPoints[e] = Builder::create(_comp, _context);
    //for (uint32_t x=0;x < _numExitPoints;x++)
    //    _builderExitPoints[x] = Builder::create(_comp, _context);
    //ext->SourceLocation(PASSLOC, _builderEntryPoints[0], ""); // make sure everything has a location; by default BCIndex is 0
}

CompileUnit::~CompileUnit() {
    //for (auto e=0;e < _numExitPoints;e++)
    //    delete _builderExitPoints[e];
    //for (auto e=0;e < _numEntryPoints;e++)
    //    delete _builderEntryPoints[e];
    //delete[] _builderExitPoints;
    //delete[] _debugEntryPoints;
    //delete[] _nativeEntryPoints;
    //delete[] _builderEntryPoints;
    //if (_myContext)
    //    delete _context;
}

void
CompileUnit::write(TextWriter &w) const {
    w.indent() << "[ " << kindName() << " " << _id << w.endl();
    w.indentIn();

    w.indent() << "[ name " << name() << " ]" << w.endl();
    w.indent() << "[ origin " << _createLocation.to_string() << " ]" << w.endl();

    writeSpecific(w);

    w << "]" << w.endl();

    w.indentOut();
    w.indent() << "]" << w.endl();
}

CompilerReturnCode
CompileUnit::compile(LOCATION, StrategyID strategy, TextWriter *logger) {
    Compilation comp(_compiler, this);
    Context context(LOC, &comp);
    comp.setContext(&context);
    comp.setLogger(logger);

    CompilerReturnCode rc = _compiler->compile(PASSLOC, &comp, strategy);
    if (rc != _compiler->CompileSuccessful) {
        return rc;
    }

    CompiledBody *body = new CompiledBody(this, &context, strategy);
    auto it = _bodies.find(strategy);
    if (it != _bodies.end()) {
        notifyRecompile(it->second, body);
        _bodies.erase(it);
    }
    _bodies.insert({strategy, body});

    return rc;
}

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

} // namespace JitBuilder
} // namespace OMR


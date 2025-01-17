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

#ifndef COMPILEUNIT_INCL
#define COMPILEUNIT_INCL

#include <map>
#include "common.hpp"
#include "CreateLoc.hpp"
#include "ExtensibleIR.hpp"


namespace OMR {
namespace JB2 {

class Compilation;
class CompiledBody;
class Compiler;
class Config;
class Context;
class Debugger;
class IR;
class Scope;
class Symbol;
class TextLogger;
class TypeDictionary;

class CompileUnit : public Extensible {
    JBALLOC_(CompileUnit)

    friend class Compilation;
    friend class IR;

public:
    // There are intentionally no public constructors. CompileUnit is meant to be subclassed.

    CompileUnitID id() const { return _id; }
    virtual String kindName() const { return "CompileUnit"; }

    Compiler *compiler() const { return _compiler; }
    const CreateLocation *createLoc() const { return &_createLocation; }

    const String & name() const { return _name; }

    CompileUnit *outerUnit() const { return _outerUnit; }

    CompiledBody *compiledBody(StrategyID strategy=NoStrategy) const;
    void saveCompiledBody(CompiledBody *body, StrategyID strategy=NoStrategy);

    #if 0
    virtual CompilerReturnCode compile(LOCATION, StrategyID strategy=NoStrategy, TextLogger *lgr=NULL);
    #endif

    void log(TextLogger & lgr) const;

    virtual void notifyRecompile(CompiledBody *oldBody, CompiledBody *newBody) { }

    virtual Builder *EntryBuilder(LOCATION, IR *ir, Scope *scope);
    //void addLocation(Location *loc ) { _locations.push_back(loc); }

protected:
    DYNAMIC_ALLOC_ONLY(CompileUnit, LOCATION, Compiler *compiler, ExtensibleKind kind, String name="");
    DYNAMIC_ALLOC_ONLY(CompileUnit, LOCATION, CompileUnit *outerUnit, ExtensibleKind kind, String name="");

    virtual void logContents(TextLogger & lgr) const { }

    // Next two are the public API for user sub classes
    virtual bool buildContext(LOCATION, Compilation *comp, Scope *scope, Context *ctx) { return true; }
    virtual bool buildIL(LOCATION, Compilation *comp, Scope *scope, Context *ctx) { return true; }

    Compiler                            * _compiler;
    CompileUnitID                         _id; // must come after _compiler
    CreateLocation                        _createLocation;
    String                                _name;
    CompileUnit                         * _outerUnit;
    std::map<StrategyID,CompiledBody *>   _bodies;

    //List<Location *>        _locations;
    SUBCLASS_KINDSERVICE_DECL(Extensible,CompileUnit);
};

} // namespace JB2
} // namespace OMR

#endif // defined(COMPILEUNIT_INCL)

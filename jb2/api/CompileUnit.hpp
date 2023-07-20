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

#define TOSTR(x) #x
#define LINETOSTR(x) TOSTR(x)

namespace OMR {
namespace JitBuilder {

class Compilation;
class CompiledBody;
class Compiler;
class Config;
class Debugger;
class Scope;
class Symbol;
class TextLogger;
class TypeDictionary;

class CompileUnit : public Allocatable {
    JBALLOC_(CompileUnit)

    friend class Compilation;

public:
    // There are intentionally no public constructors. CompileUnit is meant to be subclassed.

    CompileUnitID id() const { return _id; }
    virtual String kindName() const { return "CompileUnit"; }

    Compiler *compiler() const { return _compiler; }
    const CreateLocation *createLoc() const { return &_createLocation; }

    String name() const { return _name; }

    CompileUnit *outerUnit() const { return _outerUnit; }

    CompiledBody *compiledBody(StrategyID strategy=NoStrategy) const;
    void saveCompiledBody(CompiledBody *body, StrategyID strategy=NoStrategy);

    #if 0
    virtual CompilerReturnCode compile(LOCATION, StrategyID strategy=NoStrategy, TextLogger *lgr=NULL);
    #endif

    void log(TextLogger & lgr) const;

    virtual void notifyRecompile(CompiledBody *oldBody, CompiledBody *newBody) { }

    virtual Builder *EntryBuilder(LOCATION, Compilation *comp, Scope *scope);
    //void addLocation(Location *loc ) { _locations.push_back(loc); }

protected:
    ALL_ALLOC_ALLOWED(CompileUnit, LOCATION, Compiler *compiler, String name="");
    ALL_ALLOC_ALLOWED(CompileUnit, LOCATION, CompileUnit *outerUnit, String name="");

    virtual void logSpecific(TextLogger & lgr) const { }

    // Next two are the public API for user sub classes
    virtual bool buildContext(LOCATION, Compilation *comp, Scope *scope, Context *ctx) { return true; }
    virtual bool buildIL(LOCATION, Compilation *comp, Scope *scope, Context *ctx) { return true; }

    CompileUnitID                         _id;
    CreateLocation                        _createLocation;
    String                                _name;
    Compiler                            * _compiler;
    CompileUnit                         * _outerUnit;
    std::map<StrategyID,CompiledBody *>   _bodies;

    //List<Location *>        _locations;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(COMPILEUNIT_INCL)

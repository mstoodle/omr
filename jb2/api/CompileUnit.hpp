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

#include <exception>
#include <string>
#include <map>

#include "CreateLoc.hpp"
#include "IDs.hpp"
#include "Iterator.hpp"
#include "typedefs.hpp"

#define TOSTR(x) #x
#define LINETOSTR(x) TOSTR(x)

namespace OMR {
namespace JitBuilder {

class Compilation;
class CompiledBody;
class Compiler;
class Config;
class Debugger;
class Symbol;
class TextWriter;
class TypeDictionary;

class CompileUnit {
    friend class Compilation;

public:
    virtual ~CompileUnit();

    CompileUnitID id() const { return _id; }
    virtual std::string kindName() const { return "CompileUnit"; }

    Compiler *compiler() const { return _compiler; }
    const CreateLocation *createLoc() const { return &_createLocation; }

    std::string name() const { return _name; }

    CompiledBody *compiledBody(StrategyID strategy=NoStrategy) const;
    void saveCompiledBody(CompiledBody *body, StrategyID strategy=NoStrategy);

    //Context *context() const { return _context; }

    //uint32_t numEntryPoints() const { return _numEntryPoints; }

    //template<typename T>
    //T * nativeEntry(uint32_t e=0) const { assert(e < _numEntryPoints && _nativeEntryPoints[e] != NULL); return reinterpret_cast<T *>(_nativeEntryPoints[e]); }

    //template<typename T>
    //T *debugEntry(uint32_t e=0) const { assert(e < _numEntryPoints && _debugEntryPoints[e] != NULL); return reinterpret_cast<T *>(_debugEntryPoints[e]); }

    virtual CompilerReturnCode compile(LOCATION, StrategyID strategy=NoStrategy, TextWriter *logger=NULL);

    void write(TextWriter & w) const;

    virtual void notifyRecompile(CompiledBody *oldBody, CompiledBody *newBody) { }

    //void addLocation(Location *loc ) { _locations.push_back(loc); }

protected:
    CompileUnit(LOCATION, Compiler *compiler, std::string name=""); // meant to be subclassed
    CompileUnit(LOCATION, CompileUnit *outerUnit, std::string name="");

    virtual void writeSpecific(TextWriter & w) const { }

    // Next two are the public API for user sub classes
    virtual bool initContext(LOCATION, Compilation *comp, Context *context) { return true; }
    virtual bool buildIL(LOCATION, Compilation *comp, Context *context) { return true; }

    CompileUnitID                         _id;
    CreateLocation                        _createLocation;
    std::string                           _name;
    Compiler                            * _compiler;
    CompileUnit                         * _outerUnit;
    std::map<StrategyID,CompiledBody *>   _bodies;

    //bool                    _myContext;
    //Context               * _context;

    //uint32_t                _numEntryPoints;
    //void                 ** _nativeEntryPoints;
    //void                 ** _debugEntryPoints;
    //Debugger              * _debuggerObject;

    //Builder              ** _builderEntryPoints;
    //uint32_t                _numExitPoints;
    //Builder              ** _builderExitPoints;

    //std::vector<Location *> _locations;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(COMPILEUNIT_INCL)

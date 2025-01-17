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

#ifndef COMPILATION_INCL
#define COMPILATION_INCL


#include "common.hpp"
#include "CreateLoc.hpp"
#include "Context.hpp"
#include "EntryPoint.hpp"
#include "Extensible.hpp"
#include "IR.hpp"
#include "Scope.hpp"

namespace OMR {
namespace JB2 {

class Allocator;
class Builder;
class Compiler;
class CompileUnit;
class Config;
class Context;
class CreateLocation;
class IR;
class Literal;
class LiteralDictionary;
class Location;
class Scope;
class Symbol;
class SymbolDictionary;
class TextLogger;
class TextWriter;
class TypeDictionary;
class TypeReplacer;
class Visitor;

KINDSERVICE_CATEGORY(Compilation);

class Compilation : public Extensible {
    JBALLOC_(Compilation)

    friend class Builder;
    friend class CompileUnit;
    friend class Context;
    friend class EntryPoint;
    friend class Extension;
    friend class Literal;
    friend class LiteralDictionary;
    friend class Location;
    friend class Operation;
    friend class Scope;
    friend class Strategy;
    friend class SymbolDictionary;
    friend class Transformer;
    friend class Type;
    friend class Value;
    friend class Visitor;

    public:
    DYNAMIC_ALLOC_ONLY(Compilation, Extension *ext,
                                    ExtensibleKind kind,
                                    CompileUnit *unit,
                                    StrategyID strategy=NoStrategy,
                                    Config *config=NULL);

    CompilationID id() const { return _id; }
    Compiler *compiler() const { return _compiler; }
    Extension *ext() const { return _ext; }
    CompileUnit *unit() const { return _unit; }
    template<class T> T *context() const { return _ir->context<T>(); }
    template<class T> T *scope() const { return _ir->scope<T>(); }
    Config *config() const { return _config; }

    Allocator *mem() const { return _mem; }
    Allocator *passMem() const { return _passMem; }

    IR *ir() const { return _ir; }

    TextLogger * logger(bool enabled=true) const { return enabled ? _logger : NULL; }
    virtual void log(TextLogger &lgr);
    String to_string() const { return *_string; }

    void setWriter(TextWriter * w) { _writer = w; }
    TextWriter * writer(bool enabled=true) const { return enabled ? _writer : NULL; }

    BuilderListIterator builders() { return _ir->builders(); }

    virtual bool prepareIL(LOCATION);
    virtual void freeIL(LOCATION);

    virtual void replaceTypes(TypeReplacer *repl) { }

protected:
    void setLogger(TextLogger * lgr) { _logger = lgr; }
    //virtual void addInitialBuildersToWorklist(BuilderList & worklist);

    TransformationID getTransformationID() { return _nextTransformationID++; }

    void setPassAllocator(Allocator *a) { _passMem = a; }

    CompilationID _id;

    TransformationID _nextTransformationID;

    Compiler *_compiler;
    Extension *_ext;
    CompileUnit *_unit;
    bool _myConfig;
    Config *_config;
    StrategyID _strategy;
    Allocator *_mem;     // Compilation allocator, cannot be NULL
    Allocator *_passMem; // current Pass allocator, may be NULL

    IR *_ir; // has to be after _mem

    TextLogger * _logger;
    TextWriter * _writer;
    String *_string;

    List<Builder *> _builders; // to remove after refactoring

    SUBCLASS_KINDSERVICE_DECL(Extensible,Compilation);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(COMPILATION_INCL)

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
#include "Scope.hpp"

namespace OMR {
namespace JitBuilder {

class Allocator;
class Builder;
class Compiler;
class CompileUnit;
class Config;
class Context;
class CreateLocation;
class JB1MethodBuilder;
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
    template<class T> T *context() const { return _context->refine<T>(); }
    template<class T> T *scope() const { return _scope->refine<T>(); }
    Config *config() const { return _config; }

    Allocator *mem() const { return _mem; }
    Allocator *passMem() const { return _passMem; }

    TypeDictionary *typedict() const { return _typeDict; }
    LiteralDictionary *litdict() const { return _literalDict; }
    SymbolDictionary *symdict() const { return _symbolDict; }

    BuilderID maxBuilderID() const { return _nextBuilderID-1; }
    LiteralID maxLiteralID() { return _nextLiteralID-1; }
    LocationID maxLocationID() const { return _nextLocationID-1; }
    OperationID maxOperationID() const { return _nextOperationID-1; }
    ValueID maxValueID() const { return _nextValueID-1; }

    TextLogger * logger(bool enabled=true) const { return enabled ? _logger : NULL; }
    virtual void log(TextLogger &lgr) const;

    void setWriter(TextWriter * w) { _writer = w; }
    TextWriter * writer(bool enabled=true) const { return enabled ? _writer : NULL; }

    BuilderListIterator builders() { return _builders.iterator(); }

    virtual bool prepareIL(LOCATION);
    virtual void freeIL(LOCATION);

    virtual void constructJB1Function(JB1MethodBuilder *j1mb) { }
    virtual void jbgenProlog(JB1MethodBuilder *j1mb) { }

    virtual void replaceTypes(TypeReplacer *repl) { }

    void rememberNewValue(Value *v) { _lastNewValue = v; }
    void forgetNewValue(Value *v) { if (_lastNewValue == v) _lastNewValue = NULL; }

protected:
    void setContext(Context *context) { _context = context; }
    void setScope(Scope *scope) { _scope = scope; }
    void setLogger(TextLogger * lgr) { _logger = lgr; }
    Builder *registerBuilder(Builder *b);

    virtual void addInitialBuildersToWorklist(BuilderList & worklist);
    Literal *registerLiteral(LOCATION, const Type *type, const LiteralBytes *value);

    BuilderID getBuilderID() { return _nextBuilderID++; }
    ContextID getContextID() { return _nextContextID++; }
    LiteralID getLiteralID() { return _nextLiteralID++; }
    LocationID getLocationID() { return _nextLocationID++; }
    OperationID getOperationID() { return _nextOperationID++; }
    ScopeID getScopeID() { return this->_nextScopeID++; }
    TransformationID getTransformationID() { return _nextTransformationID++; }

    ValueID getValueID() { return _nextValueID++; }

    void setPassAllocator(Allocator *a) { _passMem = a; }

    CompilationID _id;

    BuilderID _nextBuilderID;
    ContextID _nextContextID;
    //CaseID _nextCaseID;
    LiteralID _nextLiteralID;
    LocationID _nextLocationID;
    OperationID _nextOperationID;
    ScopeID _nextScopeID;
    TransformationID _nextTransformationID;
    ValueID _nextValueID;

    Compiler *_compiler;
    Extension *_ext;
    CompileUnit *_unit;
    Context *_context;
    Scope *_scope;
    bool _myConfig;
    Config *_config;
    StrategyID _strategy;
    Allocator *_mem;     // Compilation allocator, cannot be NULL
    Allocator *_passMem; // current Pass allocator, may be NULL

    LiteralDictionary *_literalDict;
    SymbolDictionary *_symbolDict;
    TypeDictionary *_typeDict;

    TextLogger * _logger;
    TextWriter * _writer;

    Value * _lastNewValue;

    List<Builder *> _builders;

    SUBCLASS_KINDSERVICE_DECL(Extensible,Compilation);
};

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(COMPILATION_INCL)

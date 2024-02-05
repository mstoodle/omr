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

#ifndef IR_INCL
#define IR_INCL


#include "common.hpp"
#include "CompileUnit.hpp"
#include "CreateLoc.hpp"
#include "ExtensibleIR.hpp"
#include "Scope.hpp"

namespace OMR {
namespace JitBuilder {

class Allocator;
class Builder;
class Compilation;
class Compiler;
class Context;
class IRCloner;
class Literal;
class LiteralDictionary;
class Location;
class Operation;
class Scope;
class Symbol;
class SymbolDictionary;
class TextLogger;
class Type;
class TypeDictionary;
class Value;

class IR : public ExtensibleIR {
    JBALLOC_(IR)

    friend class Builder;
    friend class Compilation; // to be removed after refactoring
    friend class Context;
    friend class EntryPoint;
    friend class Extension; // maybe to be removed after refactoring
    friend class Literal;
    friend class Location;
    friend class Operation;
    friend class Scope;
    friend class Symbol;
    friend class Type;
    friend class Value;

public:
    DYNAMIC_ALLOC_ONLY(IR, CompileUnit *unit);

    Compiler *compiler() const           { return _unit->compiler(); }
    Allocator *mem() const               { return _mem; }
    CompileUnit *unit() const            { return _unit; }
    template<class T> T *scope() const   { return _scope->refine<T>(); }
    template<class T> T *context() const { return _context->refine<T>(); }

    BuilderListIterator builders()       { return _builders.iterator(); }

    TypeDictionary *typedict() const     { return _typedict; }
    LiteralDictionary *litdict() const   { return _litdict; }
    SymbolDictionary *symdict() const    { return _symdict; }

    BuilderID maxBuilderID() const       { return _nextBuilderID-1; }
    EntryPointID maxContextID() const    { return _nextContextID-1; }
    EntryPointID maxEntryPointID() const { return _nextEntryPointID-1; }
    LiteralID maxLiteralID() const       { return _nextLiteralID-1; }
    LocationID maxLocationID() const     { return _nextLocationID-1; }
    OperationID maxOperationID() const   { return _nextOperationID-1; }
    OperationID maxScopeID() const       { return _nextScopeID-1; }
    ValueID maxValueID() const           { return _nextValueID-1; }

    void addInitialBuildersToWorklist(BuilderList & worklist);

    bool prepare(LOCATION, Compilation *comp);
    bool build(LOCATION, Compilation *comp);

    // Make a copy of this IR using the provided allocator
    IR *clone(Allocator *mem) const;

protected:
    // only used by clone()
    DYNAMIC_ALLOC_ONLY(IR, const IR *source, IRCloner *cloner);

    BuilderID getBuilderID()               { return _nextBuilderID++; }
    ContextID getContextID()               { return _nextContextID++; }
    ContextID getEntryPointID()            { return _nextEntryPointID++; }
    LiteralID getLiteralID()               { return _nextLiteralID++; }
    LocationID getLocationID()             { return _nextLocationID++; }
    OperationID getOperationID()           { return _nextOperationID++; }
    ScopeID getScopeID()                   { return _nextScopeID++; }
    TransformationID getTransformationID() { return _nextTransformationID++; }
    ValueID getValueID()                   { return _nextValueID++; }

    // IR takes ownership of Context / Scope object passed here so must be dynamically allocated by this->_mem
    void setContext(Context *ctx)          { assert(ctx->allocator() == _mem); _context = ctx; }
    void setScope(Scope *scope)            { assert(scope->allocator() == _mem); _scope = scope; }

    // with this call, IR takes ownership of b object
    void registerBuilder(Builder *b);

    // with this call, IR takes ownership of location object
    void registerLocation(Location *location);

    // with this call, the created Literal takes ownership of value memory
    Literal *registerLiteral(LOCATION, const Type *type, const LiteralBytes *value);

    BuilderID _nextBuilderID;
    ContextID _nextContextID;
    ContextID _nextEntryPointID;
    LiteralID _nextLiteralID;
    LocationID _nextLocationID;
    OperationID _nextOperationID;
    ScopeID _nextScopeID;
    TransformationID _nextTransformationID;
    ValueID _nextValueID;

    CompileUnit *_unit;
    Allocator *_mem;
    Scope *_scope;
    Context *_context;

    LiteralDictionary *_litdict;
    SymbolDictionary *_symdict;
    TypeDictionary *_typedict;

    List<Builder *> _builders;
    List<Location *> _locations;

    SUBCLASS_KINDSERVICE_DECL(Extensible,IR);
};

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(IR_INCL)

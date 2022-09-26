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


#include <stdint.h>
#include <string>
#include "CreateLoc.hpp"
#include "IDs.hpp"
#include "Iterator.hpp"
#include "typedefs.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Compiler;
class Config;
class Context;
class CreateLocation;
class JB1MethodBuilder;
class Literal;
class LiteralDictionary;
class Location;
class Symbol;
class SymbolDictionary;
class TextWriter;
class TypeDictionary;
class TypeReplacer;
class Visitor;

class Compilation {
    friend class Builder;
    friend class Literal;
    friend class LiteralDictionary;
    friend class Location;
    friend class Operation;
    friend class SymbolDictionary;
    friend class Type;
    friend class Value;
    friend class Visitor;

    public:
    Compilation(Compiler *compiler, TypeDictionary *dict, Config *localConfig=NULL);
    virtual ~Compilation();

    CompilationID id() const { return _id; }
    Compiler *compiler() const { return _compiler; }
    Config *config() const { return _config; }
    Context *context() const { return _context; }

    TypeDictionary *dict() const { return _typeDict; }
    LiteralDictionary *litdict() const { return _literalDict; }
    SymbolDictionary *symdict() const { return _symbolDict; }

    virtual bool ilBuilt() const { return _ilBuilt; }

    void registerBuilder(Builder *b);

    BuilderID maxBuilderID() const { return _nextBuilderID-1; }
    LiteralID maxLiteralID() { return _nextLiteralID-1; }
    LiteralDictionaryID maxLiteralDictionaryID() const { return _nextLiteralDictionaryID-1; }
    LocationID maxLocationID() const { return _nextLocationID-1; }
    OperationID maxOperationID() const { return _nextOperationID-1; }
    SymbolDictionaryID maxSymbolDictionaryID() const { return _nextSymbolDictionaryID-1; }
    ValueID maxValueID() const { return _nextValueID-1; }

    TransformationID getTransformationID() { return _nextTransformationID++; }

    BuilderIterator buildersBegin() { return BuilderIterator(_builders); }
    BuilderIterator buildersEnd() { return endBuilderIterator; }

    virtual CompilerReturnCode compile(std::string strategy);
    void setLogger(TextWriter * logger) { _logger = logger; }
    TextWriter * logger(bool enabled=true) const { return enabled ? _logger : NULL; }
    virtual void write(TextWriter &w) const;

    virtual bool buildIL() { _ilBuilt=true; return true; }
    virtual void constructJB1Function(JB1MethodBuilder *j1mb) { }
    virtual void jbgenProlog(JB1MethodBuilder *j1mb) { }
    virtual void setNativeEntryPoint(void *entry, int i=0) { }

    virtual void replaceTypes(TypeReplacer *repl) { }

    protected:
    virtual void addInitialBuildersToWorklist(BuilderWorklist & worklist);
    Literal *registerLiteral(LOCATION, const Type *type, const LiteralBytes *value);

    BuilderID getBuilderID() { return _nextBuilderID++; }
    LiteralID getLiteralID() { return _nextLiteralID++; }
    LiteralDictionaryID getLiteralDictionaryID() { return _nextLiteralDictionaryID++; }
    LocationID getLocationID() { return _nextLocationID++; }
    OperationID getOperationID() { return _nextOperationID++; }
    SymbolDictionaryID getSymbolDictionaryID() { return _nextSymbolDictionaryID++; }
    ValueID getValueID() { return _nextValueID++; }

    CompilationID _id;
    Compiler *_compiler;
    Config *_config;
    bool _myConfig;
    Context *_context;

    LiteralDictionary *_literalDict;
    SymbolDictionary *_symbolDict;
    TypeDictionary *_typeDict;

    TextWriter * _logger;

    BuilderID _nextBuilderID;
    //CaseID _nextCaseID;
    LiteralID _nextLiteralID;
    LiteralDictionaryID _nextLiteralDictionaryID;
    LocationID _nextLocationID;
    OperationID _nextOperationID;
    SymbolDictionaryID _nextSymbolDictionaryID;
    TransformationID _nextTransformationID;
    ValueID _nextValueID;

    BuilderVector _builders;

    bool _ilBuilt;

    static CompilationID nextCompilationID;
    static BuilderIterator endBuilderIterator;
    static LiteralIterator endLiteralIterator;
};

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(COMPILATION_INCL)

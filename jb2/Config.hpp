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

#ifndef CONFIG_INCL
#define CONFIG_INCL

#include "Allocatable.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Allocator;
class Compilation;
class Compiler;
class FunctionBuilder;
class Literal;
class Location;
class Operation;
class Pass;
class Strategy;
class Symbol;
class TextLogger;
class Transformation;
class Transformer;
class Type;
class Value;

// Needs method filter concept and construction should decode filter
class Config : public Allocatable {
    JBALLOC_(Config)

    friend class Compiler;
    friend class Compilation;

public:
    ALL_ALLOC_ALLOWED_NOARGS(Config);
    ALL_ALLOC_ALLOWED(Config, Config *parent);

	virtual Config *refine(Compiler *c) { return this; }
	virtual Config *refine(Compilation *comp) { return this; }
	virtual Config *refine(Location *loc) { return this; }
	virtual Config *refine(Pass *p) { return this; }
	virtual Config *refine(Transformation *t) { return this; }
	virtual Config *refine(Operation *op) { return this; }
	virtual Config *refine(Type *t) { return this; }
	virtual Config *refine(Strategy *s) { return this; }
	virtual Config *refine(Symbol *sym) { return this; }
	virtual Config *refine(Literal *lv) { return this; }
	virtual Config *refine(Value *v) { return this; }

    // when true, turn logging on for strategies
    bool traceStrategy() const                                { return _traceStrategy; }
    Config * setTraceStrategy(bool v=true)                    { _traceStrategy = v; return this; }

    // when true, turn logging on in any subclass of Visitor
    bool traceVisitor() const                                 { return _traceVisitor; }
    Config * setTraceVisitor(bool v=true)                     { _traceVisitor = v; return this; }

    // when true, turn logging on when buildIL() is called
    bool traceBuildIL() const                                 { return _traceBuildIL; }
    Config * setTraceBuildIL(bool v=true)                     { _traceBuildIL = v; return this; }

    // when true, turn logging on when CodeGenerator runs
    bool traceCodeGenerator() const                           { return _traceCodeGenerator; }
    Config * setTraceCodeGenerator(bool v=true)               { _traceCodeGenerator = v; return this; }

    // when true, turn on logging for allocations made by the Compilation
    Config * setTraceCompilationAllocations(bool v=true)      { _traceCompilationAllocations = v; return this; }

    // when true, turn on logging for allocations made by the Compiler
    Config * setTraceCompilerAllocations(bool v=true)         { _traceCompilerAllocations = v; return this; }

    // when true, turn logging on when CodeGenerator runs
    bool traceTypeReplacer() const                            { return _traceTypeReplacer; }
    Config * setTraceTypeReplacer(bool v=true)                { _traceTypeReplacer = v; return this; }

    // when true, logs irPototype before logging compilation
    bool tracePrototypeIR() const                             { return _tracePrototypeIR; }
    Config * setTracePrototypeIR(bool v=true)                 { _tracePrototypeIR = v; return this; }

    // when true, turn on tracking for allocations made by the Compilation
    Config * setTrackCompilationAllocations(bool v=true)      { _trackCompilationAllocations = v; return this; }

    // when true, turn on tracking for allocations made by the Compiler
    Config * setTrackCompilerAllocations(bool v=true)         { _trackCompilerAllocations = v; return this; }

    Config * setVerboseErrors(bool v=true)                    { _verboseErrors = v; return this; }
    bool verboseErrors()                                      { return _verboseErrors; }

    // if >= 0, identifies the last transformation to apply
    bool limitLastTransformationIndex() const                 { return _lastTransformationIndex >= 0; }
    TransformationID lastTransformationIndex() const          { return _lastTransformationIndex; }
    Config * setLastTransformationIndex(TransformationID idx) { _lastTransformationIndex = idx; return this; }

    // when true, logging should be enabled
    bool logCompilation(Compilation * comp) const             { return false; } // TODO: match name against _logRegex
    Config * setLogRegex(String regex)                        { _logRegex = regex; return this; }

    TextLogger * logger() const                               { return _logger; }
    Config * setLogger(TextLogger * logger)                   { _logger = logger; return this; }

protected:
    Allocator * allocateAllocators(Allocator *allocator, bool tracker, bool tracer);
    void destructAllocators(Allocator *allocator, bool tracker, bool tracer);

    Allocator * compilerAllocator(Allocator *allocator) { return allocateAllocators(allocator, _trackCompilerAllocations, _traceCompilerAllocations); }
    void destructCompilerAllocator(Allocator *allocator) { destructAllocators(allocator, _trackCompilerAllocations, _traceCompilerAllocations); }

    Allocator * compilationAllocator(Allocator *allocator) { return allocateAllocators(allocator, _trackCompilationAllocations, _traceCompilationAllocations); }
    void destructCompilationAllocator(Allocator *allocator) { destructAllocators(allocator, _trackCompilationAllocations, _traceCompilationAllocations); }

    bool _traceStrategy;
    bool _traceVisitor;
    bool _traceBuildIL;
    bool _traceCodeGenerator;
    bool _traceCompilationAllocations;
    bool _traceCompilerAllocations;
    bool _traceTypeReplacer;
    bool _tracePrototypeIR;

    bool _trackCompilationAllocations;
    bool _trackCompilerAllocations;

    bool _verboseErrors;

    TransformationID _lastTransformationIndex;

    String _logRegex;

    TextLogger * _logger;
};

} // namespace JB2
} // namespace OMR

#endif // defined(CONFIG_INCL)

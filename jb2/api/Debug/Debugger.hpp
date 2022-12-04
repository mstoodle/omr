/*******************************************************************************
 * Copyright (c) 2021, 2021 IBM Corp. and others
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

#ifndef DEBUGGER_INCL
#define DEBUGGER_INCL

#include <list>
#include <map>
#include "CreateLoc.hpp"
#include "IDs.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Compiler;
class InputReader;
class Operation;
class Symbol;
class TextWriter;
class Type;
class Value;

namespace Debug {

class DebugDictionary;
class Debugger;
class DebuggerFrame;
class DebuggerFunction;
class DebuggerThunk;
class DebugValue;
class Function;
class FunctionDebugInfo;

class DebugREPL {
    friend class DebugExtension;
    friend class DebuggerFunction;
    friend class DebuggerThunk;

public:
    DebugREPL(DebugExtension *dbg, Debugger *debugger, InputReader *reader, TextWriter *writer);

    uint64_t time() { return _time; }

    DebugDictionary *getDictionary(Function *func);

    void singleStep();
    void run();
    void call(Function *func);
    void passParameter(ParameterSymbol *sym, DebugValue *value);
    DebugValue *getValue(Value *value);
    DebugValue *getValue(Symbol *symbol);
    void breakBefore(Operation *op);
    void breakBefore(Builder *b);
    void breakAfter(Operation *op);
    void breakAfter(Builder *b);

protected:

    Compiler *compiler() const { return _compiler; }

    virtual void debug(Base::FunctionCompilation *comp, DebugValue *returnValues, DebugValue *locals);
    virtual void debug(Builder *b);
    virtual bool debug(Operation *op);

    void ensureOperationDebugger(Operation * op);

    virtual void printDebugValue(DebugValue *val);
    virtual void printTypeName(const Type *type);
    virtual void printType(const Type *type);
    virtual void printValue(uint64_t idx);
    virtual void printSymbol(std::string name);
    virtual void printHelp();
    virtual void acceptCommands(Operation *op, Operation *nextOp=NULL);
    virtual void showOp(Operation *op, std::string msg);

    virtual void beforeOp(Operation *op, Operation *nextOp);
    virtual void afterOp(Operation *op, Operation *nextOp);

    virtual bool breakBefore(Builder *b);
    virtual bool breakBefore(Operation *op);
    virtual bool breakAfter(Operation *op);

    void setup();

    void recordReentryPoint(Builder *b, Operation * reentryOperation);
    Operation * fetchReentryPoint(Builder *b);
    void removeReentryPoint(Builder *b);

    // static so it can be called by generated DebuggerThunk
    static void debugFunction(Debugger *dbgr, Base::FunctionCompilation *func, DebugValue *returnValues, DebugValue *locals);

    DebugExtension *_dbg;
    Compiler *_compiler;
    Compiler *_dbgCompiler;
    Debugger *_parent;
    TextWriter *_writer;
    InputReader *_reader;
    std::list<std::string> _commandHistory;
    uint64_t _time;
    DebuggerFrame *_frame;
    std::map<CompileUnitID, FunctionDebugInfo *> _functionDebugInfos;
    bool _firstEntry;
};

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // defined(DEBUGGER_INCL)

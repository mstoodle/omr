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

#include <map>
#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

class DebuggerFrame;
class DebuggerFunction;
class DebuggerThunk;
class DebugValue;
class FunctionDebugInfo;

class Debugger {
    friend class DebugExtension;
    friend class DebuggerFunction;
    friend class DebuggerThunk;

public:
    //Debugger(DebugExtension *dbg, Debugger *debugger, InputReader *reader, TextWriter *writer);
    Debugger(Allocator *a, DebugExtension *dbx, InputReader *reader, TextLogger *writer);

    uint64_t time() { return _time; }

    void singleStep();
    void run();
    void call(Func::Function *func);
    void passParameter(Symbol *sym, DebugValue *value);
    DebugValue *getValue(Value *value);
    DebugValue *getValue(Symbol *symbol);
    void breakBefore(Operation *op);
    void breakBefore(Builder *b);
    void breakAfter(Operation *op);
    void breakAfter(Builder *b);

protected:

    Compiler *compiler() const { return _compiler; }

    virtual void debug(Func::FunctionCompilation *comp, DebugValue *returnValues, DebugValue *locals);
    virtual void debug(Builder *b);
    virtual bool debug(Operation *op);

    void ensureOperationDebugger(Operation * op);

    virtual void printDebugValue(DebugValue *val);
    virtual void printTypeName(const Type *type);
    virtual void printType(const Type *type);
    virtual void printValue(ValueID id);
    virtual void printSymbol(String name);
    virtual void printHelp();
    virtual void acceptCommands(Operation *op, Operation *nextOp=NULL);
    virtual void showOp(Operation *op, String msg);

    virtual void beforeOp(Operation *op, Operation *nextOp);
    virtual void afterOp(Operation *op, Operation *nextOp);

    void setup();

    void recordReentryPoint(Builder *b, Operation * reentryOperation);
    Operation * fetchReentryPoint(Builder *b);
    void removeReentryPoint(Builder *b);

    // static so it can be called by generated DebuggerThunk
    static void debugFunction(Debugger *dbgr, Func::FunctionCompilation *func, DebugValue *returnValues, DebugValue *locals);

    Allocator *_mem;
    DebugExtension *_dbx;
    Compiler *_compiler; // still need this?
    Compiler *_dbgCompiler;
    Debugger *_parent;
    TextLogger *_writer;
    InputReader *_reader;
    List<String> _commandHistory;
    uint64_t _time;
    DebuggerFrame *_frame;
    std::map<CompileUnitID, FunctionDebugInfo *> _functionDebugInfos;
    bool _firstEntry;
};

} // namespace Debug
} // namespace JB2
} // namespace OMR

#endif // defined(DEBUGGER_INCL)

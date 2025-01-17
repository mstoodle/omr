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

#include <cstring>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "Sim/Simulator.hpp"

#if 0
#include "Debug/DebugExtension.hpp"
#include "Debug/Debugger.hpp"
#include "Debug/DebuggerFrame.hpp"
#include "Debug/DebuggerFunction.hpp"
#include "Debug/DebuggerThunk.hpp"
#include "Debug/DebugValue.hpp"
#include "Debug/FunctionDebugInfo.hpp"
#include "Debug/OperationDebugger.hpp"
#endif

namespace OMR {
namespace JB2 {
namespace Sim {

//#define TRACE_OPSIMULATORS

Simulator::Simulator(SimExtension *sim)
    : _sim(sim)
    , _simCompiler(sim->compiler())
    , _compiler(new Compiler("Simulator internal compiler", simCompiler->config(), simCompiler))
    , _activeContext(NULL)
    , _time(0) {

}

struct Breakpoint {
    Breakpoint()
        : _id(globalID++)
        , _enabled(true)
        , _silent(false)
        , _count(0) {

    }

    virtual bool breakBefore(Operation *op) { return false; }
    virtual bool breakBefore(Builder *b)    { return false; }
    virtual bool breakAfter(Operation *op)  { return false; }
    virtual bool breakAfter(Builder *b)     { return false; }
    virtual bool breakAt(uint64_t time)     { return false; }
    virtual bool fire() {
        if (_count > 0)
            _count--;
        if (_count == 0)
            _enabled = true;
        return _enabled;
    }
    bool removeAfterFiring()                       { return _removeAfterFiring; }
    bool silent()                                  { return _silent; }

    Breakpoint * setRemoveAfterFiring(bool r=true) { _removeAfterFiring = r; return this;}
    Breakpoint * setIgnoreCount(int64_t c)         { if (c >= 0) _count = c; return this;}
    Breakpoint * setEnabled(bool e=true)           { _enabled = e; return this; }
    Breakpoint * setSilent(bool s=true  )          { _silent = s; return this; }
   
    uint64_t _id;
    bool _enabled;
    bool _removeAfterFiring;
    bool _silent;
    int64_t _count;
    static uint64_t globalID;
};

uint64_t Breakpoint::globalID = 1;

struct InternalBreakpoint : public Breakpoint {
    InternalBreakpoint() : Breakpoint() { }
    virtual void print(TextWriter *w) { }
};

void
Breakpoint::print(TextWriter *writer) {
    *writer << "Breakpoint " << _id;
    if (_enabled)
        *writer << " (enabled): ";
    else
        *writer << " (disabled, ignore count " << _count << "): ";
}

struct BreakpointAtTime : public Breakpoint {
    BreakpointAtTime(uint64_t t)
        : Breakpoint()
        , _time(t) {
      
    }
    virtual bool breakAt(uint64_t time) {
        return time == _time && fire();
    }
    virtual void print(TextWriter *writer) {
        Breakpoint::print(writer);
        *writer << "Stop at time " << _time << writer->endl();
    }

    uint64_t _time;
};

struct BreakpointStepInto : public BreakpointAtTime {
    BreakpointStepInto(uint64_t t)
        : BreakpointAtTime(t) {

    }

    virtual void print(TextWriter *writer) { }
};

struct BreakpointAfterOperation : public Breakpoint {
    BreakpointAfterOperation(uint64_t id)
        : Breakpoint()
        , _opID(id) {

    }
    virtual bool breakAfter(Operation *op) {
        return (op->id() == _opID) && fire();
    }
    virtual void print(TextWriter *writer) {
        Breakpoint::print(writer);
        *writer << "Stop after op" << _opID << writer->endl();
    }
    uint64_t _opID;
};

struct BreakpointBeforeOperation : public Breakpoint {
    BreakpointBeforeOperation(uint64_t id)
        : Breakpoint()
        , _opID(id) {

    }
    virtual bool breakBefore(Operation *op) {
        return (op->id() == _opID) && fire();
    }
    virtual void print(TextWriter *writer) {
        Breakpoint::print(writer);
        *writer << "Stop before op" << _opID << writer->endl();
    }
    uint64_t _opID;
};

struct BreakpointStepOver : public InternalBreakpoint {
    // This one is complicated :( .
    // From the current operation, control could flow:
    //     1) to an unbound builder
    //     2) to a potentially empty builder bound to the current operation
    //     3) to a potentially empty builder bound to some other operation
    //     4) to the next operation in the current builder
    //     5) to the end of of the current bound builder and returning to the parent operation which can then act like any of 1 through 5
    //     6) out of the current function (if current operation is Return)
    //
    // StepOver means to stop at the next executed operation BUT skip any operations executed by a builder object bound to this operation
    // Here is how each scenario is handled:
    //     1) add target builder's first operation to _stopOp list
    //     2) add target builder's bound operation (which is also the current operation) to the _stopOp list
    //     3) add target builder's bound operation to the _stopOp list
    //     4) add next operation to the _stopOp list
    //     5) add this operation's parent builder's bound operation to the _stopOp list
    //     6) do nothing
    // Whichever of the operations 1-5 is encountered first, that will fire this breakpoint (typically also remove the breakpoint)
    //
    BreakpointStepOver(Operation *op, Operation *nextOp)
        : InternalBreakpoint() {
        if (nextOp)
            _stopOps.push_back(nextOp);
        else if (op->parent() != NULL && op->parent()->controlReachesEnd()) {
            assert(op->parent()->isBound());
            _stopOps.push_back(op->parent()->boundToOperation());
        }

        for (BuilderIterator bIt = op->BuildersBegin(); bIt != op->BuildersEnd(); bIt++) {
            Builder *bTgt = *bIt;
            if (bTgt->isBound())
                _stopOps.push_back(bTgt->boundToOperation()); // may be op itself!
            else
                _stopOps.push_back(bTgt->firstOperation());
        }
    }

    virtual bool breakBefore(Operation *op) {
        for (auto opIt = _stopOps.iterator(); opIt.hasItem(); opIt++) {
            Operation *stopOp = opIt.item();
            if (op == stopOp)
                return fire();
        }
        return false;
    }

    List<Operation *> _stopOps;
};

struct BreakpointBeforeBuilder : public Breakpoint {
    BreakpointBeforeBuilder(uint64_t id)
        : Breakpoint()
        , _bID(id) {

    }
    virtual bool breakBefore(Builder *b) {
        return (b->id() == _bID) && fire();
    }
    virtual void print(TextWriter *writer) {
        Breakpoint::print(writer);
        *writer << "Stop before B" << _bID << writer->endl();
    }
    uint64_t _bID;
};

void
Debugger::printHelp() {
    TextWriter &w = *_writer;
    w << "JBDB Command reference" << w.endl();
    w << "   h,  help          display this help summary" << w.endl();
    w << "   l,  list          print the current methodbuilder IL" << w.endl();
    w << "   s,  step          step into the next operation, including operations in bound builders" << w.endl();
    w << "   n,  next          step over the next operation, not including operations in bound builders" << w.endl();
    w << "   c,  cont          continue until the next breakpoint" << w.endl();
    w << "   pv, printvalue    print a value (v#)" << w.endl();
    w << "   pt, printtype     print a type (t#)" << w.endl();
    w << "   p,  print         print a symbol (name)" << w.endl();
    w << "   bl, breaklist     print list of active breakpoints" << w.endl();
    w << "   bb, breakbefore   break before an operation (o#) or builder (B#)" << w.endl();
    w << "   ba, breakafter    break after an operation (o#)" << w.endl();
    w << "   b @#              break at time #" << w.endl();
    w << "   d, debug          debug opcode handler for an operation (o#)" << w.endl();
    w << w.endl();
}

// accept input on what to do next
// op is the operation currently being debugged; it can be NULL e.g. at a breakpoint *after* an operation
// nextOp is the next operation that would sequentially follow this operation in the current builder object;
//     it can be NULL e.g. if the current operation is the last one in the current builder object
void
Debugger::acceptCommands(Operation *op, Operation *nextOp) {
    bool done = false;
    while (!done) {

        fprintf(stderr, "[T=%lu] (jbdb) ", _time);
   
        char *line=NULL;
        if (line = _inputReader.getLine()) {
            if (line[0] == '\n') {
                if (_commandHistory.size() > 0) { // repeat last
                    strcpy(line, _commandHistory.back().c_str());
                } else {
                    continue;
                }
            } else {
                String current(line);
                _commandHistory.push_back(current);
            }

            char *command = strtok(line, " \r\n");
            if (strcmp(command, "h") == 0 || strcmp(command, "help") == 0) {
                printHelp();
            }
            if (strcmp(command, "n") == 0 || strcmp(command, "next") == 0) {
                Breakpoint *brkpt = (new BreakpointStepOver(op, nextOp))->setRemoveAfterFiring();
                _frame->_breakpoints.push_front(brkpt); // put it at the beginning so it will be found quickly and quick to remove
                done = true;
                break;
            }
            if (strcmp(command, "s") == 0 || strcmp(command, "step") == 0) {
                Breakpoint *brkpt = (new BreakpointStepInto(_time+1))->setRemoveAfterFiring();
                _frame->_breakpoints.push_front(brkpt); // put it at the beginning so it will be found quickly and quick to remove
                done = true;
                break;
            } else if (strcmp(command, "c") == 0 || strcmp(command, "cont") == 0 || strcmp(command, "continue") == 0) {
                done = true;
                break;
            } else if (strcmp(command, "pt") == 0 || strcmp(command, "printtype") == 0) {
                char *expr = strtok(NULL, " \r\n");
                uint64_t id = strtoul(expr, NULL, 10);
                if (expr[0] == 't')
                    id = strtoul(expr+1, NULL, 10);
         
                if (id < 1000) { // Type::maxID())
                    printType(_comp->typedict()->LookupType(id));
                } else {
                    *(_writer) << "Unrecognized type: should be t# (max id:" /*<< Type::maxID()*/ << ")" << _writer->endl();
                }
            } else if (strcmp(command, "pv") == 0 || strcmp(command, "printvalue") == 0) {
                char *expr = strtok(NULL, " \r\n");
                uint64_t id = strtoul(expr, NULL, 10);
                if (expr[0] == 'v')
                    id = strtoul(expr+1, NULL, 10);

                if (id < 1000) { // Value::maxID())
                    printValue(id);
                } else {
                    *(_writer) << "Unrecognized value: should be v# (max id:" /*<< Value::maxID()*/ << ")" << _writer->endl();
                }
            } else if (strcmp(command, "p") == 0 || strcmp(command, "print") == 0) {
                char *expr = strtok(NULL, " \r\n");
                if (expr && _comp->funcContext()->getSymbol(expr)) {
                    printSymbol(expr);
                } else {
                   fprintf(stderr, "Unrecognized symbol name\n");
                }
            } else if (strcmp(command, "l") == 0 || strcmp(command, "list") == 0) {
                _writer->print(op);
            } else if (strcmp(command, "bb") == 0 || strcmp(command, "breakbefore") == 0) {
                char *bp = strtok(NULL, " \r\n");
                if (strncmp(bp, "o", 1) == 0) {
                    uint64_t id = strtoul(bp+1, NULL, 10);
                    Breakpoint *brkpt = new BreakpointBeforeOperation(id);
                    _frame->_breakpoints.push_back(brkpt);
                    *_writer << "Breakpoint " << brkpt->_id << " will stop before operation o" << id << _writer->endl();
                } else if (bp[0] == 'B') {
                    uint64_t id = strtoul(bp+1, NULL, 10);
                    Breakpoint *brkpt = new BreakpointBeforeBuilder(id);
                    _frame->_breakpoints.push_back(brkpt);
                    *_writer << "Breakpoint " << brkpt->_id << " will stop before builder b" << id << _writer->endl();
                }
            } else if (strcmp(command, "ba") == 0 || strcmp(command, "breakafter") == 0) {
                char *bp = strtok(NULL, " \r\n");
                if (strncmp(bp, "o", 1) == 0) {
                    uint64_t id = strtoul(bp+1, NULL, 10);
                    Breakpoint *brkpt = new BreakpointAfterOperation(id);
                    _frame->_breakpoints.push_back(brkpt);
                    *_writer << "Breakpoint " << brkpt->_id << " will stop after operation o" << id << _writer->endl();
                }
            } else if (strcmp(command, "bl") == 0 || strcmp(command, "breaklist") == 0) {
                for (auto it=_frame->_breakpoints.iterator(); it.keepGoing(); it++) {
                    Breakpoint *bp = it.current();
                    bp->print(_writer);
                }
            } else if (strcmp(command, "b") == 0) {
                char *bp = strtok(NULL, " \r\n");
                if (bp[0] == '@') {
                    uint64_t time = strtoul(bp+1, NULL, 10);
                    Breakpoint *brkpt = new BreakpointAtTime(time);
                    _frame->_breakpoints.push_back(brkpt);
                    *_writer << "Breakpoint " << brkpt->_id << " will stop at time " << time << _writer->endl();
                }
            } else if (strcmp(command, "d") == 0 || strcmp(command,"debug") == 0) {
                char *opStr = strtok(NULL, " \r\n");
                if (strncmp(opStr, "o", 1) == 0) {
                    uint64_t id = strtoul(opStr+1, NULL, 10);
                    _frame->_info->_debugOperations.insert({id, true});
                    *_writer << "Will debug into operation handler for o" << id << _writer->endl();
                }
            }
        }
    }
}

void
Debugger::showOp(Operation *op, String msg) {
    TextWriter &w = *_writer;
    w << msg;
    w.writeOperation(op);
}

bool
Debugger::breakBefore(Operation *op) {
    for (auto it=_frame->_breakpoints.iterator(); it.keepGoing(); it++) {
        Breakpoint *bp=it.current();
        if (bp->breakBefore(op) || bp->breakAt(_time)) {
            bp->print(_writer);
            if (bp->removeAfterFiring())
                _frame->_breakpoints.erase(it);
            return true;
        }
    }
    return false;
}

bool
Debugger::breakAfter(Operation *op) {
    for (auto it=_frame->_breakpoints.iterator(); it.keepGoing(); it++) {
        Breakpoint *bp=it.current();
        if (bp->breakAfter(op)) {
            bp->print(_writer);
            if (bp->removeAfterFiring())
                _frame->_breakpoints.remove(it);
            return true;
        }
    }
    return false;
}

bool
Debugger::breakBefore(Builder *b) {
    for (auto it=_frame->_breakpoints.iterator(); it.keepGoing(); it++) {
        Breakpoint *bp=it.current();
        if (bp->breakBefore(b)) {
            if (bp->removeAfterFiring())
                _frame->_breakpoints.remove(it);

            if (bp->silent()) {
                Operation * op = b->firstOperation();
                auto entryPoint = _frame->_reentryPoints.find(b->id());
                if (entryPoint != _frame->_reentryPoints.end())
                    op = entryPoint->second;
               
                Breakpoint *newBP = new BreakpointBeforeOperation(op->id());
                newBP->setSilent();
                _frame->_breakpoints.push_front(newBP);

                return false;
            }

            bp->print(_writer);

            return true;
        }
    }
    return false;
}

void
Debugger::beforeOp(Operation *op, Operation *nextOp) {
    if (breakBefore(op)) {
        showOp(op, "Stopped before ");
        acceptCommands(op, nextOp);
    }
}

void
Debugger::afterOp(Operation *op, Operation *nextOp) {
    if (breakAfter(op)) {
        showOp(op, "Stopped after ");
        acceptCommands(op, nextOp);
    }
}

DebugDictionary *
Debugger::getDictionary(Base::Function *func) {
    FunctionDebugInfo *info = _functionDebugInfos[func->id()];
    assert(info);
    return &info->_dbgDict;
}

void
Debugger::debug(Base::FunctionCompilation *comp, DebugValue *returnValues, DebugValue *locals) {
    Base::FunctionCompilation *savedComp = _comp;
    DebuggerFrame *savedFrame = _frame;

    Base::Function *func = comp->func();
    Base::FunctionContext *fc = comp->funcContext();
    Builder *entry = fc->builderEntryPoint();

    DebuggerFrame frame;
    frame._debugger = this;
    frame._info = _functionDebugInfos[func->id()];
    size_t valueSizeInBytes = frame._info->_valueSizeInBytes;
    frame._returnValues = returnValues;
    frame._locals = locals;
    frame._values = reinterpret_cast<DebugValue *>(new uint8_t[comp->maxValueID() * valueSizeInBytes]);
    frame._fromBuilder = entry;
    frame._returning = false;
    frame._builderToDebug = entry;
    _frame = &frame;
    _comp = comp;

    if (_firstEntry) {
        TextWriter &w = (*_writer);
        w << "JB2 Debugger (JBDB)" << w.endl();
        w << "Happy debugging!" << w.endl() << w.endl();
        w << "Type h or help for a list of jbdb commands" << w.endl() << w.endl();
        w << "Entering function " << func->name() << " with arguments:" << w.endl();
        for (auto pIt = fc->ParametersBegin(); pIt != fc->ParametersEnd(); pIt++) {
            const ParameterSymbol *param = *pIt;
            w << "    ";
            printSymbol(param->name());
        }
        w << w.endl();
        _firstEntry = false;
    }

    Breakpoint *brkpt = new BreakpointBeforeBuilder(entry->id());
    brkpt->setSilent()
         ->setRemoveAfterFiring();
    _frame->_breakpoints.push_front(brkpt);
 
    _frame->_builderToDebug = entry;
    while (_frame->_builderToDebug) {
        Builder *b = _frame->_builderToDebug;
        //_frame->_builderToDebug = NULL;
        debug(b);
    }

    _comp = savedComp;
    _frame = savedFrame;

    //if (savedFB != fb)
    //   (*_writer) << "Returning from FB" << fb->id() << _writer->endl();
}

void
Debugger::ensureOperationDebugger(Operation * op) {
    FunctionDebugInfo *info = _frame->_info;

    auto opDbgrs = info->_operationDebuggers;
    auto debugger = opDbgrs.find(op->id());
    if (debugger != opDbgrs.end())
        return;

    OperationDebugger *opDebugFunc = new OperationDebugger(LOC, this, _comp, op);
    // need to compile here!
    //int32_t rc = opDebugFunc->Construct();

#ifdef TRACE_OPSIMULATORS
    std::cout.setf(std::ios_base::skipws);
    OMR::JB2::TextWriter printer(opDebugFunc, std::cout, String("    "));
    opDebugFunc->setLogger(&printer);
    opDebugFunc->config()
               ->setTraceBuildIL()
               //->setTraceReducer()
               //->setTraceCodeGenerator()
               ;
    std::cerr << "Operation Debugger for " << op << std::endl;
    printer.print();
#endif

    // debug entry doesn't work yet due to Type name conflicts (DebugFrame, DebugValue)
    bool useDebugEntry = (info->_debugOperations.find(op->id()) != info->_debugOperations.end());
    CompiledBody *body = opDebugFunc->compiledBody();
    auto dbgFunc = useDebugEntry ? body->debugEntryPoint<OperationDebuggerFunc>() : body->nativeEntryPoint<OperationDebuggerFunc>();
    info->_operationDebuggers[op->id()] = dbgFunc;
}

void
Debugger::debug(Builder *b) {
    auto op = b->firstOperation();
    bool usingReentryPoint = false;

    auto entryPoint = _frame->_reentryPoints.find(b->id());
    if (entryPoint != _frame->_reentryPoints.end()) {
        op = entryPoint->second;
        usingReentryPoint = true;
    } else if (_frame->_fromBuilder->isBound() && _frame->_returning) {
        // this scenario is exemplified by an AppendBuilder operation referencing a bound Builder object to which control has just been directed
        // When that builder object completes executing, it comes "back" to its parent but the parent was never entered
        // note that it is possible for any Goto to direct to *any* bound builder object of an operation

        // does this still happen?
        bool foundFromBuilder = false;
        while (op != b->lastOperation()) {
            Operation *possibleOwnerOp = op;
            foundFromBuilder = false;
            for (BuilderIterator bIt = possibleOwnerOp->BuildersBegin(); bIt != possibleOwnerOp->BuildersEnd(); bIt++) {
                Builder *bTgt = *bIt;
                if (bTgt == _frame->_fromBuilder) {
                    foundFromBuilder = true;
                    break;
                }
            }

            if (foundFromBuilder)
                break;

            op = op->next();
        }

        // something wrong if we didn't find the fromBuilderTarget
        if (!foundFromBuilder) {
            TextWriter &w = (*_writer);
            uint64_t fromBuilderID = _frame->_fromBuilder->id();

            w << "Internal debugger error:" << w.endl();
            w << "    Control arrived at B" << b->id() << w.endl();
            w << "    From B" << fromBuilderID << w.endl();
            w << "    but no operation has B" << fromBuilderID << " as a bound builder" << w.endl();
            w << "Aborting frame with no way to recover" << w.endl();

            _frame->_fromBuilder = NULL;
            _frame->_builderToDebug = NULL;
            assert(foundFromBuilder);
            return;
        }
    }
    else {
       // first time this builder has been entered
       _frame->_fromBuilder = b;
    }

    if (breakBefore(b)) {
        _writer->print(b);
        acceptCommands(NULL, op);
    }

    _frame->_builderToDebug = NULL;
    _frame->_returning = false;
    while(op != b->lastOperation()) {
        Operation *nextOp = op->next();

        beforeOp(op, nextOp);
        bool suspend = debug(op);
        afterOp(op, nextOp);

        if (suspend) {
            if (_frame->_builderToDebug->isBound() && _frame->_builderToDebug->boundToOperation() == op && !usingReentryPoint)
                recordReentryPoint(b, op);

         return;
         }

        op = nextOp;
    }

    // done with iterator now, so if we got it from the reentry points we can erase it
    if (usingReentryPoint)
        removeReentryPoint(b);

    if (b->isBound()) {
        _frame->_fromBuilder = b;
        _frame->_builderToDebug = b->boundToOperation()->parent();
        _frame->_returning = true;
        //(*_writer) << "Returning from B" << b->id() << " to B" << _builderToDebug->id() << _writer->endl();
        return;
    }

    // shouldn't fall off end of an unbound builder unless it's the end of the function!
    assert(_frame->_builderToDebug == NULL);
}

bool
Debugger::debug(Operation *op) {
    ensureOperationDebugger(op);
    showOp(op, "Executing: ");
    bool suspendBuilder = _frame->_info->_operationDebuggers[op->id()](_frame, _frame->_fromBuilder->id());
    _time++;
    return suspendBuilder;
}

extern "C"
{
#define DEBUGFUNCTION_LINENUMBER LINETOSTR(__LINE__)
void
Debugger::debugFunction(Debugger *dbgr, Base::FunctionCompilation *comp, DebugValue *returnValues, DebugValue *locals) {
    Base::Function *func = comp->func();
    Base::FunctionContext *fc = comp->funcContext();
    TextWriter &w = *(dbgr->_writer);
    w << "Calling " << func->name() << " with debugger" << w.endl();
    dbgr->debug(comp, returnValues, locals);
    w << "Debugger returning from " << func->name() << w.endl();
    if (fc->numReturnTypes() > 0) {
        w << "Returned (";
        dbgr->printDebugValue(returnValues+0);
        for (int i=0;i < fc->numReturnTypes();i++) {
            w << ", ";
            dbgr->printDebugValue(returnValues+i);
        }
        w << ")" << w.endl();
    }
}
}

} // namespace Debug
} // namespace JB2
} // namespace OMR

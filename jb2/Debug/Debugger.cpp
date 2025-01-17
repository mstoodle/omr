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
#include "Debug/DebugExtension.hpp"
#include "Debug/Debugger.hpp"
#include "Debug/DebuggerFrame.hpp"
#include "Debug/DebuggerFunction.hpp"
#include "Debug/DebuggerThunk.hpp"
#include "Debug/DebugValue.hpp"
#include "Debug/FunctionDebugInfo.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

//#define TRACE_DEBUGTHUNK
//#define TRACE_OPDEBUGGERS

Debugger::Debugger(Alllocator *a, DebugExtension *dbx, InputReader *reader, TextLogger *writer)
    : _mem(a)
    , _dbx(dbx)
    , _reader(reader)
    , _writer(writer) {

}

void
Debugger::printDebugValue(DebugValue *val) {
    TextLogger &w = (*_writer);
    if (val->_type)
        val->_type->logValue(w, &(val->_firstValueData));
    else {
        w << "Undefined";
    }
}

void
Debugger::printValue(ValueID id) {
    TextLogger &w = (*_writer);
    DebugValue *value = _frame->getValue(id);
    w << "v" << id << ": ";
    printDebugValue(value);
    w << " ]" << w.endl();
}

void
Debugger::printTypeName(const Type *type) {
    TextLogger &w = (*_writer);
    w << "t" << type->id() << " : [ " << type->name() << " ]";
}

void
Debugger::printType(const Type * type) {
    TextLogger &w = (*_writer);
    printTypeName(type);
    w << w.endl();
}

void
Debugger::printSymbol(SymbolID sym) {
    Symbol *sym = _frame->compToDebug()->context<Func::FunctionContext>()->getSymbol(expr)) {
    DebugValue *val = _frame->getLocal(id);
    w << sym->name() << " : ";
    printDebugValue(val);
    w << w.endl();
}


void
Debugger::printHelp() {
    TextLogger &w = *_writer;
    w << "JBDB Command reference" << w.endl();
    w << "   h,  help          display this help summary" << w.endl();
    w << "   l,  list          print the current IL" << w.endl();
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
    //w << "   d, debug          debug opcode handler for an operation (o#)" << w.endl();
    w << w.endl();
}

// accept input on what to do next
// op is the operation currently being debugged; it can be NULL e.g. at a breakpoint *after* an operation
// nextOp is the next operation that would sequentially follow this operation in the current builder object;
//     it can be NULL e.g. if the current operation is the last one in the current builder object
static const char *SEP=" \n\r";

void
Debugger::acceptCommands(Operation *op, Operation *nextOp) {
    TextLogger &w = *_writer;
    Compilation *comp = op->parent()->comp();

    bool done = false;
    while (!done) {

        w << "[T=" << time() << "] (jbdb) ";
   
        char *line=NULL;
        if (line = _reader->getLine()) {
            if (line[0] == '\n') {
                if (_commandHistory.length() > 0) { // repeat last
                    strcpy(line, _commandHistory.back().c_str());
                } else {
                    continue;
                }
            } else {
                String current(line);
                _commandHistory.push_back(current);
            }

            char *command = strtok(line, SEP);
            if (strcmp(command, "h") == 0 || strcmp(command, "help") == 0) {
                printHelp();
            }
            if (strcmp(command, "n") == 0 || strcmp(command, "next") == 0) {
                Breakpoint *brkpt = (new BreakpointStepOver(op, nextOp))->setRemoveAfterFiring();
                addBreakpoint(brkpt);
                done = true;
                break;
            }
            if (strcmp(command, "s") == 0 || strcmp(command, "step") == 0) {
                Breakpoint *brkpt = (new BreakpointStepInto(_time+1))->setRemoveAfterFiring();
                addBreakpoint(brkpt);
                done = true;
                break;
            } else if (strcmp(command, "c") == 0 || strcmp(command, "cont") == 0 || strcmp(command, "continue") == 0) {
                done = true;
                break;
            } else if (strcmp(command, "pt") == 0 || strcmp(command, "printtype") == 0) {
                char *expr = strtok(NULL, SEP);
                uint64_t id = strtoul(expr, NULL, 10);
                if (expr[0] == 't')
                    id = strtoul(expr+1, NULL, 10);
         
                TypeDictionary *dict = comp->typedict();
                if (id < dict->numTypes()) {
                    printType(dict->LookupType(id));
                } else {
                    w << "Unrecognized type: should be t# (max id:" << dict->numTypes() << ")" << w.endl();
                }
            } else if (strcmp(command, "pv") == 0 || strcmp(command, "printvalue") == 0) {
                char *expr = strtok(NULL, SEP);
                uint64_t id = strtoul(expr, NULL, 10);
                if (expr[0] == 'v')
                    id = strtoul(expr+1, NULL, 10);

                if (id < comp->maxValueID()) {
                    printValue(id);
                } else {
                    w << "Unrecognized value: should be v# (max id:" << comp->maxValueID() << ")" << w.endl();
                }
            } else if (strcmp(command, "p") == 0 || strcmp(command, "print") == 0) {
                char *expr = strtok(NULL, SEP);
                if (expr && comp->context<Func::FunctionContext>()->getSymbol(expr)) {
                    printSymbol(expr);
                } else {
                   w << "Unrecognized symbol name" << w.endl();
                }
            } else if (strcmp(command, "l") == 0 || strcmp(command, "list") == 0) {
                w.logOperation(op);
            #if 0
            } else if (strcmp(command, "bb") == 0 || strcmp(command, "breakbefore") == 0) {
                char *bp = strtok(NULL, SEP);
                if (strncmp(bp, "o", 1) == 0) {
                    uint64_t id = strtoul(bp+1, NULL, 10);
                    Breakpoint *brkpt = new BreakpointBeforeOperation(id);
                    d->addBreakpoint(brkpt);
                    w << "Breakpoint " << brkpt->_id << " will stop before operation o" << id << w.endl();
                } else if (bp[0] == 'B') {
                    uint64_t id = strtoul(bp+1, NULL, 10);
                    Breakpoint *brkpt = new BreakpointBeforeBuilder(id);
                    d->addBreakpoint(brkpt);
                    w << "Breakpoint " << brkpt->_id << " will stop before builder b" << id << w.endl();
                }
            } else if (strcmp(command, "ba") == 0 || strcmp(command, "breakafter") == 0) {
                char *bp = strtok(NULL, SEP);
                if (strncmp(bp, "o", 1) == 0) {
                    uint64_t id = strtoul(bp+1, NULL, 10);
                    Breakpoint *brkpt = new BreakpointAfterOperation(id);
                    d->addBreakpoint(brkpt);
                    w << "Breakpoint " << brkpt->_id << " will stop after operation o" << id << w.endl();
                }
            } else if (strcmp(command, "bl") == 0 || strcmp(command, "breaklist") == 0) {
                for (auto it=d->breakpoints().begin(); it != d->breakpoints().end(); it++) {
                    Breakpoint *bp = *it;
                    bp->print(_writer);
                }
            } else if (strcmp(command, "b") == 0) {
                char *bp = strtok(NULL, SEP);
                if (bp[0] == '@') {
                    uint64_t time = strtoul(bp+1, NULL, 10);
                    Breakpoint *brkpt = new BreakpointAtTime(time);
                    d->addBreakpoint(brkpt);
                    *_writer << "Breakpoint " << brkpt->_id << " will stop at time " << time << _writer->endl();
                }
            } else if (strcmp(command, "d") == 0 || strcmp(command,"debug") == 0) {
                char *opStr = strtok(NULL, SEP);
                if (strncmp(opStr, "o", 1) == 0) {
                    uint64_t id = strtoul(opStr+1, NULL, 10);
                    _frame->_info->_debugOperations.insert({id, true});
                    *_writer << "Will debug into operation handler for o" << id << _writer->endl();
                }
            #endif
            }
        }
    }
}

void
Debugger::showOp(Operation *op, String msg) {
    TextLogger &w = *_writer;
    w << msg;
    w.logOperation(op);
}

bool
Debugger::breakBefore(Operation *op) {
    for (auto it=breakpoints().begin(); it != breakpoints().end(); it++) {
        Breakpoint *bp=*it;
        if (bp->breakBefore(op) || bp->breakAt(_time)) {
            bp->print(_writer);
            if (bp->removeAfterFiring())
                breakpoints().erase(it);
            return true;
        }
    }
    return false;
}

bool
Debugger::breakAfter(Operation *op) {
    for (auto it=breakpoints().begin(); it != breakpoints().end(); it++) {
        Breakpoint *bp=*it;
        if (bp->breakAfter(op)) {
            bp->print(_writer);
            if (bp->removeAfterFiring())
                breakpoints().erase(it);
            return true;
        }
    }
    return false;
}

bool
Debugger::breakBefore(Builder *b) {
    for (auto it=breakpoints().begin(); it != breakpoints().end(); it++) {
        Breakpoint *bp=*it;
        if (bp->breakBefore(b)) {
            if (bp->removeAfterFiring())
                breakpoints().erase(it);

            if (bp->silent()) {
                Operation * op = b->firstOperation();
                auto entryPoint = _frame->_reentryPoints.find(b->id());
                if (entryPoint != _frame->_reentryPoints.end())
                    op = entryPoint->second;
               
                Breakpoint *newBP = new BreakpointBeforeOperation(op->id());
                newBP->setSilent();
                breakpoints().push_front(newBP);

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

void
Debugger::debug(Func::FunctionCompilation *comp, DebugValue *returnValues, DebugValue *locals) {
    Func::FunctionCompilation *savedComp = _compToDebug;
    DebuggerFrame *savedFrame = _frame;

    Func::Function *func = comp->unit()->refine<Func::Function>();
    Func::FunctionContext *fc = comp->context<Func::FunctionContext>();
    Builder *entry = comp->scope<Func::FunctionScope>()->entryPoint<BuilderEntry>()->builder();

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
    _compToDebug = comp;

    if (_firstEntry) {
        TextLogger &w = (*_writer);
        w << "JB Debugger (JBDB)" << w.endl();
        w << "Happy debugging!" << w.endl() << w.endl();
        w << "Type h or help for a list of jbdb commands" << w.endl() << w.endl();
        w << "Entering function " << func->name() << " with arguments:" << w.endl();
        for (auto pIt = fc->parameters(); pIt.hasItem(); pIt++) {
            const Func::ParameterSymbol *param = pIt.item();
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

    _compToDebug = savedComp;
    _frame = savedFrame;

    //if (savedFB != fb)
    //   (*_writer) << "Returning from FB" << fb->id() << _writer->endl();
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
            for (BuilderIterator bIt = possibleOwnerOp->builders(); bIt.hasItem(); bIt++) {
                Builder *bTgt = bIt.item();
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
            TextLogger &w = (*_writer);
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
Debugger::debugFunction(Debugger *dbgr, Func::FunctionCompilation *comp, DebugValue *returnValues, DebugValue *locals) {
    Func::Function *func = comp->unit()->refine<Func::FunctionCompilation>();
    Func::FunctionContext *fc = comp->context<Func::FunctionContext>();
    TextLogger &w = *(dbgr->_writer);
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

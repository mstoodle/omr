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

#include "Builder.hpp"
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Operation.hpp"
#include "TextWriter.hpp"
#include "Visitor.hpp"

namespace OMR {
namespace JitBuilder {

Visitor::Visitor(Compiler *compiler, std::string name, bool visitAppendedBuilders)
    : Pass(compiler, name)
    , _comp(NULL)
    , _aborted(false)
    , _visitAppendedBuilders(visitAppendedBuilders) {
}

CompilerReturnCode
Visitor::perform(Compilation *comp) {
    start(comp);
    if (_aborted)
        return _compiler->CompileFailed;
    return _compiler->CompileSuccessful;
}

void
Visitor::start(Compilation *comp) {
    _comp = comp;
    _aborted = false;

    visitBegin();

    {
        BuilderWorklist worklist;
        std::vector<bool> visited(_comp->maxBuilderID());
        _comp->addInitialBuildersToWorklist(worklist);

        visitPreCompilation(_comp);

        while(!worklist.empty()) {
            if (_aborted)
                break;
            Builder *b = worklist.back();
            visitBuilder(b, visited, worklist);
            worklist.pop_back();
        }
    }

    visitPostCompilation(_comp);

    visitEnd();

    _aborted = false;
    _comp = NULL;
}

void
Visitor::abort() {
    _aborted = true;
}

void
Visitor::start(Builder * b) {
    BuilderWorklist worklist;
    std::vector<bool> visited(_comp->maxBuilderID());
    visitBuilder(b, visited, worklist);
}

void
Visitor::start(Operation * op) {
    visitOperation(op);
}

void
Visitor::visitBuilder(Builder *b, std::vector<bool> & visited, BuilderWorklist & worklist) {
    int64_t id = b->id();
    if (visited[id])
        return;

    visited[id] = true;

    visitBuilderPreOps(b);
    visitOperations(b, visited, worklist);
    visitBuilderPostOps(b);
}

void
Visitor::visitOperations(Builder *b, std::vector<bool> & visited, BuilderWorklist & worklist) {
    for (OperationIterator opIt = b->OperationsBegin(); opIt != b->OperationsEnd(); opIt++) {
        Operation * op = *opIt;
        visitOperation(op);

        for (BuilderIterator bIt = op->BuildersBegin(); bIt != op->BuildersEnd(); bIt++) {
            Builder * inner_b = *bIt;
            if (inner_b && !visited[inner_b->id()])
                worklist.push_front(inner_b);
        }
    }
}

void
Visitor::trace(std::string msg) {
    TextWriter *log = _comp->logger();
    if (log) {
        log->indent() << msg << log->endl();
    }
}

} // namespace JitBuilder
} // namespace OMR


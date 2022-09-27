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

#include <climits>
#include <sstream>
#include "Builder.hpp"
#include "Compilation.hpp"
#include "Config.hpp"
#include "Operation.hpp"
#include "TextWriter.hpp"
#include "Transformer.hpp"

namespace OMR {
namespace JitBuilder {

void
Transformer::trace(std::string msg) {
    TextWriter *log = _comp->logger(traceEnabled());
    if (log)
        log->indent() << msg << log->endl();
}

bool
Transformer::performTransformation(Operation * op, Builder * transformed, std::string msg) {
    static int64_t lastTransformation = LLONG_MAX;
    int64_t number = _comp->getTransformationID();
    int64_t lastIndex = _comp->config()->lastTransformationIndex();
    bool succeed = (lastIndex < 0 || number < lastIndex);

    if (traceEnabled()) {
        if (succeed) {
            std::ostringstream oss;
            oss << "( " << number << " ) Transformation: " << msg;
            trace(oss.str());
            TextWriter *log= _comp->logger(traceEnabled());
            LOG_INDENT_REGION(log) {
                if (log) log->indentIn();
                if (log) log->print(op);
                if (log) log->indent() << "Replaced with operations from : " << log->endl();
                if (log) log->print(transformed);
                if (log) log->indentOut();
            }
            LOG_OUTDENT
        }
        else
            trace("Transformation not applied: " + msg);
    }

    return succeed;
}

void
Transformer::visitOperations(Builder *b, std::vector<bool> & visited, BuilderWorklist & worklist) {
    TextWriter * log = _comp->logger(traceEnabled());

    // little bit more complicated than usual because we may replace the first operation and have to restart iteration
    for (Operation *op = b->firstOperation(); op != NULL; op = op ? op->next() : b->firstOperation()) {
        if (log) {
            log->indent() << std::string("Visit ");
            log->print(op);
        }

        Builder *transformation = transformOperation(op);
        if (transformation != NULL) {
            if (performTransformation(op, transformation)) {
		// reassign parent for each operation in transformation Builder
		for (Operation *walkOp = op->next(); walkOp->prev() != transformation->lastOperation(); walkOp = walkOp->next()) {
                    walkOp->setParent(b);

                    // scan transformed operations for builder objects we need to traverse
                    for (BuilderIterator bIt = walkOp->BuildersBegin(); bIt != walkOp->BuildersEnd(); bIt++) {
                        Builder *inner_b = *bIt;
                        if (inner_b && !visited[inner_b->id()])
                            worklist.push_front(inner_b);
                    }
                }
		op = op->replace(transformation); // op may now be NULL!
            }
        }
        else {
            for (BuilderIterator bIt = op->BuildersBegin(); bIt != op->BuildersEnd(); bIt++) {
                Builder * inner_b = *bIt;
                if (inner_b)
                    worklist.push_front(inner_b);
            }
        }
    }
}

} // namespace JitBuilder
} // namespace OMR


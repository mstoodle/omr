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
#include "TextLogger.hpp"
#include "TextWriter.hpp"
#include "Transformer.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(Transformer, Passes)
SUBCLASS_KINDSERVICE_IMPL(Transformer,"Transformer",Visitor,Extensible)

Transformer::Transformer(Allocator *a, KINDTYPE(Extensible) kind, Extension *ext, String name)
    : Visitor(a, kind, ext, name)
    , _traceEnabled(false) {

}

Transformer::~Transformer() {

}

void
Transformer::trace(String msg) {
    TextLogger *lgr = _comp->logger(traceEnabled());
    if (lgr)
        lgr->indent() << msg << lgr->endl();
}

bool
Transformer::performTransformation(Operation * op, Builder * transformed, String msg) {
    static int64_t lastTransformation = LLONG_MAX;
    int64_t number = _comp->getTransformationID();
    int64_t lastIndex = _comp->config()->lastTransformationIndex();
    bool succeed = (lastIndex < 0 || number < lastIndex);

    if (traceEnabled()) {
        if (succeed) {
            std::ostringstream oss;
            oss << "( " << number << " ) Transformation: " << msg.c_str();
            trace(oss.str().c_str());
            TextLogger *lgr = _comp->logger(traceEnabled());
            if (lgr) {
                LOG_INDENT_REGION(lgr) {
                    lgr->indentIn();
                    lgr->logOperation(op);
                    lgr->indent() << "Replaced with operations from : " << lgr->endl();
                    for (Operation *logOp = transformed->firstOperation(); logOp != NULL; logOp = logOp->next())
                        lgr->logOperation(logOp);
                    lgr->indentOut();
                }
                LOG_OUTDENT
            }
        }
        else
            trace(String(allocator(), "Transformation not applied: ") + msg.c_str());
    }

    return succeed;
}

void
Transformer::visitOperations(Builder *b, BitVector & visited, BuilderList & worklist) {
    TextWriter * w = _comp->writer(traceEnabled());

    Operation *nextOp = NULL;
    for (Operation *op = b->firstOperation(); op != NULL; op = nextOp) {
        nextOp = op->next(); // may be changed if op is replaced

        if (w) {
            w->logger().indent() << String("Visit ");
            w->print(op);
        }

        Builder *transformation = transformOperation(op);
        if (transformation != NULL && performTransformation(op, transformation)) {
            // scan transformed operations for builder objects we also need to traverse
            for (Operation *scanOp = transformation->firstOperation(); scanOp != transformation->lastOperation(); scanOp = scanOp->next()) {
                for (BuilderIterator bIt = scanOp->builders(); bIt.hasItem(); bIt++) {
                    Builder *inner_b = bIt.item();
                    if (inner_b && !visited.getBit(inner_b->id()))
                        worklist.push_front(inner_b);
                }
            }

            //nextOp = op->replace(transformation); // also visit replaced operations
            op->replace(transformation); // ignore any replaced operations

            // transformation needs to be dismantled now
            delete transformation;
        }
        else {
            for (BuilderIterator bIt = op->builders(); bIt.hasItem(); bIt++) {
                Builder * inner_b = bIt.item();
                if (inner_b)
                    worklist.push_front(inner_b);
            }
        }
    }
}

} // namespace JB2
} // namespace OMR

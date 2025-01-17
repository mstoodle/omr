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

#ifndef TRANSFORMER_INCL
#define TRANSFORMER_INCL

#include "Visitor.hpp"

namespace OMR {
namespace JB2 {

class Transformer : public Visitor {
    JBALLOC_(Transformer)

    public:
    DYNAMIC_ALLOC_ONLY(Transformer, KINDTYPE(Extensible) kind, Extension *ext, String name="Transformer");
 
    Transformer * setTraceEnabled(bool v=true) { _traceEnabled = v; return this; }

protected:
    virtual void visitOperations(Builder *b, BitVector & visited, BuilderList & worklist);

    protected:
    bool traceEnabled() { return _traceEnabled; }

    // To implement any Operation transformation, subclass Transformer
    //   and override the virtual functions below as needed

    // called once on each operation
    // the operation will be replaced by the contents of any non-NULL Builder object returned
    virtual Builder * transformOperation(Operation * op) { return NULL; }

    // logging support: output msg to the log if enabled
    void trace(String msg);

    // logging support: returns true if the transformation is allowed
    //                  and if log enabled, logs details if performed and "not applied" message if not
    bool performTransformation(Operation * op, Builder * transformed, String msg="");

    bool _traceEnabled;

    SUBCLASS_KINDSERVICE_DECL(Extensible, Transformer);
};

} // namespace JB2
} // namespace OMR

#endif // defined(TRANSFORMER_INCL)

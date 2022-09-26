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

#ifndef VISITOR_INCL
#define VISITOR_INCL

#include <string>
#include "Pass.hpp"
#include "typedefs.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Compilation;
class Compiler;
class Extension;
class Operation;

class Visitor : public Pass {
public:
    Visitor(Compiler *compiler, std::string name="", bool visitAppendedBuilders=false);

    virtual CompilerReturnCode perform(Compilation *comp);

    virtual void start(Compilation *comp);
    virtual void start(Builder * b);
    virtual void start(Operation * op);

protected:

    // more dramatic visit patterns can be done by overriding these functions
    virtual void visitBuilder(Builder * b, std::vector<bool> & visited, BuilderWorklist & list);
    virtual void visitOperations(Builder * b, std::vector<bool> & visited, BuilderWorklist & worklist);
    virtual void abort();

    // subclass Visitor and override these functions as needed
    virtual void visitBegin()                             { }
    virtual void visitPreCompilation(Compilation * comp)  { }
    virtual void visitPostCompilation(Compilation * comp) { }
    virtual void visitBuilderPreOps(Builder * b)          { }
    virtual void visitBuilderPostOps(Builder * b)         { }
    virtual void visitOperation(Operation * op)           { }
    virtual void visitEnd()                               { }

    // logging support: output msg to the log if enabled
    void trace(std::string msg);

    Compilation *_comp;
    bool _aborted;
    bool _visitAppendedBuilders;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(VISITOR_INCL)


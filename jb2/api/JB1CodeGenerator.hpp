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

#ifndef JB1CODEGENERATOR_INCL
#define JB1CODEGENERATOR_INCL

#include "Visitor.hpp"


namespace OMR {
namespace JitBuilder {

class Builder;
class Case;
class FunctionBuilder;
class Location;
class Operation;
class PointerType;
class StructType;
class Type;
class Value;

typedef void *TRType;

class JB1CodeGenerator : public Visitor {
public:
    JB1CodeGenerator(Compiler *compiler);

    void * entryPoint() const  { return _entryPoint; }
    int32_t returnCode() const { return _compileReturnCode; }
    JB1MethodBuilder *j1mb() const { return _j1mb; }

    virtual CompilerReturnCode perform(Compilation *comp);

protected:
    virtual void visitPreCompilation(Compilation * comp);
    virtual void visitBuilderPreOps(Builder * b);
    virtual void visitBuilderPostOps(Builder * b);
    virtual void visitOperation(Operation * op);
    virtual void visitPostCompilation(Compilation *comp);

    void generateFunctionAPI(Compilation *comp);

    JB1MethodBuilder *_j1mb;
    void *  _entryPoint;
    int32_t _compileReturnCode;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(JB1CODEGENERATOR_INCL)

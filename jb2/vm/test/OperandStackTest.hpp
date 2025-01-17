/*******************************************************************************
 * Copyright (c) 2016, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/


#ifndef OPERANDSTACKTESTS_INCL
#define OPERANDSTACKTESTS_INCL

#include <stddef.h>
#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"
#include "vm/VM.hpp"
#include "jbgen/jbgen.hpp"
//#include "omrgen/omrgen.hpp"

#define STACKVALUETYPE	Int32
#define STACKVALUECTYPE int32_t

//#define STACKVALUETYPE  Base::Int64
//#define STACKVALUECTYPE int64_t

using namespace OMR::JB2;

class OperandStackTestFunction : public VM::VMFunction {
public:
    OperandStackTestFunction(MEM_LOCATION(a), VM::VMExtension *vmx);

    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);

    static void verifyStack(const char *step, int32_t max, int32_t num, ...);
    static bool verifyUntouched(int32_t maxTouched);

    STACKVALUECTYPE **getSPPtr() { return &_realStackTop; }

protected:
    Builder * testStack(Builder *b, bool useEqual);

    CoreExtension *_cx;
    Base::BaseExtension *_bx;
    Func::FunctionExtension *_fx;
    VM::VMExtension *_vmx;

    const Type * _valueType;

    Func::FunctionSymbol *_createStack;
    Func::FunctionSymbol *_moveStack;
    Func::FunctionSymbol *_freeStack;
    Func::FunctionSymbol *_verifyResult0;
    Func::FunctionSymbol *_verifyResult1;
    Func::FunctionSymbol *_verifyResult2;
    Func::FunctionSymbol *_verifyResult3;
    Func::FunctionSymbol *_verifyResult4;
    Func::FunctionSymbol *_verifyResult5;
    Func::FunctionSymbol *_verifyResult6;
    Func::FunctionSymbol *_verifyResult7;
    Func::FunctionSymbol *_verifyResult8;
    Func::FunctionSymbol *_verifyResult9;
    Func::FunctionSymbol *_verifyResult10;
    Func::FunctionSymbol *_verifyResult11;
    Func::FunctionSymbol *_verifyResult12;
    Func::FunctionSymbol *_verifyValuesEqual;
    Func::FunctionSymbol *_modifyTop3Elements;

    static STACKVALUECTYPE * _realStack;
    static STACKVALUECTYPE * _realStackTop;
    static int32_t _realStackSize;

    static void createStack();
    static STACKVALUECTYPE *moveStack();
    static void freeStack();
};

class OperandStackTestUsingStructFunction : public OperandStackTestFunction {
public:
    OperandStackTestUsingStructFunction(MEM_LOCATION(a), VM::VMExtension *vmx);
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);

protected:
    const Base::StructType *_threadType;
    const Base::FieldType *_spField;
    Func::ParameterSymbol *_threadParam;
};

#endif // !defined(OPERANDSTACKTESTS_INCL)

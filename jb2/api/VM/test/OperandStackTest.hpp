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
#include "Base/Function.hpp"

#define STACKVALUETYPE	Int32
#define STACKVALUECTYPE int32_t

//#define STACKVALUETYPE  Base::Int64
//#define STACKVALUECTYPE int64_t

namespace OMR {
    namespace JitBuilder {
        class Compiler;

        namespace Base {
            class BytecodeBuilder;
            class StructType;
        }
    }
}

using namespace OMR::JitBuilder;

class OperandStackTestFunction : public Base::Function {
public:
    OperandStackTestFunction(Base::BaseExtension *base, VM::VMExtension *vme);
    virtual bool buildIL();

    static void verifyStack(const char *step, int32_t max, int32_t num, ...);
    static bool verifyUntouched(int32_t maxTouched);

    STACKVALUECTYPE **getSPPtr() { return &_realStackTop; }

protected:
    VM::BytecodeBuilder * testStack(VM::BytecodeBuilder *b, bool useEqual);

    Base::BaseExtension *_base;
    VM::VMExtension *_vme;

    const Type * _valueType;

    Base::FunctionSymbol *_createStack;
    Base::FunctionSymbol *_moveStack;
    Base::FunctionSymbol *_freeStack;
    Base::FunctionSymbol *_verifyResult0;
    Base::FunctionSymbol *_verifyResult1;
    Base::FunctionSymbol *_verifyResult2;
    Base::FunctionSymbol *_verifyResult3;
    Base::FunctionSymbol *_verifyResult4;
    Base::FunctionSymbol *_verifyResult5;
    Base::FunctionSymbol *_verifyResult6;
    Base::FunctionSymbol *_verifyResult7;
    Base::FunctionSymbol *_verifyResult8;
    Base::FunctionSymbol *_verifyResult9;
    Base::FunctionSymbol *_verifyResult10;
    Base::FunctionSymbol *_verifyResult11;
    Base::FunctionSymbol *_verifyResult12;
    Base::FunctionSymbol *_verifyValuesEqual;
    Base::FunctionSymbol *_modifyTop3Elements;

    static STACKVALUECTYPE * _realStack;
    static STACKVALUECTYPE * _realStackTop;
    static int32_t _realStackSize;

    static void createStack();
    static STACKVALUECTYPE *moveStack();
    static void freeStack();
};

class OperandStackTestUsingStructFunction : public OperandStackTestFunction {
public:
    OperandStackTestUsingStructFunction(Base::BaseExtension *base, VM::VMExtension *vme);
    virtual bool buildIL();

protected:
    const Base::StructType *_threadType;
    const Base::FieldType *_spField;
    Base::ParameterSymbol *_threadParam;
};

#endif // !defined(OPERANDSTACKTESTS_INCL)

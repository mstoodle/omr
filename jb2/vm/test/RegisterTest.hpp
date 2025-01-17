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


#ifndef VMREGISTER_INCL
#define VMREGISTER_INCL

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"
#include "vm/VM.hpp"
#include "jbgen/jbgen.hpp"
//#include "omrgen/omrgen.hpp"

using namespace OMR::JB2;

class VMRegisterFunction : public Func::Function {
public:
    VMRegisterFunction(MEM_LOCATION(a), Compiler *compiler);
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);

protected:
    CoreExtension *_cx;
    Base::BaseExtension *_bx;
    Func::FunctionExtension *_fx;
    VM::VMExtension *_vmx;

    Func::ParameterSymbol *_values;
    Func::ParameterSymbol *_count;
};

typedef struct VMRegisterStruct {
   int8_t *values;
   int32_t count;
} VMRegisterStruct;

class VMRegisterInStructFunction : public Func::Function {
public:
    VMRegisterInStructFunction(MEM_LOCATION(a), Compiler *compiler);
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx);

    protected:
    CoreExtension *_cx;
    Base::BaseExtension *_bx;
    Func::FunctionExtension *_fx;
    VM::VMExtension *_vmx;

    const Base::FieldType *_valuesField;
    const Base::FieldType *_countField;
    Func::ParameterSymbol *_param;
};

#endif // !defined(VMREGISTER_INCL)

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


#include <assert.h>
#include <dlfcn.h>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "RegisterTest.hpp"

using std::cout;
using std::cerr;

#define TOSTR(x)     #x
#define LINETOSTR(x) TOSTR(x)

#define DO_LOGGING false

int
main(int argc, char *argv[]) {
    cout << "Step 0: load jb2core.so\n";
    void *handle = dlopen("libjb2core.so", RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        return -1;
    }

    cout << "Step 1: create a Compiler\n";
    Compiler c("VirtualMachineRegisterTest");

    cout << "Step 2: load extensions (Base and VM)\n";
    Base::BaseExtension *bx = c.loadExtension<Base::BaseExtension>(LOC);
    VM::VMExtension *vmx = c.loadExtension<VM::VMExtension>(LOC);
    assert(vmx);

    cout << "Step 3: Create Function object\n";
    VMRegisterFunction vmrFunc(LOC, &c);

    cout << "Step 4: Set up logging configuration\n";
    TextLogger logger(std::cout, String("    "));
    TextWriter *writer = c.textWriter(logger);
    TextWriter *wrt = (DO_LOGGING) ? writer : NULL;
    
    cout << "Step 5: compile vmregister function\n";
    CompilerReturnCode result = bx->compile(LOC, &vmrFunc, c.jb1cgStrategyID, wrt);

    if (result != c.CompileSuccessful) {
        cout << "Compile failed: " << result << "\n";
        exit(-1);
    }
    
    cout << "Step 6: invoke compiled vmregister function and print results\n";
    typedef int32_t (VMRegisterMethodFunction)(int8_t **values, int32_t count);
    VMRegisterMethodFunction *vmregister = vmrFunc.compiledBody(c.jb1cgStrategyID)->nativeEntryPoint<VMRegisterMethodFunction>();

    int8_t values[] = {7,2,9,5,3,1,6};
    int8_t *vals = values;
    int32_t retVal = vmregister(&vals, 7);
    cout << "vmregister(values) returned " << retVal << "\n";

    cout << "Step 7: compile vmregisterInStruct function\n";
    VMRegisterInStructFunction vmrisFunc(LOC, &c);
    result = bx->compile(LOC, &vmrisFunc, c.jb1cgStrategyID, wrt); 

    if (result != c.CompileSuccessful) {
        cout << "Compile failed: " << result << "\n";
        exit(-2);
    }
    
    cout << "Step 8: invoke compiled vmregisterInStruct function and print results\n";
    typedef int32_t (VMRegisterInStructCFunction)(VMRegisterStruct *param);
    VMRegisterInStructCFunction *vmregisterInStruct = vmrisFunc.compiledBody(c.jb1cgStrategyID)->nativeEntryPoint<VMRegisterInStructCFunction>();

    VMRegisterStruct param;
    param.count = 7;
    param.values = values;
    retVal = vmregisterInStruct(&param);
    cout << "vmregisterInStruct(values) returned " << retVal << "\n";

    retVal=0;
    for (int32_t i=0;i < 7;i++)
        retVal += values[i];
    cout << "Correct return value should be " << retVal << " in both cases\n";

    cout << "Step 9: allow Compiler object to die (shuts down JIT because it's the last Compiler)\n";
}


VMRegisterFunction::VMRegisterFunction(LOCATION, Compiler *compiler)
    : Func::Function(PASSLOC, compiler)
    , _bx(compiler->lookupExtension<Base::BaseExtension>())
    , _fx(compiler->lookupExtension<Func::FunctionExtension>())
    , _vmx(compiler->lookupExtension<VM::VMExtension>()) {

    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);
    DefineName("vmregister");
}

bool
VMRegisterFunction::initContext(LOCATION, Func::FunctionCompilation *fcomp, Func::FunctionContext *fc) {
    Base::BaseCompilation *comp = static_cast<Base::BaseCompilation *>(fcomp);
    _values = fc->DefineParameter("valuesPtr", _bx->PointerTo(LOC, comp, _bx->PointerTo(LOC, comp, _bx->Int8)));
    _count = fc->DefineParameter("count", _bx->Int32);
    fc->DefineReturnType(_bx->Int32);
    return true;
}

bool
VMRegisterFunction::buildIL(LOCATION, Func::FunctionCompilation *fcomp, Func::FunctionContext *fc) {
    Base::BaseCompilation *comp = static_cast<Base::BaseCompilation *>(fcomp);

    Builder *entry = fc->builderEntryPoint();
    Allocator *mem = fcomp->mem();
    VM::VirtualMachineRegister *vmreg = new (mem) VM::VirtualMachineRegister(MEM_LOC(mem), _vmx, "MYBYTES", comp, _fx->Load(LOC, entry, _values));

    Func::LocalSymbol *result = fc->DefineLocal("result", _bx->Int32);
    _fx->Store(LOC, entry, result, _bx->ConstInt32(LOC, entry, 0));

    Func::LocalSymbol *iterVar = fc->DefineLocal("i", _bx->Int32);
    Base::ForLoopBuilder loop = _bx->ForLoopUp(LOC, entry, iterVar, 
                                               _bx->ConstInt32(LOC, entry, 0),
                                               _fx->Load(LOC, entry, _count),
                                               _bx->ConstInt32(LOC, entry, 1)); {

        Builder *body = loop.loopBody();

        Value *val = _bx->LoadAt(LOC, body, vmreg->Load(LOC, body));

        Value *bumpAmount = _bx->ConvertTo(LOC, body, _bx->Int32, val);
        Value *newval = _bx->Add(LOC, body, _fx->Load(LOC, body, result), bumpAmount);
        _fx->Store(LOC, body, result, newval);
        vmreg->Adjust(LOC, body, 1);
    }

    _fx->Return(LOC, entry, _fx->Load(LOC, entry, result));

    return true;
}


VMRegisterInStructFunction::VMRegisterInStructFunction(LOCATION, Compiler *compiler)
    : Func::Function(PASSLOC, compiler)
    , _bx(compiler->lookupExtension<Base::BaseExtension>())
    , _fx(compiler->lookupExtension<Func::FunctionExtension>())
    , _vmx(compiler->lookupExtension<VM::VMExtension>()) {

    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);
    DefineName("vmregisterInStruct");
}

bool
VMRegisterInStructFunction::initContext(LOCATION, Func::FunctionCompilation *fcomp, Func::FunctionContext *fc) {
    Base::BaseCompilation *comp = static_cast<Base::BaseCompilation *>(fcomp);

    Base::StructTypeBuilder builder(_bx, comp);
    builder.setName("VMRegisterStruct")
           ->addField("values", _bx->PointerTo(LOC, comp, _bx->Int8), 8*offsetof(VMRegisterStruct, values))
           ->addField("count", _bx->Int32, 8*offsetof(VMRegisterStruct, count));
    const Base::StructType *vmRegisterStruct = builder.create(LOC);
    _valuesField = vmRegisterStruct->LookupField("values");
    _countField = vmRegisterStruct->LookupField("count");
    _param = fc->DefineParameter("param", _bx->PointerTo(LOC, comp, vmRegisterStruct));
    fc->DefineReturnType(_bx->Int32);
    return true;
}

bool
VMRegisterInStructFunction::buildIL(LOCATION, Func::FunctionCompilation *fcomp, Func::FunctionContext *fc) {
    Base::BaseCompilation *comp = static_cast<Base::BaseCompilation *>(fcomp);

    Builder *entry = fc->builderEntryPoint();
    Allocator *mem = fcomp->mem();
    VM::VirtualMachineRegisterInStruct *vmreg = new (mem) VM::VirtualMachineRegisterInStruct(MEM_LOC(mem), _vmx, "VALUES", comp, _valuesField, _param);

    Func::LocalSymbol *result = fc->DefineLocal("result", _bx->Int32);
    _fx->Store(LOC, entry, result, _bx->ConstInt32(LOC, entry, 0));

    Func::LocalSymbol *iterVar = fc->DefineLocal("i", _bx->Int32);
    Base::ForLoopBuilder loop = _bx->ForLoopUp(LOC, entry, iterVar, 
                                               _bx->ConstInt32(LOC, entry, 0),
                                               _bx->LoadFieldAt(LOC, entry, _countField, _fx->Load(LOC, entry, _param)),
                                               _bx->ConstInt32(LOC, entry, 1)); {

        Builder *body = loop.loopBody();

        Value *val = _bx->LoadAt(LOC, body, vmreg->Load(LOC, body));

        Value *bumpAmount = _bx->ConvertTo(LOC, body, _bx->Int32, val);
        Value *newval = _bx->Add(LOC, body, _fx->Load(LOC, body, result), bumpAmount);
        _fx->Store(LOC, body, result, newval);
        vmreg->Adjust(LOC, body, 1);
    }

    _fx->Return(LOC, entry, _fx->Load(LOC, entry, result));

    return true;
}

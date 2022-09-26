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


#include <dlfcn.h>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "JBCore.hpp"
#include "Base/Base.hpp"
#include "VM/VM.hpp"
#include "RegisterTest.hpp"

using std::cout;
using std::cerr;

#define TOSTR(x)     #x
#define LINETOSTR(x) TOSTR(x)

#define DO_LOGGING true

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
    Base::BaseExtension *base = c.loadExtension<Base::BaseExtension>();
    assert(base);
    VM::VMExtension *vme = c.loadExtension<VM::VMExtension>();
    assert(vme);

    cout << "Step 3: Create Function object\n";
    VMRegisterFunction vmrFunc(&c);

    cout << "Step 4: Set up logging configuration\n";
    Base::FunctionCompilation *comp = vmrFunc.comp();
    TextWriter logger(comp, std::cout, std::string("    "));
    TextWriter *log = (DO_LOGGING) ? &logger : NULL;
    
    cout << "Step 5: compile vmregister function\n";
    CompilerReturnCode result = vmrFunc.Compile(log);

    if (result != c.CompileSuccessful) {
        cout << "Compile failed: " << result << "\n";
        exit(-1);
    }
    
    cout << "Step 6: invoke compiled vmregister function and print results\n";
    typedef int32_t (VMRegisterMethodFunction)(int8_t **values, int32_t count);
    VMRegisterMethodFunction *vmregister = vmrFunc.nativeEntry<VMRegisterMethodFunction *>();

    int8_t values[] = {7,2,9,5,3,1,6};
    int8_t *vals = values;
    int32_t retVal = vmregister(&vals, 7);
    cout << "vmregister(values) returned " << retVal << "\n";

    cout << "Step 7: compile vmregisterInStruct function\n";
    VMRegisterInStructFunction vmrisFunc(&c);
    result = vmrisFunc.Compile(log); 

    if (result != c.CompileSuccessful) {
        cout << "Compile failed: " << result << "\n";
        exit(-2);
    }
    
    cout << "Step 8: invoke compiled vmregisterInStruct function and print results\n";
    typedef int32_t (VMRegisterInStructFunction)(VMRegisterStruct *param);
    VMRegisterInStructFunction *vmregisterInStruct = vmrisFunc.nativeEntry<VMRegisterInStructFunction *>();

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


VMRegisterFunction::VMRegisterFunction(Compiler *compiler)
    : Base::Function(compiler)
    , _base(compiler->lookupExtension<Base::BaseExtension>())
    , _vme(compiler->lookupExtension<VM::VMExtension>()) {

    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);

    DefineName("vmregister");
    _values = DefineParameter("valuesPtr", _base->PointerTo(LOC, comp(), _base->PointerTo(LOC, comp(), _base->Int8)));
    _count = DefineParameter("count", _base->Int32);
    DefineReturnType(_base->Int32);
}

bool
VMRegisterFunction::buildIL() {
    Builder *entry = builderEntry();
    VM::VirtualMachineRegister *vmreg = new VM::VirtualMachineRegister(LOC, _vme, "MYBYTES", this, _base->Load(LOC, entry, _values));

    Base::LocalSymbol *result = DefineLocal("result", _base->Int32);
    _base->Store(LOC, entry, result, _base->ConstInt32(LOC, entry, 0));

    Base::LocalSymbol *iterVar = DefineLocal("i", _base->Int32);
    Base::ForLoopBuilder *loop = _base->ForLoopUp(LOC, entry, iterVar, 
                                                  _base->ConstInt32(LOC, entry, 0),
                                                  _base->Load(LOC, entry, _count),
                                                  _base->ConstInt32(LOC, entry, 1)); {

        Builder *body = loop->loopBody();

        Value *val = _base->LoadAt(LOC, body, vmreg->Load(LOC, body));

        Value *bumpAmount = _base->ConvertTo(LOC, body, _base->Int32, val);
        Value *newval = _base->Add(LOC, body, _base->Load(LOC, body, result), bumpAmount);
        _base->Store(LOC, body, result, newval);
        vmreg->Adjust(LOC, body, 1);
    }

    _base->Return(LOC, entry, _base->Load(LOC, entry, result));

    return true;
}


VMRegisterInStructFunction::VMRegisterInStructFunction(Compiler *compiler)
    : Base::Function(compiler)
    , _base(compiler->lookupExtension<Base::BaseExtension>())
    , _vme(compiler->lookupExtension<VM::VMExtension>()) {

    DefineLine(LINETOSTR(__LINE__));
    DefineFile(__FILE__);

    DefineName("vmregisterInStruct");
    Base::StructTypeBuilder builder(_base, this);
    builder.setName("VMRegisterStruct")
          ->addField("values", _base->PointerTo(LOC, comp(), _base->Int8), 8*offsetof(VMRegisterStruct, values))
          ->addField("count", _base->Int32, 8*offsetof(VMRegisterStruct, count));
    const Base::StructType *vmRegisterStruct = builder.create(LOC);
    _valuesField = vmRegisterStruct->LookupField("values");
    _countField = vmRegisterStruct->LookupField("count");
    _param = DefineParameter("param", _base->PointerTo(LOC, comp(), vmRegisterStruct));
    DefineReturnType(_base->Int32);
}

bool
VMRegisterInStructFunction::buildIL() {
    Builder *entry = builderEntry();
    VM::VirtualMachineRegisterInStruct *vmreg = new VM::VirtualMachineRegisterInStruct(LOC, _vme, "VALUES", this, _valuesField, _param);

    Base::LocalSymbol *result = DefineLocal("result", _base->Int32);
    _base->Store(LOC, entry, result, _base->ConstInt32(LOC, entry, 0));

    Base::LocalSymbol *iterVar = DefineLocal("i", _base->Int32);
    Base::ForLoopBuilder *loop = _base->ForLoopUp(LOC, entry, iterVar, 
                                                  _base->ConstInt32(LOC, entry, 0),
                                                  _base->LoadFieldAt(LOC, entry, _countField, _base->Load(LOC, entry, _param)),
                                                  _base->ConstInt32(LOC, entry, 1)); {

        Builder *body = loop->loopBody();

        Value *val = _base->LoadAt(LOC, body, vmreg->Load(LOC, body));

        Value *bumpAmount = _base->ConvertTo(LOC, body, _base->Int32, val);
        Value *newval = _base->Add(LOC, body, _base->Load(LOC, body, result), bumpAmount);
        _base->Store(LOC, body, result, newval);
        vmreg->Adjust(LOC, body, 1);
    }

    _base->Return(LOC, entry, _base->Load(LOC, entry, result));

    return true;
}

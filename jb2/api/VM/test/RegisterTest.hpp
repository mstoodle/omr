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

#include "Base/Function.hpp"

namespace OMR {
    namespace JitBuilder {

        class Compiler;

        namespace Base {
            class BaseExtension;
            class LocalSymbol;
            class ParameterSymbol;
        }

        namespace VM {
            class VMExtension;
        }
    }
}

using namespace OMR::JitBuilder;

class VMRegisterFunction : public Base::Function {
public:
    VMRegisterFunction(Compiler *compiler);
    virtual bool buildIL();

protected:
    Base::BaseExtension *_base;
    VM::VMExtension *_vme;

    Base::ParameterSymbol *_values;
    Base::ParameterSymbol *_count;
};

typedef struct VMRegisterStruct {
   int8_t *values;
   int32_t count;
} VMRegisterStruct;

class VMRegisterInStructFunction : public Base::Function {
public:
    VMRegisterInStructFunction(Compiler *compiler);
    virtual bool buildIL();

    protected:
    Base::BaseExtension *_base;
    VM::VMExtension *_vme;
    const Base::FieldType *_valuesField;
    const Base::FieldType *_countField;
    Base::ParameterSymbol *_param;
};

#endif // !defined(VMREGISTER_INCL)

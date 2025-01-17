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

#ifndef DISPATCHER_INCL
#define DISPATCHER_INCL

#include "common.hpp"
#include "Compiler.hpp"
#include "Extensible.hpp"
#include "Pass.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Compilation;
class Extension;

template<class T>
class Dispatcher : public Pass {
    JBALLOC_NO_DESTRUCTOR_(Dispatcher)

protected:
public:
    DYNAMIC_ALLOC_ONLY(Dispatcher, Extension *ext, String name)
        : Pass(a, CLASSKIND(Pass, Extensible), ext, name) {

    }

    CompilerReturnCode perform(Compilation *comp) {
        Compiler *c = compiler();
        CompilerReturnCode success = c->CompileSuccessful;
        for (List<Extensible *>::Iterator it = c->extensibles(CLASSKIND(T, Extensible)); it.hasItem(); it++) {
            Extensible *e = it.item();
            T *p = e->refine<T>();
            CompilerReturnCode rc = p->perform(comp);
            if (rc != success)
                return rc;
        }
        return success;
    }
};

INIT_JBALLOC_TEMPLATE(Dispatcher, Pass::allocCat());

} // namespace JB2
} // namespace OMR

#endif // !defined(DISPATCHER_INCL)

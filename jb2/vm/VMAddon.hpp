/*******************************************************************************
 * Copyright (c) 2024 IBM Corp.
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

#ifndef VMADDONIR_INCL
#define VMADDONIR_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace VM {

class VMExtension;

// This class doesn't have much in it yet, but collects all VM classes that are Addons
class VMAddonIR : public AddonIR {
    JBALLOC_NO_DESTRUCTOR_(VMAddonIR)

public:
    DYNAMIC_ALLOC_ONLY(VMAddonIR, VMExtension *vmx, Extensible *root, KINDTYPE(Extensible)=KIND(Extensible));
    DYNAMIC_ALLOC_ONLY(VMAddonIR, const VMAddonIR *source, IRCloner *cloner);

protected:
    VMExtension *vmx() const;

    SUBCLASS_KINDSERVICE_DECL(Extensible, VMAddonIR);
};

} // namespace VM
} // namespace JB2
} // namespace OMR

#endif // !defined(VMADDONIR_INCL)

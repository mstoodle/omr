/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
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

#ifndef BASEIR_INCL
#define BASEIR_INCL

#include <map>
#include "JBCore.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

class BaseExtension;
class PointerType;
class StructType;

class BaseIRAddon : public AddonIR {
    JBALLOC_NO_DESTRUCTOR_(BaseIRAddon)

    friend class BaseExtension;

public:
    const PointerType * pointerTypeFromBaseType(const Type * baseType);
    void registerPointerType(const PointerType * pType);
    const StructType * structTypeFromName(const String & name);
    void registerStructType(const StructType * sType);

    CaseID getCaseID() { return _nextCaseID++; }

protected:
    BaseIRAddon(Allocator *a, BaseExtension *bx, IR *root);
    BaseIRAddon(Allocator *a, const BaseIRAddon *source, IRCloner *cloner);

    virtual AddonIR *clone(Allocator *a, IRCloner *cloner) const;

    std::map<const Type *,const PointerType *> _pointerTypeFromBaseType;
    std::map<String,const StructType *> _structTypeFromName;

    SUBCLASS_KINDSERVICE_DECL(Extensible, BaseIRAddon);

    CaseID _nextCaseID;
};

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // !defined(BASEIR_INCL)

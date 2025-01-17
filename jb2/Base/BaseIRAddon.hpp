/********************************************************************************
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

#ifndef BASEIRADDON_INCL
#define BASEIRADDON_INCL

#include <map>
#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

class AddressType;
class BaseExtension;
class Float32Type;
class Float64Type;
class Int8Type;
class Int16Type;
class Int32Type;
class Int64Type;
class IntegerType;
class PointerType;
class StructType;

class BaseIRAddon : public AddonIR {
    JBALLOC_NO_DESTRUCTOR_(BaseIRAddon)

    friend class BaseExtension;

public:
    const Int8Type *Int8;
    const Int16Type *Int16;
    const Int32Type *Int32;
    const Int64Type *Int64;
    const Float32Type *Float32;
    const Float64Type *Float64;
    const AddressType *Address;
    const IntegerType *Word;

    const PointerType * pointerTypeFromBaseType(const Type * baseType);
    void registerPointerType(const PointerType * pType);
    const StructType * structTypeFromName(const String & name);
    void registerStructType(const StructType * sType);

    CaseID getCaseID() { return _nextCaseID++; }

protected:
    BaseIRAddon(MEM_LOCATION(a), BaseExtension *bx, IR *root);
    BaseIRAddon(Allocator *a, const BaseIRAddon *source, IRCloner *cloner);

    virtual AddonIR *clone(Allocator *a, IRCloner *cloner) const;

    std::map<const Type *,const PointerType *> _pointerTypeFromBaseType;
    std::map<String,const StructType *> _structTypeFromName;

    SUBCLASS_KINDSERVICE_DECL(Extensible, BaseIRAddon);

    CaseID _nextCaseID;
};

} // namespace Base
} // namespace JB2
} // namespace OMR

#endif // !defined(BASEIRADDON_INCL)

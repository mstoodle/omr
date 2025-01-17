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

#ifndef FUNCTIONIRADDON_INCL
#define FUNCTIONIRADDON_INCL

#include <map>
#include "JBCore.hpp"
#include "Func/FunctionExtension.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

class FunctionTypeBuilder;

class FunctionIRAddon : public AddonIR {
    JBALLOC_NO_DESTRUCTOR_(FunctionIRAddon)

    friend class FunctionExtension;

public:
    const FunctionType * lookupFunctionType(FunctionTypeBuilder & ftb);
    void registerFunctionType(const FunctionType * fType);

    FunctionSymbolIterator functions() const { return this->_functions.iterator(); }
    FunctionSymbolList resetFunctions();
    FunctionSymbol *LookupFunction(String name);

protected:
    FunctionIRAddon(Allocator *a, FunctionExtension *bx, IR *root);
    FunctionIRAddon(Allocator *a, const FunctionIRAddon *source, IRCloner *cloner);

    FunctionExtension *fx() const { return static_cast<FunctionExtension *>(ext()); }

    virtual AddonIR *clone(Allocator *a, IRCloner *cloner) const;

    std::map<String,const FunctionType *> _functionTypesFromName;
    FunctionSymbolList _functions;

    SUBCLASS_KINDSERVICE_DECL(Extensible, FunctionIRAddon);
};

} // namespace Func
} // namespace JB2
} // namespace OMR

#endif // !defined(FUNCTIONIRADDON_INCL)

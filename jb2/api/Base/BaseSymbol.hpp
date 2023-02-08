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

#ifndef BASESYMBOLS_INCL
#define BASESYMBOLS_INCL

#include "JBCore.hpp"
#include "Func/Func.hpp"

namespace OMR {
namespace JitBuilder {
namespace Base {

class FieldType;
class StructType;

class FieldSymbol : public Symbol {
    JBALLOC_(FieldSymbol)

    friend class BaseExtension;
public:
    FieldSymbol(Allocator *a, String name, const StructType *structType, const FieldType *fieldType);

    const StructType *structType() const { return _structType; }
    const FieldType *fieldType() const { return _fieldType; }

    static const SymbolKind getSymbolClassKind();

protected:
    const StructType *_structType;
    const FieldType *_fieldType;

    static SymbolKind SYMBOLKIND;
    static bool kindRegistered;
};

} // namespace Base
} // namespace JitBuilder
} // namespace OMR

#endif // !defined(BASESYMBOLS_INCL)

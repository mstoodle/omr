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

#ifndef LITERALDICTIONARY_INCL
#define LITERALDICTIONARY_INCL


#include "Dictionary.hpp"

namespace OMR {
namespace JB2 {

class CompilerLiteralDictionary;

class LiteralDictionary : public Dictionary<Literal, LiteralID, NoLiteral, LiteralList> {
    typedef Dictionary<Literal, LiteralID, NoLiteral, LiteralList> DictBaseType;
    JBALLOC_(LiteralDictionary)

    friend class IR;
    friend class IRCloner;
    friend class Literal;

  public:
    DYNAMIC_ALLOC_ONLY(LiteralDictionary, IR *ir, String name);

protected:
    LiteralDictionary(Allocator *a, const LiteralDictionary *source, IRCloner *cloner);
    virtual ExtensibleIR *clone(Allocator *mem, IRCloner *cloner) const { return cloneDictionary(mem, cloner); }
    virtual LiteralDictionary *cloneDictionary(Allocator *mem, IRCloner *cloner) const;

    Literal *registerLiteral(LOCATION, const Type *type, const LiteralBytes *value);

    SUBCLASS_KINDSERVICE_DECL(Extensible, LiteralDictionary);
};

} // namespace JB2
} // namespace OMR

#endif // defined(LITERALDICTIONARY_INCL)

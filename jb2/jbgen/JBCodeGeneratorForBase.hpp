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

#ifndef JBCODEGENERATORFORBASE_INCL
#define JBCODEGENERATORFORBASE_INCL

#include <map>
#include "JBCore.hpp"
#include "Base/Base.hpp"

namespace OMR {
namespace JB2 {
namespace jbgen {

class JBCodeGenerator;
class JBMethodBuilder;

class JBCodeGeneratorForBase : public Base::CodeGeneratorForBase {
    JBALLOC_(JBCodeGeneratorForBase)

public:
    DYNAMIC_ALLOC_ONLY(JBCodeGeneratorForBase, JBCodeGenerator *jbcg, Base::BaseExtension *base);

    virtual Builder *gencode(Operation *op);

    virtual bool registerSymbol(Symbol *sym) { return false; }
    virtual bool registerType(const Type *type);

protected:
    Base::BaseExtension *bx() const;
    JBCodeGenerator *jbcg() const;
    JBMethodBuilder *jbmb() const;

    virtual void registerField(const Type *ft, String baseStructName, String fieldName, const Type *fieldType, size_t fieldOffset);

    DEFINE_CG_BASE_HANDLERS(JBCodeGeneratorForBase);

    Base::BaseExtension *_bx;
    DEFINE_CG_BASE_VFT_FIELDS;

    typedef std::map<const Base::FieldType *, String *> FieldMapType;
    std::map<const Base::StructType *, FieldMapType> _structFieldNameMap;

    SUBCLASS_KINDSERVICE_DECL(Extensible,JBCodeGeneratorForBase);
};

} // namespace jbgen
} // namespace JB2
} // namespace OMR

#endif // defined(JBCODEGENERATORFORBASE_INCL)

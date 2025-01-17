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

#ifndef JBCODEGENERATORFORCORE_INCL
#define JBCODEGENERATORFORCORE_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace jbgen {

class JBCodeGenerator;
class JBMethodBuilder;

class JBCodeGeneratorForCore : public CodeGeneratorForCore {
    JBALLOC_(JBCodeGeneratorForCore)

public:
    DYNAMIC_ALLOC_ONLY(JBCodeGeneratorForCore, JBCodeGenerator *jbcg, CoreExtension *cx);

    bool registerType(const Type *t);
    bool registerBuilder(Builder *b);

protected:
    JBCodeGenerator *jbcg() const;
    JBMethodBuilder *jbmb() const;

    DEFINE_CG_CORE_HANDLERS

    SUBCLASS_KINDSERVICE_DECL(Extensible,JBCodeGeneratorForCore);
};

} // namespace jbgen
} // namespace JB2
} // namespace OMR

#endif // defined(JBCODEGENERATORFORCORE_INCL)

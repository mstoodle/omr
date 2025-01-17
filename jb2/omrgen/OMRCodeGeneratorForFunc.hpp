/*******************************************************************************
 * Copyright (c) 2021, 2023 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distr/bution and is available at http://eclipse.org/legal/epl-2.0
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

#ifndef OMRCODEGENERATORFORFUNC_INCL
#define OMRCODEGENERATORFORFUNC_INCL

#include "JBCore.hpp"
#include "Func/Func.hpp"

namespace OMR {
namespace JB2 {
namespace omrgen {

class OMRCodeGenerator;
class OMRIlGen;

class OMRCodeGeneratorForFunc : public Func::CodeGeneratorForFunc {
    JBALLOC_(OMRCodeGeneratorForFunc)

public:
    DYNAMIC_ALLOC_ONLY(OMRCodeGeneratorForFunc, OMRCodeGenerator *omrcg, Func::FunctionExtension *func);

    virtual void setupbody(Compilation *comp);
    virtual void genbody(Compilation *comp);
    virtual Builder *gencode(Operation *op);

    virtual bool registerSymbol(Symbol *sym);
    virtual bool registerType(const Type *type);

protected:
    OMRCodeGenerator *omrcg() const;
    OMRIlGen *ilgen() const;

    DEFINE_CG_FUNC_HANDLERS(OMRCodeGeneratorForFunc);
    DEFINE_CG_FUNC_VFT_FIELDS;

    Func::FunctionExtension *_fx;

    SUBCLASS_KINDSERVICE_DECL(Extensible,OMRCodeGeneratorForFunc);
};

} // namespace omrgen
} // namespace JB2
} // namespace OMR

#endif // defined(OMRCODEGENERATORFORFUNC_INCL)

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

#ifndef CODEGENERATORFORFUNC_INCL
#define CODEGENERATORFORFUNC_INCL

#include "CodeGeneratorForExtension.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

class FunctionExtension;

// Can be used to define VFTs for Base extension Operations
#define DEFINE_CG_FUNC_VFT_FIELDS \
    Array<gencodeFunction> _gencodeVFT

#define INIT_CG_FUNC_VFT_FIELDS(a) \
    _gencodeVFT(NULL, a)

// Can be used to define dispatch handlers for Base extension Operations
#define DEFINE_CG_FUNC_HANDLERS(C) \
    typedef Builder * (C::*gencodeFunction)(Operation *op); \
    Builder *gencodeCall(Operation *op); \
    Builder *gencodeCallVoid(Operation *op); \
    Builder *gencodeLoad(Operation *op); \
    Builder *gencodeReturn(Operation *op); \
    Builder *gencodeReturnVoid(Operation *op); \
    Builder *gencodeStore(Operation *op)

// assign these in reverse order so VFT only has to be grown once (technically only last one has to go first)
#define INIT_CG_FUNC_HANDLERS(C) \
    _gencodeVFT.assign(fx->aStore, &C::gencodeStore); \
    _gencodeVFT.assign(fx->aReturnVoid, &C::gencodeReturnVoid); \
    _gencodeVFT.assign(fx->aReturn, &C::gencodeReturn); \
    _gencodeVFT.assign(fx->aLoad, &C::gencodeLoad); \
    _gencodeVFT.assign(fx->aCallVoid, &C::gencodeCallVoid); \
    _gencodeVFT.assign(fx->aCall, &C::gencodeCall)

#define DEFINE_CG_FUNC_HANDLER_DISPATCH(C) \
    Builder * \
    C::gencode(Operation *op) { \
        ActionID a = op->action(); \
        gencodeFunction f = _gencodeVFT[a]; \
        return (this->*f)(op); \
    }

class CodeGeneratorForFunc : public CodeGeneratorForExtension {
    JBALLOC_(CodeGeneratorForFunc)

public:
    DYNAMIC_ALLOC_ONLY(CodeGeneratorForFunc, CodeGenerator *cg, FunctionExtension *fx);

    virtual Builder *gencode(Operation *op);

protected:
    FunctionExtension *fx() const;

    DEFINE_CG_FUNC_HANDLERS(CodeGeneratorForFunc);

    DEFINE_CG_FUNC_VFT_FIELDS;

    SUBCLASS_KINDSERVICE_DECL(Extensible,CodeGeneratorForFunc);
};

} // namespace Func
} // namespace JB2
} // namespace OMR

#endif // defined(CODEGENERATORFORFUNC_INCL)

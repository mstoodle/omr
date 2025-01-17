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

#ifndef DEBUGCONTEXT_INCL
#define DEBUGCONTEXT_INCL

#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

class DebugExtension;

class DebugContext : public Func::FunctionContext {
    JBALLOC_(DebugContext);

public:
    DebugContext(LOCATION, DebugExtension *dbx, Func::FunctionCompilation *comp, Compilation *compToDebug, String name="");
    DebugContext(LOCATION, DebugExtension *dbx, DebugContext *caller, String name="");

    Compilation *compToDebug() const { return _compToDebug; }

    virtual bool dissolveAtEntry(Operation *op, Builder *b);
    virtual bool dissolveAtExit(Operation *op, Builder *b);

protected:
    DebugExtension *dbx() const { return _dbx; }
    Base::BaseExtension *bx() const { return _bx; }
    Func::FunctionExtension *fx() const { return _fx; }

    void storeValue(LOCATION, Builder *b, Symbol *local, Value *value);
    void storeValue(LOCATION, Builder *b, Value *debugvalue, Value *value);
    void storeReturnValue(LOCATION, Builder *b, int32_t resultIdx, Value *value);
    Value *loadValue(LOCATION, Builder *b, Symbol *local);
    Value *loadValue(LOCATION, Builder *b, Value *value);
    void storeToDebugValue(LOCATION, Builder *b, Value *debugValue, Value *value);
    Value *loadFromDebugValue(LOCATION, Builder *b, Value *debugValue, const Type *type);

    uint64_t index(const Symbol *symbol) const;
    uint64_t index(const Value *value) const;

private:
    DebugExtension *_dbx;
    Base::BaseExtension *_bx;
    Func::FunctionExtension *_fx;
    Compilation *_compToDebug;

    SUBCLASS_KINDSERVICE_DECL(Context, DebugContext);
};

} // namespace Debug
} // namespace JB2
} // namespace OMR

#endif // defined(DEBUGCONTEXT_INCL)

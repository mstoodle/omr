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

#ifndef FUNCTION_INCL
#define FUNCTION_INCL

#include <exception>
#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Func {

class Debugger;
class FunctionCompilation;
class FunctionContext;
class FunctionScope;
class FunctionSymbol;
class LocalSymbol;
class ParameterSymbol;

class Function : public CompileUnit {
    JBALLOC_(Function)

    friend class FunctionCompilation;
    friend class FunctionExtension;

public:
    // no public constructors intentionally: meant to be subclassed

    virtual String kindName() const { return "Function"; }

    void DefineName(String name);
    void DefineFile(String file);
    void DefineLine(String line);

    const String & name() const { return _givenName; }
    const String & fileName() const { return _fileName; }
    const String & lineNumber() const { return _lineNumber; }

protected:
    DYNAMIC_ALLOC_ONLY(Function, LOCATION, Compiler *compiler, String name="");
    DYNAMIC_ALLOC_ONLY(Function, LOCATION, Function *outerFunction, String name="");
    DYNAMIC_ALLOC_ONLY(Function, LOCATION, Compiler *compiler, ExtensibleKind kind, String name="");
    DYNAMIC_ALLOC_ONLY(Function, LOCATION, Function *outerFunction, ExtensibleKind kind, String name="");
    virtual void logContents(TextLogger &lgr) const;


    CoreExtension *cx() { return _cx; }

    // Next two are the API that driving user sub classes of Function
    virtual bool buildContext(LOCATION, FunctionCompilation *comp, FunctionScope *scope, FunctionContext *ctx) { return true; }
    virtual bool buildIL(LOCATION, FunctionCompilation *comp, FunctionScope *scope, FunctionContext *ctx) { return true; }

    static FunctionCompilation *fcomp(Compilation *comp);
    static FunctionScope *fscope(Compilation *comp);
    static FunctionContext *fctx(Compilation *comp);

private:
    CoreExtension * _cx;
    String _givenName;
    String _fileName;
    String _lineNumber;

    SUBCLASS_KINDSERVICE_DECL(Extensible,Function);
};

} // namespace Func
} // namespace JB2
} // namespace OMR

#endif // defined(FUNCTION_INCL)

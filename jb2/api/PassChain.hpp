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

#ifndef PASSCHAIN_INCL
#define PASSCHAIN_INCL

#include <deque>

namespace OMR {
namespace JitBuilder {

class Builder;
class Literal;
class Operation;
class Pass;
class PassExtension;
class Symbol;
class Type;
class Value;

class PassChain
    enum ChainPolicy {
	SameOrder=0;
        ReverseOrder=1;
    };

public:
    PassChain(Pass *pass)
        : _pass(pass) {

    }

    virtual bool processBuilder(Pass *pass, Builder *b);
    virtual bool processLiteral(Pass *pass, Literal *lv);
    virtual bool processOperation(Pass *pass, Operation *op);
    virtual bool processSymbol(Pass *pass, Symbol *sym);
    virtual bool processType(Pass *pass, Type *type);
    virtual bool processValue(Pass *pass, Value *v);

    void addPassExtension(PassExtension *pass, ChainPolicy policy=ReverseOrder);

protected:
    Pass *_pass;
    std::deque<PassExtension *> _chain;
};

} // namespace JitBuilder
} // namespace OMR


#endif // !defined(PASSCHAIN_INCL)

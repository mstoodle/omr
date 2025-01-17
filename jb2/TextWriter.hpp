/*******************************************************************************
 * Copyright (c) 2022, 2021 IBM Corp. and others
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

#ifndef TEXTWRITER_INCL
#define TEXTWRITER_INCL

#include "common.hpp"
#include <iomanip>
#include "TextLogger.hpp"
#include "Visitor.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Builder;
class Compilation;
class Operation;

class TextWriter : public Visitor {
    JBALLOC_(TextWriter)

public:
    DYNAMIC_ALLOC_ONLY(TextWriter, Compiler * compiler, std::ostream & os, String perIndent);
    DYNAMIC_ALLOC_ONLY(TextWriter, Compiler * compiler, TextLogger & logger);

    TextLogger & logger() { return _logger; }

    virtual void start(Compilation *comp);
    void print(Compilation *comp) { start(comp); }
    void print(Builder * b) { this->Visitor::start(b); }
    void print(Operation * op) { this->Visitor::start(op); }

protected:
    TextLogger & _logger;

    virtual void visitPreCompilation(Compilation * comp);
    virtual void visitPostCompilation(Compilation * comp);
    virtual void visitBuilderPreOps(Builder * b);
    virtual void visitBuilderPostOps(Builder * b);
    virtual void visitOperation(Operation * op);

    SUBCLASS_KINDSERVICE_DECL(Extensible, TextWriter);
};

} // namespace JB2
} // namespace OMR

#endif // defined(TEXTWRITER_INCL)

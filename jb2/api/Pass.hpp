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

#ifndef PASS_INCL
#define PASS_INCL

#include "CreateLoc.hpp"
#include "IDs.hpp"
#include "Loggable.hpp"
#include "typedefs.hpp"

namespace OMR {
namespace JitBuilder {

class Compilation;
class Compiler;
class PassChain;

class Pass : public Loggable {
    friend class Extension;
    friend class Strategy;

public:
    Pass(Compiler *compiler, std::string name);

    std::string name() const { return _name; }
    PassID id() const { return _id; }
    PassChain *chain() const { return _chain; }

    virtual CompilerReturnCode perform(Compilation *comp);

protected:
    Compiler *_compiler;
    PassID _id;
    std::string _name;
    PassChain *_chain;
    bool _traceEnabled;
};

} // namespace JitBuilder
} // namespace OMR

#endif // !defined(PASS_INCL)


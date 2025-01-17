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

#ifndef STRATEGY_INCL
#define STRATEGY_INCL

#include "common.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Compilation;
class Compiler;
class Pass;
class TextLogger;

class Strategy : public Allocatable {
    JBALLOC_(Strategy)

    friend class Compiler;

protected:
    // TODO: need some mechanism for managing data during lifetime of strategy (longer than a pass)
    //  e.g. use-def-info, dominators, etc.
    class StrategyData {
        public:
        StrategyData(Compilation *comp);

        protected:
    };

public:
    DYNAMIC_ALLOC_ONLY(Strategy, Compiler *compiler, String name);

    Strategy *addPass(Pass *pass);

    StrategyID id() const { return _id; }
    const String & name() const { return _name; }

    virtual CompilerReturnCode perform(Compilation *comp);
    virtual void allocateData() { }
    
protected:
    TextLogger *lgr() const;

    StrategyID _id;
    Compiler *_compiler;
    String _name;
    Config *_config;
    List<Pass *> _passes;
    StrategyData *_data;
};

} // namespace JB2
} // namespace OMR

#endif // !defined(STRATEGY_INCL)

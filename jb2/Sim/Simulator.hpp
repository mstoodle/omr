/*******************************************************************************
 * Copyright (c) 2021, 2021 IBM Corp. and others
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

#ifndef SIMULATOR_INCL
#define SIMULATOR_INCL

#include <map>
#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace Sim {

class SimDictionary;
class SimExtension;
class SimContext;
class SimValue;
typedef uint64_t SimulatedTime;

class Simulator {
    friend class SimExtension;

public:
    Simulator(SimExtension *sim);

    uint64_t time() { return _time; }

    SimDictionary *getDictionary(Context *context);

    void stepOnce();
    void stepOver();
    void run();
    void enter(SimContext *context, unsigned e=0);
    unsigned leave(SimContext *context);
    void stopBefore(Operation *op);
    void stopBefore(Builder *b);
    void stopAfter(Operation *op);
    void stopAfter(Builder *b);

    LiteralBytes *simulatedValue(Value *value);
    LiteralBytes *simulatedValue(Symbol *symbol);

    static const unsigned NoExit;

protected:

    Compiler *compiler() const { return _compiler; }

    void simulate(Builder *b);
    bool simulate(Operation *op);

    void ensureOperationSimulator(Operation * op);

    void beforeOp(Operation *op);
    void afterOp(Operation *op);

    void setup();

    #if 0
    void recordReentryPoint(Builder *b, Operation * reentryOperation);
    Operation * fetchReentryPoint(Builder *b);
    void removeReentryPoint(Builder *b);
    #endif

    SimExtension *_sim;
    Compiler *_simCompiler;
    Compiler *_compiler;
    SimContext *_activeContext;
    std::map<ContextID, SimContext *> _simContexts;
    SimulatedTime _time;
};

} // namespace Sim
} // namespace JB2
} // namespace OMR

#endif // defined(SIMULATOR_INCL)

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

#include <assert.h>
#include "Debug/DebugExtension.hpp"
#include "Debug/Debugger.hpp"
#include "Debug/DebuggerThunk.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

const SemanticVersion DebugExtension::requiredBaseVersion(REQUIRED_BASEEXT_MAJOR,REQUIRED_BASEEXT_MINOR,REQUIRED_BASEEXT_PATCH);
const SemanticVersion DebugExtension::requiredFuncVersion(REQUIRED_FUNCEXT_MAJOR,REQUIRED_FUNCEXT_MINOR,REQUIRED_FUNCEXT_PATCH);
const SemanticVersion DebugExtension::version(DEBUGEXT_MAJOR,DEBUGEXT_MINOR,DEBUGEXT_PATCH);
const String DebugExtension::NAME("jb2debug");

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        return new (compiler->mem()) DebugExtension(MEM_PASSLOC(compiler->mem()), compiler);
    }
}

DebugExtension::DebugExtension(MEM_LOCATION(a), Compiler *compiler, bool extended, String extensionName)
    : Extension(MEM_PASSLOC(a), CLASSKIND(DebugExtension,Extensible), compiler, (extended ? extensionName : NAME))
    , _cx(compiler->coreExt())
    , _fx(compiler->loadExtension<Func::FunctionExtension>(PASSLOC, &requiredFuncVersion))
    , _bx(compiler->loadExtension<Base::BaseExtension>(PASSLOC, &requiredBaseVersion)) {

}

DebugExtension::~DebugExtension() {
}

DebugEntry *
DebugExtension::debugEntry(LOCATION, Compilation *comp) {
    //rememberCompilation(comp);
    Allocator *mem = _compiler->mem();
    IR *debugIR = comp->ir()->clone(mem);

    //prepareDebugger
    Debugger *jbdb = createDebugger(PASSLOC, mem);

    //compileEntryThunk
    DebuggerThunk *thunk = new (mem) DebuggerThunk(MEM_LOC(mem), jbdb, debugIR);

}

Debugger *
DebugExtension::createDebugger(MEM_LOCATION(a), InputReader *reader, TextLogger *logger) {
    if (reader == NULL) {
        reader = new (a) InputReader(a, stdin); // default reads commands from stdin
    }
    if (logger == NULL) {
        logger = new (a) TextLogger(a, std::cout, "    "); // default writes output to std::cout
    }

    return new (a) Debugger(a, this, reader, logger);
}

#if
    DebuggerThunk *thunk = new DebuggerThunk(this, _func);

#ifdef TRACE_DEBUGTHU
    std::cout.setf(std::ios_base::skipws);
    OMR::JB2::TextWriter printer(thunk, std::cout, String("    "));
    thunk->setLogger(&printer);
    thunk->config()
                   //->setTraceBuildIL()
                   //->setTraceReducer()
                   ->setTraceCodeGenerator();
    std::cerr << "Debug Entry Thunk:" << std::endl;
    printer.print();
    TextWriter *logger = &printer;
#else
    TextWriter *logger = NULL;
#endif

    *returnCode = thunk->Compile(logger, strategy);
    return thunk->nativeEntry<void *>();
#endif

} // namespace Debug
} // namespace JB2
} // namespace OMR

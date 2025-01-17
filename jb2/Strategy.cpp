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

#include "AllocatorRaw.hpp"
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Config.hpp"
#include "Pass.hpp"
#include "Strategy.hpp"
#include "TextLogger.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(Strategy, Passes)

Strategy::Strategy(Allocator *a, Compiler *compiler, String name)
    : Allocatable(a)
    , _id(NoStrategy)
    , _compiler(compiler)
    , _name(name)
    , _config(NULL)
    , _passes(NULL, a) {

    _id = compiler->addStrategy(this);
    _config = compiler->config()->refine(this);
}

Strategy::~Strategy() {
    #if 0 // freed by compiler
    for (auto it = _passes.iterator(); it.hasItem(); it++) {
        Pass *pass = it.item();
        delete pass;
    }
    #endif
    _passes.erase();
}

Strategy *
Strategy::addPass(Pass *pass) {
    // TODO: convert to CompilationException
    assert(pass->compiler() == _compiler);
    _passes.push_back(pass);
    return this;
}

CompilerReturnCode
Strategy::perform(Compilation *comp) {
    _config = comp->config()->refine(this);
    TextLogger *lgr = this->lgr();

    if (lgr) lgr->taggedSectionStart("Strategy", _name);

    CompilerReturnCode rc = _compiler->CompileSuccessful;
    for (auto it = _passes.iterator(); it.hasItem(); it++) {
        Pass *pass = it.item();

        if (lgr) lgr->sectionStart("Strategy pass") << pass->name() << lgr->endl();

        if (lgr) {
            lgr->sectionStart("Before IR") << lgr->endl();
            comp->ir()->log(comp, *lgr);
            lgr->sectionEnd("Before IR") << lgr->endl();
        }

        {
            Allocator passMem("Pass allocator", comp->mem());

            comp->setPassAllocator(&passMem);
            rc = pass->perform(comp);
            comp->setPassAllocator(NULL);
        }

        if (lgr) {
            lgr->sectionStart("After IR") << lgr->endl();
            comp->ir()->log(comp, *lgr);
            lgr->sectionEnd("After IR") << lgr->endl();
        }

        if (lgr) lgr->sectionEnd("Strategy pass") << pass->name() << lgr->endl();

        if (rc != _compiler->CompileSuccessful)
            break;
    }

    if (lgr) {
        lgr->sectionStart("Final IR") << lgr->endl();
        comp->ir()->log(comp, *lgr);
        lgr->sectionEnd("Final IR") << lgr->endl();
    }

    if (lgr) lgr->taggedSectionEnd("Strategy", _name);
    return rc;
}

TextLogger *
Strategy::lgr() const {
    return _config->traceStrategy() ? _config->logger() : NULL;
}

} // namespace JB2
} // namespace OMR

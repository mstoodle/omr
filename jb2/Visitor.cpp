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

#include "Builder.hpp"
#include "Compilation.hpp"
#include "Compiler.hpp"
#include "Config.hpp"
#include "Extension.hpp"
#include "Operation.hpp"
#include "TextLogger.hpp"
#include "Visitor.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(Visitor, Passes)
SUBCLASS_KINDSERVICE_IMPL(Visitor,"Visitor",Pass,Extensible)

Visitor::Visitor(Allocator *a, KINDTYPE(Extensible) kind, Extension *ext, String name, bool visitAppendedBuilders)
    : Pass(a, kind, ext, name)
    , _comp(NULL)
    , _errorCode(ext->compiler()->CompileSuccessful)
    , _aborted(false)
    , _visitAppendedBuilders(visitAppendedBuilders) {
}

Visitor::~Visitor() {

}

CompilerReturnCode
Visitor::perform(Compilation *comp) {
    // got to be a better way to do this
    _config = ext()->compiler()->config()->refine(this);
    this->Pass::perform(comp);
    start(comp);
    if (_aborted) {
        if (_errorCode == compiler()->CompileSuccessful)
            _errorCode = compiler()->CompileFailed;
        return _errorCode;
    }
    return compiler()->CompileSuccessful;
}

void
Visitor::start(Compilation *comp) {
    TextLogger *lgr = this->lgr();
    if (lgr) lgr->taggedSectionStart("Visitor", this->to_string());

    _comp = comp;
    _aborted = false;

    if (lgr) lgr->sectionStart("visitBegin") << this->to_string() << lgr->endl();
    visitBegin();
    if (lgr) lgr->sectionEnd("visitBegin") << this->to_string() << lgr->endl();

    {
        Allocator *mem = comp->mem();
        BuilderList worklist(NULL, mem);
        BitVector visited(mem, _comp->ir()->maxBuilderID());
        _comp->ir()->addInitialBuildersToWorklist(worklist);

        if (lgr) lgr->sectionStart("visitPreCompilation") << _comp->to_string() << lgr->endl();
        visitPreCompilation(_comp);
        if (lgr) lgr->sectionEnd("visitPreCompilation") << _comp->to_string() << lgr->endl();

        while(!worklist.empty()) {
            if (_aborted)
                break;
            Builder *b = worklist.back();

            if (lgr) lgr->sectionStart("visitBuilder") << b << lgr->endl();
            visitBuilder(b, visited, worklist);
            if (lgr) lgr->sectionEnd("visitBuilder") << b << lgr->endl();

            worklist.pop_back();
        }
    }

    if (lgr) lgr->sectionStart("visitPostCompilation") << _comp->to_string() << lgr->endl();
    visitPostCompilation(_comp);
    if (lgr) lgr->sectionEnd("visitPostCompilation") << _comp->to_string() << lgr->endl();

    if (lgr) lgr->sectionStart("visitEnd") << this->to_string() << lgr->endl();
    visitEnd();
    if (lgr) lgr->sectionEnd("visitEnd") << this->to_string() << lgr->endl();

    _aborted = false;
    _comp = NULL;

    if (lgr) lgr->taggedSectionEnd("Visitor", this->to_string());
}

void
Visitor::abort(CompilerReturnCode code) {
    _errorCode = code;
    _aborted = true;

    TextLogger *lgr = this->lgr();
    if (lgr) { lgr->indent() << "Aborted error code is " << _errorCode << lgr->endl(); }
    if (lgr) lgr->taggedSectionEnd("Visitor", this->to_string());
}

void
Visitor::start(Builder * b) {
    BuilderList worklist(NULL, _comp->mem());
    BitVector visited(_comp->mem(), _comp->ir()->maxBuilderID());
    visitBuilder(b, visited, worklist);
}

void
Visitor::start(Operation * op) {
    visitOperation(op);
}

void
Visitor::visitBuilder(Builder *b, BitVector & visited, BuilderList & worklist) {
    TextLogger *lgr = this->lgr();

    int64_t id = b->id();
    if (visited.getBit(id))
        return;

    visited.setBit(id);

    visitBuilderPreOps(b);
    visitOperations(b, visited, worklist);
    visitBuilderPostOps(b);
}

void
Visitor::visitOperations(Builder *b, BitVector & visited, BuilderList & worklist) {
    TextLogger *lgr = this->lgr();
    for (Operation *op = b->firstOperation(); op != NULL; op = op->next()) {

        if (lgr) lgr->sectionStart("visitOperation") << op << lgr->endl();
        visitOperation(op);

        for (auto it = op->builders(); it.hasItem(); it++) {
            Builder * inner_b = it.item();
            if (inner_b && !visited.getBit(inner_b->id())) {
                worklist.push_front(inner_b);
            }
        }
        if (lgr) lgr->sectionEnd("visitOperation") << op << lgr->endl();
    }
}

TextLogger *
Visitor::lgr() const {
   return _config->traceVisitor() ? _config->logger() : NULL;
}

void
Visitor::trace(String msg) {
    TextLogger *lgr = _comp->logger();
    if (lgr) {
        lgr->indent() << msg << lgr->endl();
    }
}

} // namespace JB2
} // namespace OMR

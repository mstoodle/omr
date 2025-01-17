/********************************************************************************
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
#include "CoreExtension.hpp"
#include "Literal.hpp"
#include "LiteralDictionary.hpp"
#include "Operation.hpp"
#include "Symbol.hpp"
#include "SymbolDictionary.hpp"
#include "TextWriter.hpp"
#include "Type.hpp"
#include "TypeDictionary.hpp"
#include "Value.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_REUSECAT(TextWriter, Compiler)
SUBCLASS_KINDSERVICE_IMPL(TextWriter,"TextWriter",Visitor, Extensible);

TextWriter::TextWriter(Allocator *a, Compiler * compiler, std::ostream & os, String perIndent)
    : Visitor(a, CLASSKIND(TextWriter, Extensible), compiler->coreExt(), "TextWriter")
    , _logger(*(new (a) TextLogger(a, os, perIndent))) {

}

TextWriter::TextWriter(Allocator *a, Compiler * compiler, TextLogger & logger)
    : Visitor(a, CLASSKIND(TextWriter, Extensible), compiler->coreExt(), "TextWriter")
    , _logger(logger) {

}

TextWriter::~TextWriter() {

}

void
TextWriter::start(Compilation *comp) {
    bool savedFlag = _config->traceVisitor();
    _config->setTraceVisitor(false);
    this->Visitor::start(comp);
    _config->setTraceVisitor(savedFlag);
}

void
TextWriter::visitPreCompilation(Compilation * comp) {
    _logger.indent() << "[ Compilation " << _logger.endl();
    _logger.indentIn();
}

void
TextWriter::visitPostCompilation(Compilation * comp) {
    _logger.indentOut();
    _logger.indent() << "]" << _logger.endl();
}

void
TextWriter::visitBuilderPreOps(Builder * b) {
    b->logPrefix(_logger);
}

void
TextWriter::visitBuilderPostOps(Builder * b) {
    b->logSuffix(_logger);
}

void
TextWriter::visitOperation(Operation *op) {
    _logger.logOperation(op);
}

} // namespace JB2
} // namespace OMR

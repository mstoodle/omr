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

#include "AllocatorTracer.hpp"
#include "AllocatorTracker.hpp"
#include "Config.hpp"


namespace OMR {
namespace JB2 {

INIT_JBALLOC(Config)

Config::Config(Allocator *a)
    : Allocatable(a)
    , _traceStrategy(false)
    , _traceVisitor(false)
    , _traceBuildIL(false)
    , _traceCodeGenerator(false)
    , _traceCompilationAllocations(false)
    , _traceCompilerAllocations(false)
    , _traceTypeReplacer(false)
    , _trackCompilationAllocations(false)
    , _trackCompilerAllocations(false)
    , _verboseErrors(false)
    , _lastTransformationIndex(-1) // no limit
    , _logRegex("")
    , _logger(NULL) {
}

Config::Config()
    : Allocatable()
    , _traceStrategy(false)
    , _traceVisitor(false)
    , _traceBuildIL(false)
    , _traceCodeGenerator(false)
    , _traceCompilationAllocations(false)
    , _traceCompilerAllocations(false)
    , _traceTypeReplacer(false)
    , _trackCompilationAllocations(false)
    , _trackCompilerAllocations(false)
    , _verboseErrors(false)
    , _lastTransformationIndex(-1) // no limit
    , _logRegex("")
    , _logger(NULL) {
}

Config::Config(Allocator *a, Config *parent)
    : Allocatable(a)
    , _traceStrategy(parent->_traceStrategy)
    , _traceVisitor(parent->_traceVisitor)
    , _traceBuildIL(parent->_traceBuildIL)
    , _traceCodeGenerator(parent->_traceCodeGenerator)
    , _traceCompilationAllocations(false)
    , _traceCompilerAllocations(false)
    , _traceTypeReplacer(parent->_traceTypeReplacer)
    , _trackCompilationAllocations(false)
    , _trackCompilerAllocations(false)
    , _verboseErrors(false)
    , _lastTransformationIndex(parent->_lastTransformationIndex)
    , _logRegex(parent->_logRegex)
    , _logger(parent->_logger) {
}

Config::Config(Config *parent)
    : Allocatable()
    , _traceStrategy(parent->_traceStrategy)
    , _traceVisitor(parent->_traceVisitor)
    , _traceBuildIL(parent->_traceBuildIL)
    , _traceCodeGenerator(parent->_traceCodeGenerator)
    , _traceCompilationAllocations(false)
    , _traceCompilerAllocations(false)
    , _traceTypeReplacer(parent->_traceTypeReplacer)
    , _trackCompilationAllocations(false)
    , _trackCompilerAllocations(false)
    , _verboseErrors(false)
    , _lastTransformationIndex(parent->_lastTransformationIndex)
    , _logRegex(parent->_logRegex)
    , _logger(parent->_logger) {
}

Config::~Config() {
    // allocates and holds no memory so nothing to do
}

Allocator *
Config::allocateAllocators(Allocator *allocator, bool tracker, bool tracer) {
    if (tracker)
        allocator = new (allocator) AllocatorTracker("Tracker", allocator, _logger);
    if (tracer) {
        assert(_logger);
        allocator = new (allocator) AllocatorTracer("Tracer", allocator, *_logger);
    }
    return allocator;
}

void
Config::destructAllocators(Allocator *allocator, bool tracker, bool tracer) {
    assert(allocator != NULL);
    if (tracer) {
        Allocator *tracer = allocator;
        allocator = allocator->parent();
        assert(allocator != NULL);
        delete tracer;
    }
    if (tracker) {
        Allocator *tracker = allocator;
        allocator = allocator->parent();
        assert(allocator != NULL);
        delete tracker;
    }
}

} // namespace JB2
} // namespace OMR

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

#ifndef CONFIG_INCL
#define CONFIG_INCL

#include <string>

namespace OMR {
namespace JitBuilder {

class FunctionBuilder;
class Transformer;

class Config {
public:
    Config()
        : _traceBuildIL(false)
        , _traceCodeGenerator(false)
        , _traceTypeReplacer(false)
        , _lastTransformationIndex(-1) // no limit
        , _logRegex("") {
    }

    // when true, turn logging on when buildIL() is called
    bool traceBuildIL() const                                 { return _traceBuildIL; }
    Config * setTraceBuildIL(bool v=true)                     { _traceBuildIL = v; return this; }

    // when true, turn logging on when CodeGenerator runs
    bool traceCodeGenerator() const                           { return _traceCodeGenerator; }
    Config * setTraceCodeGenerator(bool v=true)               { _traceCodeGenerator = v; return this; }

    // when true, turn logging on when CodeGenerator runs
    bool traceTypeReplacer() const                            { return _traceTypeReplacer; }
    Config * setTraceTypeReplacer(bool v=true)                { _traceTypeReplacer = v; return this; }

    // if >= 0, identifies the last transformation to apply
    bool limitLastTransformationIndex() const                 { return _lastTransformationIndex >= 0; }
    TransformationID lastTransformationIndex() const          { return _lastTransformationIndex; }
    Config * setLastTransformationIndex(TransformationID idx) { _lastTransformationIndex = idx; return this; }

    // when true, logging should be enabled
    bool logCompilation(Compilation * fb) const               { return false; } // TODO: match name against _logRegex
    Config * setLogRegex(std::string regex)                   { _logRegex = regex; return this; }

protected:
    bool _traceBuildIL;
    bool _traceCodeGenerator;
    bool _traceTypeReplacer;

    TransformationID _lastTransformationIndex;

    std::string _logRegex;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(CONFIG_INCL)


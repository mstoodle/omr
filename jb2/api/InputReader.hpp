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

#ifndef INPUTREADER_INCL
#define INPUTREADER_INCL

#include <stdint.h>
#include <stdio.h>

namespace OMR {
namespace JitBuilder {

class InputReader {
public:
    InputReader(FILE *inputFile);

    char * getLine();
    bool done() const { return _done; }

protected:
    FILE *_inputFile;
    uint32_t _bufferLength;
    char *_buffer;
    bool _done;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(INPUTREADER_INCL)

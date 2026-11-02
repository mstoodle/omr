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

// JBIRFileWriter.hpp — writes JBIR format data to a binary FILE*.

#ifndef JBIRFILEWRITER_INCL
#define JBIRFILEWRITER_INCL

#include <cstdio>
#include "JBIRWriter.hpp"

namespace OMR {
namespace JB2 {

class JBIRFileWriter : public JBIRWriter {
public:
    // Construct a writer targeting the given open binary FILE*.
    // The file must already be positioned at the point where writing begins.
    // The caller opens and closes the file; this class owns no resources.
    explicit JBIRFileWriter(FILE *f);

protected:
    bool writeRawBytes(const uint8_t *bytes, size_t len) override;

private:
    FILE *_f;
};

} // namespace JB2
} // namespace OMR

#endif // defined(JBIRFILEWRITER_INCL)

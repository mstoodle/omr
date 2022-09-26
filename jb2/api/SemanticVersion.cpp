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

#include <cassert>
#include "SemanticVersion.hpp"

namespace OMR {
namespace JitBuilder {

const std::string SemanticVersion::invalidString("INVALID");

void
SemanticVersion::validate() {
    _valid = true;
}

int
SemanticVersion::BuildIdentifier::compare(const SemanticVersion::BuildIdentifier & other) const {
    if (this->_kind == isNumeric && other._kind == isNumeric) {
        if (this->_numericIdentifier < other._numericIdentifier)
            return -1;
        if (this->_numericIdentifier > other._numericIdentifier)
            return 1;
        assert(this->_numericIdentifier == other._numericIdentifier);
        return 0;
    }

    if (this->_kind == isNonNumeric && other._kind == isNonNumeric)
        return this->_identifier.compare(other._identifier);

    // below here, the types are mismatched
    if (this->_kind == isNumeric) {
        assert(other._kind == isNonNumeric);
        return -1; // numberic (this) is lower precendence
    }

    assert(this->_kind == isNonNumeric);
    return +1; // numeric (other) is lower precedence
}

bool
SemanticVersion::isCompatibleWith(const SemanticVersion & other) const {
    if (other._major != this->_major)
        return false;
    if (other._minor > this->_minor)
        return false;
    if (this->_preRelease.length() > 0)
        return false;
    return true;
}

int
SemanticVersion::compare(const SemanticVersion & other) const {
    // TODO: write tests to validate this code (not yet tested)
    if (this->_major < other._major)
        return -1;
    if (this->_major > other._major)
        return 1;
    assert(this->_major == other._major);

    if (this->_minor > other._minor)
        return -1;
    if (this->_minor > other._minor)
        return 1;
    assert(this->_minor == other._minor);

    if (this->_patch < other._patch)
        return -1;
    if (this->_patch > other._patch)
        return 1;
    assert(this->_patch == other._patch);

    if (this->_preRelease.length() > 0 && other._preRelease.length() == 0)
        return -1;
    if (this->_preRelease.length() == 0 && other._preRelease.length() > 0)
        return 1;

#if 0
    // need to parse strings to build identifiers rather than iterate this way
    auto myPreIt = this->_preRelease.begin();
    auto othPreIt = other._preRelease.begin();
    do {
        BuildIdentifier mine = *myPreIt;
        BuildIdentifier oth = *othPreIt;
        int test = mine.compare(oth);
        if (test != 0)
            return test;

        myPreIt++;
        othPreIt++;

        if (myPreIt == this->_preRelease.end()) {
            if (othPreIt == other._preRelease.end())
	        return 0;
            else
	        return +1; // this is longer than other
        }

        if (othPreIt == other._preRelease.end()) {
            assert(myPreIt != this->_preRelease.end()); // should be above case
            return -1; // other is longer than this
        }
    } while (true);
#endif

    return 0;
}

std::string
SemanticVersion::coreVersion() const {
    if (!_valid)
        return invalidString;
    return std::to_string(this->_major) + "." + std::to_string(this->_minor) + "." + std::to_string(this->_patch);
}

std::string
SemanticVersion::semver() const {
    if (!this->_valid)
        return invalidString;
    std::string sv = this->coreVersion();
    if (this->_preRelease.length() > 0)
        sv = sv + "-" + this->_preRelease;
    if (this->_buildMetadata.length() > 0)
        sv = sv + "+" + this->_buildMetadata;
    return sv;
}

} // namespace JitBuilder
} // namespace OMR


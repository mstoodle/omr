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

#ifndef SEMANTICVERSION_INCL
#define SEMANTICVERSION_INCL

#include <list>
#include <string>

namespace OMR {
namespace JitBuilder {

typedef uint16_t MajorID;
typedef uint16_t MinorID;
typedef uint16_t PatchID;

class SemanticVersion {
protected:

    // TODO: to be used in comparing build IDs
    struct BuildIdentifier {
        enum { isNumeric, isNonNumeric } _kind;
        std::string _identifier;
        uint64_t _numericIdentifier;
        int compare(const BuildIdentifier & other) const;
    };

public:
    SemanticVersion(MajorID major, MinorID minor, PatchID patch, std::string preRelease, std::string buildMetadata)
        : _valid(false)
        , _major(major)
        , _minor(minor)
        , _patch(patch)
        , _preRelease(preRelease)
        , _buildMetadata(buildMetadata) {
            validate();
        }
    SemanticVersion(MajorID major=0, MinorID minor=0, PatchID patch=0)
        : _valid(false)
        , _major(major)
        , _minor(minor)
        , _patch(patch)
        , _preRelease("")
        , _buildMetadata("") {
            validate();
        }

    std::string coreVersion() const;
    std::string semver() const;

    bool isValid() const { return this->_valid; }
    bool isStable() const { return (this->_major > 0); }
    bool isCompatibleWith(const SemanticVersion & other) const;
    int compare(const SemanticVersion & other) const;

    MajorID major() const { return this->_major; }
    MinorID minor() const { return this->_minor; }
    PatchID patch() const { return this->_patch; }
    std::string preRelease() const { return this->_preRelease; }
    std::string buildMetadata() const { return this->_buildMetadata; }

    protected:
    void validate();

    bool _valid;
    MajorID _major;
    MinorID _minor;
    PatchID _patch;
    std::string _preRelease;
    std::string _buildMetadata;

    static const std::string invalidString;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(SEMANTICVERSION_INCL)


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

#ifndef STRING_INCL
#define STRING_INCL

#include <string>

namespace OMR {
namespace JitBuilder {

class TextWriter;

class String {
    friend class TextWriter;

public:
    String()
        : _string("") {
    }
    String(const char *s) 
        : _string(s) {
    }
    String(const String & s)
        : _string(s._string) {
    }

    String operator=(const char *s) {
        _string = std::string(s);
        return *this;
    }
    String operator=(const String other) {
        _string = other._string;
        return *this;
    }
    operator std::string() const { return _string; }

    size_t length() const {
        return _string.length();
    }

    void write(TextWriter & w) const;

    String append(const String & other) {
        _string = _string.append(other._string);
        return *this;
    }

    String operator+(String other) {
        String concat((_string + other._string).c_str());
        return concat;
    }
    String operator+(const char * other) {
        String concat((_string + other).c_str());
        return concat;
    }
    bool operator==(const String other) const {
        return _string == other._string;
    }
    bool operator<(const String other) const {
        return _string < other._string;
    }

    const char *c_str() const {
        return _string.c_str();
    }

    static String to_string(int64_t v) {
        String *str = new String(std::to_string(v).c_str());
        return *str;
    }
    static String to_string(uint64_t v) {
        String *str = new String(std::to_string(v).c_str());
        return *str;
    }
    static String to_string(int32_t v) {
        String *str = new String(std::to_string(v).c_str());
        return *str;
    }
    static String to_string(uint32_t v) {
        String *str = new String(std::to_string(v).c_str());
        return *str;
    }
    static String to_string(int16_t v) {
        String *str = new String(std::to_string(v).c_str());
        return *str;
    }
    static String to_string(uint16_t v) {
        String *str = new String(std::to_string(v).c_str());
        return *str;
    }

protected:
    std::string _string;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(STRING_INCL)


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

#include "Allocatable.hpp"

namespace OMR {
namespace JB2 {

class TextLogger;

class String : public Allocatable {
    JBALLOC_(String)

    friend class TextLogger;

public:
    // Explicit Allocator parameter is for the underlying characters, not for the String object itself
    ALL_ALLOC_ALLOWED(String, Allocator *dataAllocator);
    ALL_ALLOC_ALLOWED(String, Allocator *dataAllocator, const char *s);
    ALL_ALLOC_ALLOWED(String, Allocator *dataAllocator, const String & s);

    // If explicit Allocator isn't provided, then some operations will fail unless an Allocator is provided (provideAllocator)
    // Commonly used to provide default values for String parameters
    String();
    String(const char *s);

    // rule of 3
    String(const String & other);
    String operator=(const String & other);
        
    // rule of 5
    String(String && other);
    String & operator=(const String && other);
    String & operator=(const char *s);

    size_t length()    const { return _length; }
    const char *c_str() const {
        if (_length == 0)
            return "";
        return _bytes;
    }

    void provideAllocator(Allocator *mem) { _dataAllocator = mem; }

    void log(TextLogger & log) const;

    String & append(const String & other);
    String operator+(const String & other);
    String operator+(const char * other);
    bool operator==(const char * other) const;
    bool operator<(const char * other) const;
    bool operator==(const String & other) const;
    bool operator<(const String & other) const;

#if defined(OSX)
    static String to_string(Allocator *dataAllocator, size_t v);
#endif
    static String to_string(Allocator *dataAllocator, int64_t v);
    static String to_string(Allocator *dataAllocator, uint64_t v);
    static String to_string(Allocator *dataAllocator, int32_t v);
    static String to_string(Allocator *dataAllocator, uint32_t v);
    static String to_string(Allocator *dataAllocator, int16_t v);
    static String to_string(Allocator *dataAllocator, uint16_t v);

protected:
    void initializeBytes(size_t len, const char *source);
    void grow(const char *suffix, size_t suffixLen);

    Allocator *_dataAllocator;
    const char *_bytes;
    size_t _length;
};

} // namespace JB2
} // namespace OMR

#endif // defined(STRING_INCL)

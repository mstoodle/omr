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

#include "String.hpp"
#include "TextLogger.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC(String)

String::String()
    : Allocatable()
    , _dataAllocator(NULL)
    , _bytes(NULL)
    , _length(0) {

}

String::String(const char *s) 
    : Allocatable()
    , _dataAllocator(NULL)
    , _bytes(s)
    , _length(strlen(s)) {

}

String::String(Allocator *dataAllocator)
    : Allocatable()
    , _dataAllocator(dataAllocator)
    , _bytes(NULL)
    , _length(0) {

}

String::String(Allocator *a, Allocator *dataAllocator, const char *s) 
    : Allocatable(a)
    , _dataAllocator(dataAllocator)
    , _bytes(NULL)
    , _length(0) {

    if (s != NULL)
        initializeBytes(strlen(s), s);
}

String::String(Allocator *dataAllocator, const char *s) 
    : Allocatable()
    , _dataAllocator(dataAllocator)
    , _bytes(NULL)
    , _length(0) {

    if (s != NULL)
        initializeBytes(strlen(s), s);
}

String::String(Allocator *a, Allocator *dataAllocator, const String & s)
    : Allocatable(a)
    , _dataAllocator(dataAllocator)
    , _bytes(NULL)
    , _length(0) {

    initializeBytes(s._length, s._bytes);
}

String::String(Allocator *dataAllocator, const String & s)
    : Allocatable()
    , _dataAllocator(dataAllocator)
    , _bytes(NULL)
    , _length(0) {

    initializeBytes(s._length, s._bytes);
}

// rule of 3
String::String(const String & other)
    : Allocatable(other)
    , _dataAllocator(other._dataAllocator)
    , _bytes(NULL)
    , _length(0) {

    if (_dataAllocator == NULL) {
        _bytes = other._bytes;
        _length = other._length;
    } else {
        initializeBytes(other._length, other._bytes);
    }
}

String
String::operator=(const String & other) {
    if (_dataAllocator != NULL) {
        if (_bytes != NULL)
            _dataAllocator->deallocate(const_cast<char *>(_bytes));
    } else {
        _dataAllocator = other._dataAllocator;
    }

    if (_dataAllocator != NULL) {
        initializeBytes(other._length, other._bytes);
    } else {
        _bytes = other._bytes;
        _length = other._length;
    }
    return *this;
}

// rule of 5
String::String(String && other)
    : Allocatable(other)
    , _dataAllocator(other._dataAllocator)
    , _length(other._length) {

    if (_dataAllocator != NULL) {
        initializeBytes(other._length, other._bytes);
    } else {
        _bytes = other._bytes;
        _length = other._length;
    }
}

String &
String::operator=(const String && other) {
    if (_dataAllocator != NULL) {
        if (_bytes != NULL)
            _dataAllocator->deallocate(const_cast<char *>(_bytes));
    } else {
        _dataAllocator = other._dataAllocator;
    }

    if (_dataAllocator != NULL) {
        initializeBytes(other._length, other._bytes);
    } else {
        _bytes = other._bytes;
        _length = other._length;
    }
    return *this;
}

String::~String() {
    if (_bytes != NULL && _dataAllocator != NULL)
        _dataAllocator->deallocate(const_cast<char *>(_bytes));
}

String &
String::operator=(const char *s) {
    if (_dataAllocator != NULL) {
        if (_bytes != NULL)
            _dataAllocator->deallocate(const_cast<char *>(_bytes));
        initializeBytes(strlen(s), s);
    } else {
        _bytes = s;
        _length = strlen(s);
    }
    return *this;
}

String &
String::append(const String & other) {
    if (_dataAllocator == NULL) {
        if (other._dataAllocator == NULL) {
            assert(0); // need some allocator to append
        } else {
            _dataAllocator = other._dataAllocator;
        }
    }
    grow(other._bytes, other._length);
    return *this;
}

String
String::operator+(const String & other) {
    String concat(*this);
    concat.append(other);
    return concat;
}

String
String::operator+(const char * other) {
    String concat(*this);
    String appender(_dataAllocator, other);
    concat.append(appender);
    return concat;
}

bool
String::operator==(const char * other) const {
    return strncmp(_bytes, other, _length+1) == 0;
}

bool
String::operator<(const char * other) const {
    return strncmp(_bytes, other, _length+1) < 0;
}

bool
String::operator==(const String & other) const {
    return strncmp(_bytes, other._bytes, _length+1) == 0;
}

bool
String::operator<(const String & other) const {
    return strncmp(_bytes, other._bytes, _length+1) < 0;
}

#if defined(OSX)
String
String::to_string(Allocator *dataAllocator, size_t v) {
    char digits[25];
    snprintf(digits, 25, "%zu", v);
    String str(dataAllocator, digits);
    return str;
}
#endif

String
String::to_string(Allocator *dataAllocator, int64_t v) {
    char digits[25];
    snprintf(digits, 25, "%lld", v);
    String str(dataAllocator, digits);
    return str;
}

String
String::to_string(Allocator *dataAllocator, uint64_t v) {
    char digits[25];
    snprintf(digits, 25, "%llu", v);
    String str(dataAllocator, digits);
    return str;
}

String
String::to_string(Allocator *dataAllocator, int32_t v) {
    char digits[15];
    snprintf(digits, 15, "%d", v);
    String str(dataAllocator, digits);
    return str;
}

String
String::to_string(Allocator *dataAllocator, uint32_t v) {
    char digits[15];
    snprintf(digits, 15, "%u", v);
    String str(dataAllocator, digits);
    return str;
}

String
String::to_string(Allocator *dataAllocator, int16_t v) {
    char digits[8];
    snprintf(digits, 8, "%d", v);
    String str(dataAllocator, digits);
    return str;
}

String
String::to_string(Allocator *dataAllocator, uint16_t v) {
    char digits[8];
    snprintf(digits, 8, "%u", v);
    String str(dataAllocator, digits);
    return str;
}

void
String::initializeBytes(size_t len, const char *source) {
    assert(_dataAllocator != NULL);
    if (len > 0) {
        size_t copyLen = len+1;
        char *buffer = _dataAllocator->allocate<char>(copyLen);
        memcpy(buffer, source, copyLen);
        _length = len;
        _bytes = buffer;
    } else {
        _bytes = NULL;
        _length = 0;
    }
}

void
String::grow(const char *suffix, size_t suffixLen) {
    assert(_dataAllocator != NULL);
    assert(suffixLen > 0);
    size_t newLength = _length + suffixLen;
    char *newBytes = _dataAllocator->allocate<char>(newLength+1);
    if (_length > 0)
        memcpy(newBytes, _bytes, _length);
    memcpy(newBytes+_length, suffix, suffixLen+1);
    if (_bytes != NULL)
        _dataAllocator->deallocate(const_cast<char *>(_bytes));
    _bytes = newBytes;
    _length = newLength;
}

void
String::log(TextLogger & log) const {
    log << _bytes;
}

} // namespace JB2
} // namespace OMR

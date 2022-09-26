/*******************************************************************************
 * Copyright (c) 2022, 2021 IBM Corp. and others
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

#ifndef TEXTWRITER_INCL
#define TEXTWRITER_INCL

#include <iostream>
#include <iomanip>
#include <vector>
#include <deque>
#include "Visitor.hpp"

namespace OMR {
namespace JitBuilder {

class Builder;
class Case;
class Compilation;
class Literal;
class LiteralDictionary;
class Operation;
class Symbol;
class SymbolDictionary;
class Type;
class TypeDictionary;
class Value;

class TextWriter : public Visitor {
public:
    TextWriter(Compilation * comp, std::ostream & os, std::string perIndent);

    void print(Compilation *comp) { start(comp); }
    void print(Builder * b) { start(b); }
    void print(Operation * op) { start(op); }

    friend TextWriter &operator<<(TextWriter &w, const bool v) {
        w._os << v;
        return w;
    }
    friend TextWriter &operator<<(TextWriter &w, const int8_t v) {
        w._os << v;
        return w;
    }
    friend TextWriter &operator<<(TextWriter &w, const int16_t v) {
        w._os << v;
        return w;
    }
    friend TextWriter &operator<<(TextWriter &w, const int32_t v) {
        w._os << v;
        return w;
    }
    friend TextWriter &operator<<(TextWriter &w, const int64_t v) {
        w._os << v;
        return w;
    }
    friend TextWriter &operator<<(TextWriter &w, const uint64_t v) {
        w._os << v;
        return w;
    }
    #if 0
    friend TextWriter &operator<<(TextWriter &w, const size_t v) {
        w._os << v;
        return w;
    }
    #endif
    friend TextWriter &operator<<(TextWriter &w, const void * v) {
        w._os << v;
        return w;
    }
    friend TextWriter &operator<<(TextWriter &w, const float v) {
        w._os << v;
        return w;
    }
    friend TextWriter &operator<<(TextWriter &w, const double v) {
        w._os << v;
        return w;
    }
    friend TextWriter &operator<<(TextWriter &w, const std::string s) {
        w._os << s;
        return w;
    }
    friend TextWriter &operator<<(TextWriter &w, const char *s) {
        w._os << s;
        return w;
    }
    friend TextWriter & operator<<(TextWriter &w, const Builder *b);
   //friend TextWriter &operator<<(TextWriter &w, const Case *c);
    friend TextWriter & operator<<(TextWriter &w, const Literal *lv);
    friend TextWriter & operator<<(TextWriter &w, const LiteralDictionary *ld);
    friend TextWriter & operator<<(TextWriter &w, const Operation *op);
    friend TextWriter & operator<<(TextWriter &w, const Symbol *s);
    friend TextWriter & operator<<(TextWriter &w, const SymbolDictionary *sd);
    friend TextWriter & operator<<(TextWriter &w, const Type *t);
    friend TextWriter & operator<<(TextWriter &w, const TypeDictionary *dict);
    friend TextWriter & operator<<(TextWriter &w, const Value *v);

    void writeType(const Type *type, bool indent=true);
    void writeOperation(Operation *op);

    std::string endl() {
        return std::string("\n");
    }

    TextWriter & indent() {
        for (int32_t in=0;in < _indent;in++)
            _os << _perIndent;
        return *this;
    }
    void indentIn() {
        _indent++;
    }
    void indentOut() {
        _indent--;
    }

protected:

    virtual void visitPreCompilation(Compilation * comp);
    virtual void visitPostCompilation(Compilation * comp);
    virtual void visitBuilderPreOps(Builder * b);
    virtual void visitBuilderPostOps(Builder * b);
    virtual void visitOperation(Operation * op);

    void printTypePrefix(const Type * type, bool indent=true);
    void printOperationPrefix(Operation * op);

    std::ostream & _os;
    std::string _perIndent;
    int32_t _indent;
};

// RAII class for indenting log output
class LogIndent {
    public:
    LogIndent(TextWriter *log)
        : _log(log) {

        if (log)
            log->indentIn();
    }

    ~LogIndent() {
        if (_log)
            _log->indentOut();
    }

private:
    TextWriter *_log;
};

// This macro can be used to bracket a code region where log output should be indented
#define LOG_INDENT_REGION(log) if (true) { LogIndent __log__indent__var(log);
#define LOG_OUTDENT            }

} // namespace JitBuilder
} // namespace OMR

#endif // defined(TEXTWRITER_INCL)


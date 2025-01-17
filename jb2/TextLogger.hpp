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

#ifndef TEXTLOGGER_INCL
#define TEXTLOGGER_INCL

#include <iomanip>
#include "common.hpp"
#include "Extensible.hpp"
#include "KindService.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Builder;
class Case;
class Literal;
class LiteralDictionary;
class Operation;
class Scope;
class Symbol;
class SymbolDictionary;
class Type;
class TypeDictionary;
class Value;

class TextLogger : public Allocatable { // should be Allocatable
    JBALLOC_(TextLogger)

public:
    ALL_ALLOC_ALLOWED(TextLogger, std::ostream & os, String perIndent);

    friend TextLogger &operator<<(TextLogger &log, const bool v);
    friend TextLogger &operator<<(TextLogger &log, const int8_t v);
    friend TextLogger &operator<<(TextLogger &log, const int16_t v);
    friend TextLogger &operator<<(TextLogger &log, const int32_t v);
    friend TextLogger &operator<<(TextLogger &log, const int64_t v);
    friend TextLogger &operator<<(TextLogger &log, const uint32_t v);
    friend TextLogger &operator<<(TextLogger &log, const uint64_t v);
    #if defined(OSX)
    friend TextLogger &operator<<(TextLogger &log, const size_t v);
    #endif
    friend TextLogger &operator<<(TextLogger &log, const void * v);
    friend TextLogger &operator<<(TextLogger &log, const float v);
    friend TextLogger &operator<<(TextLogger &log, const double v);
    friend TextLogger &operator<<(TextLogger &log, const String & s);
    friend TextLogger &operator<<(TextLogger &log, const char *s);

    friend TextLogger & operator<<(TextLogger &log, const Builder *b);
    friend TextLogger & operator<<(TextLogger &log, const Literal *lv);
    friend TextLogger & operator<<(TextLogger &log, const LiteralDictionary *ld);
    friend TextLogger & operator<<(TextLogger &log, const Operation *op);
    friend TextLogger & operator<<(TextLogger &log, const Scope *s);
    friend TextLogger & operator<<(TextLogger &log, const Symbol *sym);
    friend TextLogger & operator<<(TextLogger &log, const SymbolDictionary *sd);
    friend TextLogger & operator<<(TextLogger &log, const Type *t);
    friend TextLogger & operator<<(TextLogger &log, const TypeDictionary *dict);
    friend TextLogger & operator<<(TextLogger &log, const Value *v);

    void logOperation(Operation *op);

    TextLogger & tagLine();

    String sectionBegin();
    String sectionStop();

    TextLogger & taggedSectionStart (String section, String extra);
    TextLogger & taggedSectionEnd (String section, String extra);
    TextLogger & sectionStart (String section);
    TextLogger & sectionEnd(String section);

    String irStart();
    String irStop();
    String irSpacedStop();
    TextLogger & irListBegin(String name, size_t numEntries);
    TextLogger & irListEnd(size_t numEntries);
    TextLogger & irSectionBegin(String title, String designator, uint64_t id, ExtensibleKind kind, String name);
    TextLogger & irSectionEnd();
    TextLogger & irOneLinerBegin(String title, String designator, uint64_t id);
    TextLogger & irOneLinerEnd();
    TextLogger & irFlagBegin(String flag);
    TextLogger & irFlagEnd();
    template<typename T>
    TextLogger & irFlagOrNull(String flag, T *thing) {
        irFlagBegin(flag);
        if (thing == NULL)
            *this << "NULL";
        else
            *this << thing;
        irFlagEnd();
        return *this;
    }
    TextLogger & irBooleanFlag(String flag, bool on);

    String endl() const;
    TextLogger & indent();
    void indentIn();
    void indentOut();

protected:

    //void logTypePrefix(const Type * type, bool indent=true);

    std::ostream & _os;
    String _perIndent;
    int32_t _indent;
};

// RAII class for indenting log output
class LogIndent {
    public:
    LogIndent(TextLogger *log)
        : _log(log) {

        if (log)
            log->indentIn();
    }
    LogIndent(TextLogger & log)
        : _log(&log) {

        log.indentIn();
    }

    ~LogIndent() {
        if (_log)
            _log->indentOut();
    }

private:
    TextLogger *_log;
};

// This macro can be used to bracket a code region where log output should be indented
#define LOG_INDENT_REGION(log) if (true) { LogIndent __log__indent__var(log);
#define LOG_OUTDENT            }

} // namespace JB2
} // namespace OMR

#endif // defined(TEXTLOGGER_INCL)

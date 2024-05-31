/*******************************************************************************
 * Copyright IBM Corp. and others 2024
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

#include <dlfcn.h>
#include <limits>
#include <stdio.h>
#include "gtest/gtest.h"
#include "JBCore.hpp"
#include "Func/Func.hpp"
#include "Base/Base.hpp"
#include "omrgen/omrgen.hpp"

using namespace OMR::JitBuilder;

static Compiler *getCompiler();

static bool initialized = false;
static Compiler global("Global Compiler");
static Compiler *c = getCompiler();
static Config *config = getCompiler()->config();
static CoreExtension *cx = getCompiler()->lookupExtension<CoreExtension>();
static Base::BaseExtension *bx = getCompiler()->lookupExtension<Base::BaseExtension>();
static Func::FunctionExtension *fx = getCompiler()->lookupExtension<Func::FunctionExtension>();
static omrgen::OMRExtension *ogx = getCompiler()->lookupExtension<omrgen::OMRExtension>();
static const Type *NoType = getCompiler()->lookupExtension<CoreExtension>()->NoType;
static const Type *Int8 = getCompiler()->lookupExtension<Base::BaseExtension>()->Int8;
static const Type *Int16 = getCompiler()->lookupExtension<Base::BaseExtension>()->Int16;
static const Type *Int32 = getCompiler()->lookupExtension<Base::BaseExtension>()->Int32;
static const Type *Int64 = getCompiler()->lookupExtension<Base::BaseExtension>()->Int64;
static const Type *Float32 = getCompiler()->lookupExtension<Base::BaseExtension>()->Float32;
static const Type *Float64 = getCompiler()->lookupExtension<Base::BaseExtension>()->Float64;
static const Type *Address = getCompiler()->lookupExtension<Base::BaseExtension>()->Address;
static const Type *Word = getCompiler()->lookupExtension<Base::BaseExtension>()->Word;

static Compiler *getCompiler() {
    if (!initialized) {
        initialized=true;

        c = &global;
        c->loadExtension<Base::BaseExtension>(LOC);
        c->loadExtension<Func::FunctionExtension>(LOC);
        c->loadExtension<omrgen::OMRExtension>(LOC);
    }
    return c;
}


int main(int argc, char** argv) {
    void *handle = dlopen(CORELIB, RTLD_LAZY);
    if (!handle) {
        fputs(dlerror(), stderr);
        return -1;
    }

    // creating a Compiler here means JIT will be initialized and shutdown only once
    //  which means all compiled functions can be logged/tracked
    // otherwise JIT will be initialized and shutdown with each test's Compiler instance
    //  and verbose/logs are overwritten and recreated for each Compiler instance
    //  making it much more difficult to log an individual compiled function
    getCompiler();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class TestSetup : public Func::Function {
public:
    TestSetup(LOCATION, Compiler *compiler, bool log)
        : Func::Function(PASSLOC, compiler)
        , _compiler(compiler)
        , _config(compiler->config())
        , _lgr(std::cout, String("    "))
        , _wrt(log ? &_lgr : NULL) {

        if (_wrt)
            _config->setTraceBuildIL();
    }

    virtual void compile(LOCATION) = 0;
    virtual void run(LOCATION) = 0;

protected:
    Compiler *compiler() const { return _compiler; }
    Config *config() const { return _config; }
    TextLogger *wrt() const { return _wrt; }
    Func::FunctionExtension *fx() const { return ::fx; }
    Base::BaseExtension *bx() const { return ::bx; }
    omrgen::OMRExtension *ogx() const { return ::ogx; }

private:
    Compiler *_compiler;
    Config *_config;
    TextLogger _lgr;
    TextLogger *_wrt;
};

class TestFunc : public TestSetup {
public:
    TestFunc(LOCATION, Compiler *compiler, bool log)
        : TestSetup(PASSLOC, compiler, log) {

    }

    void compile(LOCATION) {
        const StrategyID codegenStrategy = cx()->strategyCodegen;
        _body = fx()->compile(PASSLOC, this, codegenStrategy, wrt());
        EXPECT_NE(_body, nullptr) << "Compiled body ok";
        EXPECT_EQ((int)_body->rc(), (int)compiler()->CompileSuccessful) << "Compiled function ok";
    }

protected:
    CompiledBody *body() const { return _body; }

private:
    CompiledBody *_body;
};

// Return void
class ReturnVoidFunc : public TestFunc {
public:
    ReturnVoidFunc(LOCATION, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log) {

        DefineName("ReturnVoidFunc");
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    
    void run(LOCATION) {
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineReturnType(NoType);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        fx()->Return(LOC, entry);
        return true;
    }
};

TEST(omrgenExtension, ReturnVoid) {
    ReturnVoidFunc rvf(LOC, c, false);
    rvf.compile(LOC);
}

// Return primitive value
template<typename FuncPrototype, typename cType>
class ReturnValueFunc : public TestFunc {
public:
    ReturnValueFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _type(NULL)
        , _value(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(), _value) << "Compiled f() returns " << _value;
    }
    void test(LOCATION, const Type *type, cType value) {
        _type = type;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineReturnType(_type);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        Literal *lv = new (comp->mem()) Literal(MEM_LOC(comp->mem()), comp->ir(), _type, reinterpret_cast<const LiteralBytes *>(&_value));
        Value *c = bx()->Const(LOC, entry, lv);
        fx()->Return(LOC, entry, c);
        return true;
    }

private:
    const Type *_type;
    cType _value;
};

TEST(omrgenExtension, ReturnConstInt8) {
    typedef int8_t (FuncProto)();
    ReturnValueFunc<FuncProto, int8_t> rpt_int8(LOC, "ReturnConstInt8", c, false);
    rpt_int8.test(LOC, Int8, 0);
    rpt_int8.test(LOC, Int8, 3);
    rpt_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
    rpt_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
}
TEST(omrgenExtension, ReturnConstInt16) {
    typedef int16_t (FuncProto)();
    ReturnValueFunc<FuncProto, int16_t> rpt_int16(LOC, "ReturnConstInt16", c, false);
    rpt_int16.test(LOC, Int16, 0);
    rpt_int16.test(LOC, Int16, 3);
    rpt_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
    rpt_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
}
TEST(omrgenExtension, ReturnConstInt32) {
    typedef int32_t (FuncProto)();
    ReturnValueFunc<FuncProto, int32_t> rpt_int32(LOC, "ReturnConstInt32", c, false);
    rpt_int32.test(LOC, Int32, 0);
    rpt_int32.test(LOC, Int32, 3);
    rpt_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
    rpt_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
}
TEST(omrgenExtension, ReturnConstInt64) {
    typedef int64_t (FuncProto)();
    ReturnValueFunc<FuncProto, int64_t> rpt_int64(LOC, "ReturnConstInt64", c, false);
    rpt_int64.test(LOC, Int64, 0);
    rpt_int64.test(LOC, Int64, 3);
    rpt_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
    rpt_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
}
TEST(omrgenExtension, ReturnConstFloat32) {
    typedef float (FuncProto)();
    ReturnValueFunc<FuncProto, float> rpt_float32(LOC, "ReturnConstFloat32", c, false);
    rpt_float32.test(LOC, Float32, 0);
    rpt_float32.test(LOC, Float32, 3);
    rpt_float32.test(LOC, Float32, std::numeric_limits<float>::min());
    rpt_float32.test(LOC, Float32, std::numeric_limits<float>::min());
}
TEST(omrgenExtension, ReturnConstFloat64) {
    typedef double (FuncProto)();
    ReturnValueFunc<FuncProto, double> rpt_float64(LOC, "ReturnConstFloat64", c, false);
    rpt_float64.test(LOC, Float64, 0);
    rpt_float64.test(LOC, Float64, 3);
    rpt_float64.test(LOC, Float64, std::numeric_limits<double>::min());
    rpt_float64.test(LOC, Float64, std::numeric_limits<double>::min());
}
TEST(omrgenExtension, ReturnConstAddress) {
    typedef uintptr_t (FuncProto)();
    ReturnValueFunc<FuncProto, uintptr_t> rpt_address(LOC, "ReturnConstAddress", c, false);
    rpt_address.test(LOC, Address, 0);
    rpt_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
    rpt_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
}

// Test returning the value of a paramater
template<typename FuncPrototype, typename cType>
class ReturnParameterFunc : public TestFunc {
public:
    ReturnParameterFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _type(NULL)
        , _value(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_value), _value) << "Compiled f(" << _value << ") returns " << _value;
    }
    void test(LOCATION, const Type *type, cType value) {
        _type = type;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineParameter("value", _type);
        ctx->DefineReturnType(_type);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto parmSym=ctx->LookupLocal("value"); 
        Value *p = fx()->Load(LOC, entry, parmSym);
        fx()->Return(LOC, entry, p);
        return true;
    }

private:
    const Type *_type;
    cType _value;
};

TEST(omrgenExtension, ReturnParamInt8) {
    typedef int8_t (FuncProto)(int8_t);
    ReturnParameterFunc<FuncProto, int8_t> rpp_int8(LOC, "ReturnParamInt8", c, false);
    rpp_int8.test(LOC, Int8, 0);
    rpp_int8.test(LOC, Int8, 3);
    rpp_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
    rpp_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
}
TEST(omrgenExtension, ReturnParamInt16) {
    typedef int16_t (FuncProto)(int16_t);
    ReturnParameterFunc<FuncProto, int16_t> rpp_int16(LOC, "ReturnParamInt16", c, false);
    rpp_int16.test(LOC, Int16, 0);
    rpp_int16.test(LOC, Int16, 3);
    rpp_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
    rpp_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
}
TEST(omrgenExtension, ReturnParamInt32) {
    typedef int32_t (FuncProto)(int32_t);
    ReturnParameterFunc<FuncProto, int32_t> rpp_int32(LOC, "ReturnParamInt32", c, false);
    rpp_int32.test(LOC, Int32, 0);
    rpp_int32.test(LOC, Int32, 3);
    rpp_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
    rpp_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
}
TEST(omrgenExtension, ReturnParamInt64) {
    typedef int64_t (FuncProto)(int64_t);
    ReturnParameterFunc<FuncProto, int64_t> rpp_int64(LOC, "ReturnParamInt64", c, false);
    rpp_int64.test(LOC, Int64, 0);
    rpp_int64.test(LOC, Int64, 3);
    rpp_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
    rpp_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
}
TEST(omrgenExtension, ReturnParamFloat32) {
    typedef float (FuncProto)(float);
    ReturnParameterFunc<FuncProto, float> rpp_float32(LOC, "ReturnParamFloat32", c, false);
    rpp_float32.test(LOC, Float32, 0);
    rpp_float32.test(LOC, Float32, 3);
    rpp_float32.test(LOC, Float32, std::numeric_limits<float>::min());
    rpp_float32.test(LOC, Float32, std::numeric_limits<float>::min());
}
TEST(omrgenExtension, ReturnParamDouble) {
    typedef double (FuncProto)(double);
    ReturnParameterFunc<FuncProto, double> rpp_float64(LOC, "ReturnParamFloat64", c, false);
    rpp_float64.test(LOC, Float64, 0);
    rpp_float64.test(LOC, Float64, 3);
    rpp_float64.test(LOC, Float64, std::numeric_limits<double>::min());
    rpp_float64.test(LOC, Float64, std::numeric_limits<double>::min());
}
TEST(omrgenExtension, ReturnParamAddress) {
    typedef uintptr_t (FuncProto)(uintptr_t);
    ReturnParameterFunc<FuncProto, uintptr_t> rpp_address(LOC, "ReturnParamAddress", c, false);
    rpp_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
    rpp_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
    rpp_address.test(LOC, Address, reinterpret_cast<uintptr_t>(Address));
}

// Test returning the value of a local variable where a parameter value has been stored
template<typename FuncPrototype, typename cType>
class ReturnLocalFunc : public TestFunc {
public:
    ReturnLocalFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _type(NULL)
        , _valueSym(NULL)
        , _tempSym(NULL)
        , _value(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_value), _value) << "Compiled f(" << _value << ") returns " << _value;
    }
    void test(LOCATION, const Type *type, cType value) {
        _type = type;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _valueSym = ctx->DefineParameter("value", _type);
        _tempSym = ctx->DefineLocal("temp", _type); 
        ctx->DefineReturnType(_type);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        Value *parmValue = fx()->Load(LOC, entry, _valueSym);
        fx()->Store(LOC, entry, _tempSym, parmValue);
        Value *p = fx()->Load(LOC, entry, _tempSym);
        fx()->Return(LOC, entry, p);
        return true;
    }

private:
    const Type *_type;
    Func::ParameterSymbol *_valueSym;
    Func::LocalSymbol *_tempSym;
    cType _value;
};

TEST(omrgenExtension, ReturnLocalInt8) {
    typedef int8_t (FuncProto)(int8_t);
    ReturnLocalFunc<FuncProto, int8_t> rlp_int8(LOC, "ReturnLocalInt8", c, false);
    rlp_int8.test(LOC, Int8, 0);
    rlp_int8.test(LOC, Int8, 3);
    rlp_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
    rlp_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
}
TEST(omrgenExtension, ReturnLocalInt16) {
    typedef int16_t (FuncProto)(int16_t);
    ReturnLocalFunc<FuncProto, int16_t> rlp_int16(LOC, "ReturnLocalInt16", c, false);
    rlp_int16.test(LOC, Int16, 0);
    rlp_int16.test(LOC, Int16, 3);
    rlp_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
    rlp_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
}
TEST(omrgenExtension, ReturnLocalInt32) {
    typedef int32_t (FuncProto)(int32_t);
    ReturnLocalFunc<FuncProto, int32_t> rlp_int32(LOC, "ReturnLocalInt32", c, false);
    rlp_int32.test(LOC, Int32, 0);
    rlp_int32.test(LOC, Int32, 3);
    rlp_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
    rlp_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
}
TEST(omrgenExtension, ReturnLocalInt64) {
    typedef int64_t (FuncProto)(int64_t);
    ReturnLocalFunc<FuncProto, int64_t> rlp_int64(LOC, "ReturnLocalInt64", c, false);
    rlp_int64.test(LOC, Int64, 0);
    rlp_int64.test(LOC, Int64, 3);
    rlp_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
    rlp_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
}
TEST(omrgenExtension, ReturnLocalFloat32) {
    typedef float (FuncProto)(float);
    ReturnLocalFunc<FuncProto, float> rlp_float32(LOC, "ReturnLocalFloat32", c, false);
    rlp_float32.test(LOC, Float32, 0);
    rlp_float32.test(LOC, Float32, 3);
    rlp_float32.test(LOC, Float32, std::numeric_limits<float>::min());
    rlp_float32.test(LOC, Float32, std::numeric_limits<float>::min());
}
TEST(omrgenExtension, ReturnLocalFloat64) {
    typedef double (FuncProto)(double);
    ReturnLocalFunc<FuncProto, double> rlp_float64(LOC, "ReturnLocalFloat64", c, false);
    rlp_float64.test(LOC, Float64, 0);
    rlp_float64.test(LOC, Float64, 3);
    rlp_float64.test(LOC, Float64, std::numeric_limits<double>::min());
    rlp_float64.test(LOC, Float64, std::numeric_limits<double>::min());
}
TEST(omrgenExtension, ReturnLocalAddress) {
    typedef uintptr_t (FuncProto)(uintptr_t);
    ReturnLocalFunc<FuncProto, uintptr_t> rlp_address(LOC, "ReturnLocalAddress", c, false);
    rlp_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
    rlp_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
    rlp_address.test(LOC, Address, reinterpret_cast<uintptr_t>(Address));
}

// Test returning a converted parameter value
template<typename FuncPrototype, typename cTypeFrom, typename cTypeTo>
class ConvertToFunc : public TestFunc {
public:
    ConvertToFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _typeFrom(NULL)
        , _typeTo(NULL)
        , _valueSym(NULL)
        , _valueFrom(0)
        , _expectedResult(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_valueFrom), _expectedResult) << "Compiled f(" << _valueFrom << ") returns " << _expectedResult;
    }
    void test(LOCATION, bool doCompile, const Type *typeFrom, const Type *typeTo, cTypeFrom value, cTypeTo expectedResult) {
        _typeFrom = typeFrom;
        _typeTo = typeTo;
        _valueFrom = value;
        _expectedResult = expectedResult;
        if (doCompile)
            compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _valueSym = ctx->DefineParameter("value", _typeFrom);
        ctx->DefineReturnType(_typeTo);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        Value *parmValue = fx()->Load(LOC, entry, _valueSym);
        Value *convertedValue = bx()->ConvertTo(LOC, entry, _typeTo, parmValue);
        fx()->Return(LOC, entry, convertedValue);
        return true;
    }

private:
    const Type *_typeFrom;
    const Type *_typeTo;
    Func::ParameterSymbol *_valueSym;
    cTypeTo _valueFrom;
    cTypeFrom _expectedResult;
};

// test converting Int8 to other primitive types
TEST(omrgenExtension, ConvertInt8ToInt8) {
    typedef int8_t (FuncProtoInt8)(int8_t);
    ConvertToFunc<FuncProtoInt8, int8_t, int8_t> ct_int8_int8(LOC, "ConvertInt8ToInt8", c, false);
    ct_int8_int8.test(LOC, true, Int8, Int8, 0, 0);
    ct_int8_int8.test(LOC, false, Int8, Int8, 3, 3);
    ct_int8_int8.test(LOC, false, Int8, Int8, std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_int8_int8.test(LOC, false, Int8, Int8, std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertInt8ToInt16) {
    typedef int16_t (FuncProtoInt16)(int8_t);
    ConvertToFunc<FuncProtoInt16, int8_t, int16_t> ct_int8_int16(LOC, "ConvertInt8ToInt16", c, false);
    ct_int8_int16.test(LOC, true, Int8, Int16, 0, (int16_t)0);
    ct_int8_int16.test(LOC, false, Int8, Int16, 3, (int16_t)3);
    ct_int8_int16.test(LOC, false, Int8, Int16, std::numeric_limits<int8_t>::min(), (int16_t) std::numeric_limits<int8_t>::min());
    ct_int8_int16.test(LOC, false, Int8, Int16, std::numeric_limits<int8_t>::max(), (int16_t) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertInt8ToInt32) {
    typedef int32_t (FuncProtoInt32)(int8_t);
    ConvertToFunc<FuncProtoInt32, int8_t, int32_t> ct_int8_int32(LOC, "ConvertInt8ToInt32", c, false);
    ct_int8_int32.test(LOC, true, Int8, Int32, 0, (int32_t)0);
    ct_int8_int32.test(LOC, false, Int8, Int32, 3, (int32_t)3);
    ct_int8_int32.test(LOC, false, Int8, Int32, std::numeric_limits<int8_t>::min(), (int32_t) std::numeric_limits<int8_t>::min());
    ct_int8_int32.test(LOC, false, Int8, Int32, std::numeric_limits<int8_t>::max(), (int32_t) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertInt8ToInt64) {
    typedef int64_t (FuncProtoInt64)(int8_t);
    ConvertToFunc<FuncProtoInt64, int8_t, int64_t> ct_int8_int64(LOC, "ConvertInt8ToInt64", c, false);
    ct_int8_int64.test(LOC, true, Int8, Int64, 0, (int64_t)0);
    ct_int8_int64.test(LOC, false, Int8, Int64, 3, (int64_t)3);
    ct_int8_int64.test(LOC, false, Int8, Int64, std::numeric_limits<int8_t>::min(), (int64_t) std::numeric_limits<int8_t>::min());
    ct_int8_int64.test(LOC, false, Int8, Int64, std::numeric_limits<int8_t>::max(), (int64_t) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertInt8ToFloat32) {
    typedef float (FuncProtoFloat32)(int8_t);
    ConvertToFunc<FuncProtoFloat32, int8_t, float> ct_int8_float(LOC, "ConvertInt8ToFloat32", c, false);
    ct_int8_float.test(LOC, true, Int8, Float32, 0, (float)0);
    ct_int8_float.test(LOC, false, Int8, Float32, 3, (float)3);
    ct_int8_float.test(LOC, false, Int8, Float32, std::numeric_limits<int8_t>::min(), (float) std::numeric_limits<int8_t>::min());
    ct_int8_float.test(LOC, false, Int8, Float32, std::numeric_limits<int8_t>::max(), (float) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertInt8ToFloat64) {
    typedef double (FuncProtoFloat64)(int8_t);
    ConvertToFunc<FuncProtoFloat64, int8_t, float> ct_int8_double(LOC, "ConvertInt8ToFloat64", c, false);
    ct_int8_double.test(LOC, true, Int8, Float64, 0, (float)0);
    ct_int8_double.test(LOC, false, Int8, Float64, 3, (float)3);
    ct_int8_double.test(LOC, false, Int8, Float64, std::numeric_limits<int8_t>::min(), (double) std::numeric_limits<int8_t>::min());
    ct_int8_double.test(LOC, false, Int8, Float64, std::numeric_limits<int8_t>::max(), (double) std::numeric_limits<int8_t>::max());
}

// test converting Int16 to other primitive types
TEST(omrgenExtension, ConvertInt16ToInt8) {
    typedef int8_t (FuncProtoInt8)(int16_t);
    ConvertToFunc<FuncProtoInt8, int16_t, int8_t> ct_int16_int8(LOC, "ConvertInt16ToInt8", c, false);
    ct_int16_int8.test(LOC, true, Int16, Int8, 0, (int8_t)0);
    ct_int16_int8.test(LOC, false, Int16, Int8, 3, (int8_t)3);
    ct_int16_int8.test(LOC, false, Int16, Int8, std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_int16_int8.test(LOC, false, Int16, Int8, std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertInt16ToInt16) {
    typedef int16_t (FuncProtoInt16)(int16_t);
    ConvertToFunc<FuncProtoInt16, int16_t, int16_t> ct_int16_int16(LOC, "ConvertInt16ToInt16", c, false);
    ct_int16_int16.test(LOC, true, Int16, Int16, 0, (int16_t)0);
    ct_int16_int16.test(LOC, false, Int16, Int16, 3, (int16_t)3);
    ct_int16_int16.test(LOC, false, Int16, Int16, std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_int16_int16.test(LOC, false, Int16, Int16, std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, ConvertInt16ToInt32) {
    typedef int32_t (FuncProtoInt32)(int16_t);
    ConvertToFunc<FuncProtoInt32, int16_t, int32_t> ct_int16_int32(LOC, "ConvertInt16ToInt32", c, false);
    ct_int16_int32.test(LOC, true, Int16, Int32, 0, (int32_t)0);
    ct_int16_int32.test(LOC, false, Int16, Int32, 3, (int32_t)3);
    ct_int16_int32.test(LOC, false, Int16, Int32, std::numeric_limits<int16_t>::min(), (int32_t) std::numeric_limits<int16_t>::min());
    ct_int16_int32.test(LOC, false, Int16, Int32, std::numeric_limits<int16_t>::max(), (int32_t) std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, ConvertInt16ToInt64) {
    typedef int64_t (FuncProtoInt64)(int16_t);
    ConvertToFunc<FuncProtoInt64, int16_t, int64_t> ct_int16_int64(LOC, "ConvertInt16ToInt64", c, false);
    ct_int16_int64.test(LOC, true, Int16, Int64, 0, (int64_t)0);
    ct_int16_int64.test(LOC, false, Int16, Int64, 3, (int64_t)3);
    ct_int16_int64.test(LOC, false, Int16, Int64, std::numeric_limits<int16_t>::min(), (int64_t) std::numeric_limits<int16_t>::min());
    ct_int16_int64.test(LOC, false, Int16, Int64, std::numeric_limits<int16_t>::max(), (int64_t) std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, ConvertInt16ToFloat32) {
    typedef float (FuncProtoFloat32)(int16_t);
    ConvertToFunc<FuncProtoFloat32, int16_t, float> ct_int16_float32(LOC, "ConvertInt16ToFloat32", c, false);
    ct_int16_float32.test(LOC, true, Int16, Float32, 0, (float)0);
    ct_int16_float32.test(LOC, false, Int16, Float32, 3, (float)3);
    ct_int16_float32.test(LOC, false, Int16, Float32, std::numeric_limits<int16_t>::min(), (float)std::numeric_limits<int16_t>::min());
    ct_int16_float32.test(LOC, false, Int16, Float32, std::numeric_limits<int16_t>::max(), (float)std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, ConvertInt16ToFloat64) {
    typedef double (FuncProtoFloat64)(int16_t);
    ConvertToFunc<FuncProtoFloat64, int16_t, double> ct_int16_float64(LOC, "ConvertInt16ToFloat64", c, false);
    ct_int16_float64.test(LOC, true, Int16, Float64, 0, (double)0);
    ct_int16_float64.test(LOC, false, Int16, Float64, 3, (double)3);
    ct_int16_float64.test(LOC, false, Int16, Float64, std::numeric_limits<int16_t>::min(), (double)std::numeric_limits<int16_t>::min());
    ct_int16_float64.test(LOC, false, Int16, Float64, std::numeric_limits<int16_t>::max(), (double)std::numeric_limits<int16_t>::max());
}

// test converting Int32 to other primitive types
TEST(omrgenExtension, ConvertInt32ToInt8) {
    typedef int8_t (FuncProtoInt8)(int32_t);
    ConvertToFunc<FuncProtoInt8, int32_t, int8_t> ct_int32_int8(LOC, "ConvertInt32ToInt8", c, false);
    ct_int32_int8.test(LOC, true, Int32, Int8, (int32_t)0, (int8_t)0);
    ct_int32_int8.test(LOC, false, Int32, Int8, (int32_t)3, (int8_t)3);
    ct_int32_int8.test(LOC, false, Int32, Int8, (int32_t)std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_int32_int8.test(LOC, false, Int32, Int8, (int32_t)std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertInt32ToInt16) {
    typedef int16_t (FuncProtoInt16)(int32_t);
    ConvertToFunc<FuncProtoInt16, int32_t, int16_t> ct_int32_int16(LOC, "ConvertInt32ToInt16", c, false);
    ct_int32_int16.test(LOC, true, Int32, Int16, (int32_t)0, (int16_t)0);
    ct_int32_int16.test(LOC, false, Int32, Int16, (int32_t)3, (int16_t)3);
    ct_int32_int16.test(LOC, false, Int32, Int16, (int32_t)std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_int32_int16.test(LOC, false, Int32, Int16, (int32_t)std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, ConvertInt32ToInt32) {
    typedef int32_t (FuncProtoInt32)(int32_t);
    ConvertToFunc<FuncProtoInt32, int32_t, int32_t> ct_int32_int32(LOC, "ConvertInt32ToInt32", c, false);
    ct_int32_int32.test(LOC, true, Int32, Int32, (int32_t)0, (int32_t)0);
    ct_int32_int32.test(LOC, false, Int32, Int32, (int32_t)3, (int32_t)3);
    ct_int32_int32.test(LOC, false, Int32, Int32, (int32_t)std::numeric_limits<int32_t>::min(), (int32_t) std::numeric_limits<int32_t>::min());
    ct_int32_int32.test(LOC, false, Int32, Int32, (int32_t)std::numeric_limits<int32_t>::max(), (int32_t) std::numeric_limits<int32_t>::max());
}
TEST(omrgenExtension, ConvertInt32ToInt64) {
    typedef int64_t (FuncProtoInt64)(int32_t);
    ConvertToFunc<FuncProtoInt64, int32_t, int64_t> ct_int32_int64(LOC, "ConvertInt32ToInt64", c, false);
    ct_int32_int64.test(LOC, true, Int32, Int64, (int32_t)0, (int64_t)0);
    ct_int32_int64.test(LOC, false, Int32, Int64, (int32_t)3, (int64_t)3);
    ct_int32_int64.test(LOC, false, Int32, Int64, (int32_t)std::numeric_limits<int32_t>::min(), (int64_t) std::numeric_limits<int32_t>::min());
    ct_int32_int64.test(LOC, false, Int32, Int64, (int32_t)std::numeric_limits<int32_t>::max(), (int64_t) std::numeric_limits<int32_t>::max());
}
TEST(omrgenExtension, ConvertInt32ToFloat32) {
    typedef float (FuncProtoFloat32)(int32_t);
    ConvertToFunc<FuncProtoFloat32, int32_t, float> ct_int32_float32(LOC, "ConvertInt32ToFloat32", c, false);
    ct_int32_float32.test(LOC, true, Int32, Float32, (int32_t)0, (float)0);
    ct_int32_float32.test(LOC, false, Int32, Float32, (int32_t)3, (float)3);
    ct_int32_float32.test(LOC, false, Int32, Float32, (int32_t)std::numeric_limits<int32_t>::min(), (float)std::numeric_limits<int32_t>::min());
    ct_int32_float32.test(LOC, false, Int32, Float32, (int32_t)std::numeric_limits<int32_t>::max(), (float)std::numeric_limits<int32_t>::max());
}
TEST(omrgenExtension, ConvertInt32ToFloat64) {
    typedef double (FuncProtoFloat64)(int32_t);
    ConvertToFunc<FuncProtoFloat64, int32_t, double> ct_int32_float64(LOC, "ConvertInt32ToFloat64", c, false);
    ct_int32_float64.test(LOC, true, Int32, Float64, (int32_t)0, (double)0);
    ct_int32_float64.test(LOC, false, Int32, Float64, (int32_t)3, (double)3);
    ct_int32_float64.test(LOC, false, Int32, Float64, (int32_t)std::numeric_limits<int32_t>::min(), (double)std::numeric_limits<int32_t>::min());
    ct_int32_float64.test(LOC, false, Int32, Float64, (int32_t)std::numeric_limits<int32_t>::max(), (double)std::numeric_limits<int32_t>::max());
}

// test converting Int64 to other primitive types
TEST(omrgenExtension, ConvertInt64ToInt8) {
    typedef int8_t (FuncProtoInt8)(int64_t);
    ConvertToFunc<FuncProtoInt8, int64_t, int8_t> ct_int64_int8(LOC, "ConvertInt64ToInt8", c, false);
    ct_int64_int8.test(LOC, true, Int64, Int8, (int64_t)0, (int8_t)0);
    ct_int64_int8.test(LOC, false, Int64, Int8, (int64_t)3, (int8_t)3);
    ct_int64_int8.test(LOC, false, Int64, Int8, (int64_t)std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_int64_int8.test(LOC, false, Int64, Int8, (int64_t)std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertInt64ToInt16) {
    typedef int16_t (FuncProtoInt16)(int64_t);
    ConvertToFunc<FuncProtoInt16, int64_t, int16_t> ct_int64_int16(LOC, "ConvertInt64ToInt16", c, false);
    ct_int64_int16.test(LOC, true, Int64, Int16, (int64_t)0, (int16_t)0);
    ct_int64_int16.test(LOC, false, Int64, Int16, (int64_t)3, (int16_t)3);
    ct_int64_int16.test(LOC, false, Int64, Int16, (int64_t)std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_int64_int16.test(LOC, false, Int64, Int16, (int64_t)std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, ConvertInt64ToInt32) {
    typedef int32_t (FuncProtoInt32)(int64_t);
    ConvertToFunc<FuncProtoInt32, int64_t, int32_t> ct_int64_int32(LOC, "ConvertInt64ToInt32", c, false);
    ct_int64_int32.test(LOC, true, Int64, Int32, (int64_t)0, (int32_t)0);
    ct_int64_int32.test(LOC, false, Int64, Int32, (int64_t)3, (int32_t)3);
    ct_int64_int32.test(LOC, false, Int64, Int32, (int64_t)std::numeric_limits<int32_t>::min(), (int32_t) std::numeric_limits<int32_t>::min());
    ct_int64_int32.test(LOC, false, Int64, Int32, (int64_t)std::numeric_limits<int32_t>::max(), (int32_t) std::numeric_limits<int32_t>::max());
}
TEST(omrgenExtension, ConvertInt64ToInt64) {
    typedef int64_t (FuncProtoInt64)(int64_t);
    ConvertToFunc<FuncProtoInt64, int64_t, int64_t> ct_int64_int64(LOC, "ConvertInt64ToInt64", c, false);
    ct_int64_int64.test(LOC, true, Int64, Int64, (int64_t)0, (int64_t)0);
    ct_int64_int64.test(LOC, false, Int64, Int64, (int64_t)3, (int64_t)3);
    ct_int64_int64.test(LOC, false, Int64, Int64, (int64_t)std::numeric_limits<int64_t>::min(), (int64_t) std::numeric_limits<int64_t>::min());
    ct_int64_int64.test(LOC, false, Int64, Int64, (int64_t)std::numeric_limits<int64_t>::max(), (int64_t) std::numeric_limits<int64_t>::max());
}
TEST(omrgenExtension, ConvertInt64ToFloat32) {
    typedef float (FuncProtoFloat32)(int64_t);
    ConvertToFunc<FuncProtoFloat32, int64_t, float> ct_int64_float32(LOC, "ConvertInt64ToFloat32", c, false);
    ct_int64_float32.test(LOC, true, Int64, Float32, (int64_t)0, (float)0);
    ct_int64_float32.test(LOC, false, Int64, Float32, (int64_t)3, (float)3);
    ct_int64_float32.test(LOC, false, Int64, Float32, (int64_t)std::numeric_limits<int64_t>::min(), (float)std::numeric_limits<int64_t>::min());
    ct_int64_float32.test(LOC, false, Int64, Float32, (int64_t)std::numeric_limits<int64_t>::max(), (float)std::numeric_limits<int64_t>::max());
}
TEST(omrgenExtension, ConvertInt64ToFloat64) {
    typedef double (FuncProtoFloat64)(int64_t);
    ConvertToFunc<FuncProtoFloat64, int64_t, double> ct_int64_float64(LOC, "ConvertInt64ToFloat64", c, false);
    ct_int64_float64.test(LOC, true, Int64, Float64, (int64_t)0, (double)0);
    ct_int64_float64.test(LOC, false, Int64, Float64, (int64_t)3, (double)3);
    ct_int64_float64.test(LOC, false, Int64, Float64, (int64_t)std::numeric_limits<int64_t>::min(), (double)std::numeric_limits<int64_t>::min());
    ct_int64_float64.test(LOC, false, Int64, Float64, (int64_t)std::numeric_limits<int64_t>::max(), (double)std::numeric_limits<int64_t>::max());
}

// test converting Float32 to other primitive types
TEST(omrgenExtension, ConvertFloat32ToInt8) {
    typedef int8_t (FuncProtoInt8)(float);
    ConvertToFunc<FuncProtoInt8, float, int8_t> ct_float32_int8(LOC, "ConvertFloat32ToInt8", c, false);
    ct_float32_int8.test(LOC, true, Float32, Int8, (float)0, (int8_t)0);
    ct_float32_int8.test(LOC, false, Float32, Int8, (float)3, (int8_t)3);
    ct_float32_int8.test(LOC, false, Float32, Int8, (float)std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_float32_int8.test(LOC, false, Float32, Int8, (float)std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertFloat32ToInt16) {
    typedef int16_t (FuncProtoInt16)(float);
    ConvertToFunc<FuncProtoInt16, float, int16_t> ct_float32_int16(LOC, "ConvertFloat32ToInt16", c, false);
    ct_float32_int16.test(LOC, true, Float32, Int16, (float)0, (int16_t)0);
    ct_float32_int16.test(LOC, false, Float32, Int16, (float)3, (int16_t)3);
    ct_float32_int16.test(LOC, false, Float32, Int16, (float)std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_float32_int16.test(LOC, false, Float32, Int16, (float)std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, ConvertFloat32ToInt32) {
    typedef int32_t (FuncProtoInt32)(float);
    ConvertToFunc<FuncProtoInt32, float, int32_t> ct_float32_int32(LOC, "ConvertFloat32ToIn32", c, false);
    ct_float32_int32.test(LOC, true, Float32, Int32, (float)0, (int32_t)0);
    ct_float32_int32.test(LOC, false, Float32, Int32, (float)3, (int32_t)3);
    ct_float32_int32.test(LOC, false, Float32, Int32, (float)std::numeric_limits<int32_t>::min(), (int32_t)(float)std::numeric_limits<int32_t>::min());
    ct_float32_int32.test(LOC, false, Float32, Int32, (float)std::numeric_limits<int32_t>::max(), (int32_t)(float)std::numeric_limits<int32_t>::max());
}
TEST(omrgenExtension, ConvertFloat32ToInt64) {
    typedef int64_t (FuncProtoInt64)(float);
    ConvertToFunc<FuncProtoInt64, float, int64_t> ct_float32_int64(LOC, "ConvertFloat32ToInt64", c, false);
    ct_float32_int64.test(LOC, true, Float32, Int64, (float)0, (int64_t)0);
    ct_float32_int64.test(LOC, false, Float32, Int64, (float)3, (int64_t)3);
    ct_float32_int64.test(LOC, false, Float32, Int64, (float)std::numeric_limits<int32_t>::min(), (int64_t)(float)std::numeric_limits<int32_t>::min());
    ct_float32_int64.test(LOC, false, Float32, Int64, (float)std::numeric_limits<int32_t>::max(), (int64_t)(float)std::numeric_limits<int32_t>::max());
}
TEST(omrgenExtension, ConvertFloat32ToFloat32) {
    typedef float (FuncProtoFloat32)(float);
    ConvertToFunc<FuncProtoFloat32, float, float> ct_float32_float32(LOC, "ConvertFloat32ToFloat32", c, false);
    ct_float32_float32.test(LOC, true, Float32, Float32, (float)0, (float)0);
    ct_float32_float32.test(LOC, false, Float32, Float32, (float)3, (float)3);
    ct_float32_float32.test(LOC, false, Float32, Float32, (float)std::numeric_limits<float>::min(), (float)std::numeric_limits<float>::min());
    ct_float32_float32.test(LOC, false, Float32, Float32, (float)std::numeric_limits<float>::max(), (float)std::numeric_limits<float>::max());
}
TEST(omrgenExtension, ConvertFloat32ToFloat64) {
    typedef double (FuncProtoFloat64)(float);
    ConvertToFunc<FuncProtoFloat64, float, double> ct_float32_float64(LOC, "ConvertFloat32ToFloat64", c, false);
    ct_float32_float64.test(LOC, true, Float32, Float64, (float)0, (double)0);
    ct_float32_float64.test(LOC, false, Float32, Float64, (float)3, (double)3);
    ct_float32_float64.test(LOC, false, Float32, Float64, (float)std::numeric_limits<float>::min(), (double)std::numeric_limits<float>::min());
    ct_float32_float64.test(LOC, false, Float32, Float64, (float)std::numeric_limits<float>::max(), (double)std::numeric_limits<float>::max());
}

// test converting Float64 to other primitive types
TEST(omrgenExtension, ConvertFloat64ToInt8) {
    typedef int8_t (FuncProtoInt8)(double);
    ConvertToFunc<FuncProtoInt8, double, int8_t> ct_float64_int8(LOC, "ConvertFloat64ToInt8", c, false);
    ct_float64_int8.test(LOC, true, Float64, Int8, (double)0, (int8_t)0);
    ct_float64_int8.test(LOC, false, Float64, Int8, (double)3, (int8_t)3);
    ct_float64_int8.test(LOC, false, Float64, Int8, (double)std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_float64_int8.test(LOC, false, Float64, Int8, (double)std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, ConvertFloat64ToInt16) {
    typedef int16_t (FuncProtoInt16)(double);
    ConvertToFunc<FuncProtoInt16, double, int16_t> ct_float64_int16(LOC, "ConvertFloat64ToInt16", c, false);
    ct_float64_int16.test(LOC, true, Float64, Int16, (double)0, (int16_t)0);
    ct_float64_int16.test(LOC, false, Float64, Int16, (double)3, (int16_t)3);
    ct_float64_int16.test(LOC, false, Float64, Int16, (double)std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_float64_int16.test(LOC, false, Float64, Int16, (double)std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, ConvertFloat64ToInt32) {
    typedef int32_t (FuncProtoInt32)(double);
    ConvertToFunc<FuncProtoInt32, double, int32_t> ct_float64_int32(LOC, "ConvertFloat64ToInt32", c, false);
    ct_float64_int32.test(LOC, true, Float64, Int32, (double)0, (int32_t)0);
    ct_float64_int32.test(LOC, false, Float64, Int32, (double)3, (int32_t)3);
    ct_float64_int32.test(LOC, false, Float64, Int32, (double)std::numeric_limits<int32_t>::min(), (int32_t)(float)std::numeric_limits<int32_t>::min());
    ct_float64_int32.test(LOC, false, Float64, Int32, (double)std::numeric_limits<int32_t>::max(), (int32_t)(float)std::numeric_limits<int32_t>::max());
}
TEST(omrgenExtension, ConvertFloat64ToInt64) {
    typedef int64_t (FuncProtoInt64)(double);
    ConvertToFunc<FuncProtoInt64, double, int64_t> ct_float64_int64(LOC, "ConvertFloat64ToInt64", c, false);
    ct_float64_int64.test(LOC, true, Float64, Int64, (double)0, (int64_t)0);
    ct_float64_int64.test(LOC, false, Float64, Int64, (double)3, (int64_t)3);
    ct_float64_int64.test(LOC, false, Float64, Int64, (double)std::numeric_limits<int64_t>::min(), (int64_t)(float)std::numeric_limits<int64_t>::min());
    ct_float64_int64.test(LOC, false, Float64, Int64, (double)std::numeric_limits<int64_t>::max(), (int64_t)(float)std::numeric_limits<int64_t>::max());
}
TEST(omrgenExtension, ConvertFloat64ToFloat32) {
    typedef float (FuncProtoFloat32)(double);
    ConvertToFunc<FuncProtoFloat32, double, float> ct_float64_float32(LOC, "ConvertFloat64ToFloat32", c, false);
    ct_float64_float32.test(LOC, true, Float64, Float32, (double)0, (float)0);
    ct_float64_float32.test(LOC, false, Float64, Float32, (double)3, (float)3);
    ct_float64_float32.test(LOC, false, Float64, Float32, (double)std::numeric_limits<float>::min(), (float)std::numeric_limits<float>::min());
    ct_float64_float32.test(LOC, false, Float64, Float32, (double)std::numeric_limits<float>::max(), (float)std::numeric_limits<float>::max());
}
TEST(omrgenExtension, ConvertFloat64ToFloat64) {
    typedef double (FuncProtoFloat64)(double);
    ConvertToFunc<FuncProtoFloat64, double, double> ct_float64_float64(LOC, "ConvertFloat64ToFloat64", c, false);
    ct_float64_float64.test(LOC, true, Float64, Float64, (double)0, (double)0);
    ct_float64_float64.test(LOC, false, Float64, Float64, (double)3, (double)3);
    ct_float64_float64.test(LOC, false, Float64, Float64, (double)std::numeric_limits<double>::min(), (double)std::numeric_limits<double>::min());
    ct_float64_float64.test(LOC, false, Float64, Float64, (double)std::numeric_limits<double>::max(), (double)std::numeric_limits<double>::max());
}

// Test returning the value of a paramater that has a pointer type using LoadAt
template<typename FuncPrototype, typename cType>
class ReturnPointerParameterFunc : public TestFunc {
public:
    ReturnPointerParameterFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _type(NULL)
        , _value(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        cType *pValue = &this->_value;
        EXPECT_EQ(f(pValue), _value) << "Compiled f(" << _value << ") returns " << _value;
    }
    void test(LOCATION, const Type *type, cType value) {
        _type = type;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineParameter("value", bx()->PointerTo(PASSLOC, comp, _type));
        ctx->DefineReturnType(_type);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto parmSym=ctx->LookupLocal("value"); 
        Value *addr = fx()->Load(LOC, entry, parmSym);
        Value *v = bx()->LoadAt(LOC, entry, addr);
        fx()->Return(LOC, entry, v);
        return true;
    }

protected:
    const Type *_type;
    cType _value;
};

TEST(omrgenExtension, ReturnPointerParam_pInt8) {
    typedef int8_t (FuncProto)(int8_t *);
    ReturnPointerParameterFunc<FuncProto, int8_t> rpp_int8(LOC, "ReturnPointerParam_pInt8", c, false);
    rpp_int8.test(LOC, Int8, static_cast<int8_t>(0));
    rpp_int8.test(LOC, Int8, static_cast<int8_t>(3));
    rpp_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
    rpp_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
}
TEST(omrgenExtension, ReturnPointerParam_pInt16) {
    typedef int16_t (FuncProto)(int16_t *);
    ReturnPointerParameterFunc<FuncProto, int16_t> rpp_int16(LOC, "ReturnPointerParam_pInt16", c, false);
    rpp_int16.test(LOC, Int16, static_cast<int16_t>(0));
    rpp_int16.test(LOC, Int16, static_cast<int16_t>(3));
    rpp_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
    rpp_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
}
TEST(omrgenExtension, ReturnPointerParam_pInt32) {
    typedef int32_t (FuncProto)(int32_t *);
    ReturnPointerParameterFunc<FuncProto, int32_t> rpp_int32(LOC, "ReturnPointerParam_pInt32", c, false);
    rpp_int32.test(LOC, Int32, static_cast<int32_t>(0));
    rpp_int32.test(LOC, Int32, static_cast<int32_t>(3));
    rpp_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
    rpp_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
}
TEST(omrgenExtension, ReturnPointerParam_pInt64) {
    typedef int64_t (FuncProto)(int64_t *);
    ReturnPointerParameterFunc<FuncProto, int64_t> rpp_int64(LOC, "ReturnPointerParam_pInt64", c, false);
    rpp_int64.test(LOC, Int64, static_cast<int64_t>(0));
    rpp_int64.test(LOC, Int64, static_cast<int64_t>(3));
    rpp_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
    rpp_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
}
TEST(omrgenExtension, ReturnPointerParam_pFloat32) {
    typedef float (FuncProto)(float *);
    ReturnPointerParameterFunc<FuncProto, float> rpp_float32(LOC, "ReturnPointerParam_pFloat32", c, false);
    rpp_float32.test(LOC, Float32, static_cast<float>(0));
    rpp_float32.test(LOC, Float32, static_cast<float>(3));
    rpp_float32.test(LOC, Float32, std::numeric_limits<float>::min());
    rpp_float32.test(LOC, Float32, std::numeric_limits<float>::min());
}
TEST(omrgenExtension, ReturnPointerParam_pDouble) {
    typedef double (FuncProto)(double *);
    ReturnPointerParameterFunc<FuncProto, double> rpp_float64(LOC, "ReturnPointerParam_pFloat64", c, false);
    rpp_float64.test(LOC, Float64, static_cast<double>(0));
    rpp_float64.test(LOC, Float64, static_cast<double>(3));
    rpp_float64.test(LOC, Float64, std::numeric_limits<double>::min());
    rpp_float64.test(LOC, Float64, std::numeric_limits<double>::min());
}
TEST(omrgenExtension, ReturnPointerParam_pAddress) {
    typedef uintptr_t (FuncProto)(uintptr_t *);
    ReturnPointerParameterFunc<FuncProto, uintptr_t> rpp_address(LOC, "ReturnPointerParam_pAddress", c, false);
    rpp_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
    rpp_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
    rpp_address.test(LOC, Address, reinterpret_cast<uintptr_t>(Address));
}

// Test returning the value of a paramater that has a pointer to pointer type using LoadAt
template<typename FuncPrototype, typename cType>
class ReturnPointerToPointerParameterFunc : public TestFunc {
public:
    ReturnPointerToPointerParameterFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _type(NULL)
        , _value(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        cType *pValue = &this->_value;
        EXPECT_EQ(f(&pValue), pValue) << "Compiled f(" << (intptr_t)(&pValue) << ") returns " << (intptr_t)(pValue);
        EXPECT_EQ((*pValue), _value) << "and (*" << (intptr_t)(pValue) << ") is still " << _value;
    }
    void test(LOCATION, const Type *type, cType value) {
        _type = type;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        const Type *pType = bx()->PointerTo(PASSLOC, comp, _type);
        ctx->DefineParameter("value", bx()->PointerTo(PASSLOC, comp, pType));
        ctx->DefineReturnType(pType);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto parmSym=ctx->LookupLocal("value"); 
        Value *addr = fx()->Load(LOC, entry, parmSym);
        Value *v = bx()->LoadAt(LOC, entry, addr);
        fx()->Return(LOC, entry, v);
        return true;
    }

protected:
    const Type *_type;
    cType _value;
};

TEST(omrgenExtension, ReturnPointerToPointerParam_ppInt8) {
    typedef int8_t *(FuncProto)(int8_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, int8_t> rpp_int8(LOC, "ReturnPointerToPointerParam_ppInt8", c, false);
    rpp_int8.test(LOC, Int8, static_cast<int8_t>(0));
    rpp_int8.test(LOC, Int8, static_cast<int8_t>(3));
    rpp_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
    rpp_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min());
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppInt16) {
    typedef int16_t *(FuncProto)(int16_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, int16_t> rpp_int16(LOC, "ReturnPointerToPointerParam_ppInt16", c, false);
    rpp_int16.test(LOC, Int16, static_cast<int16_t>(0));
    rpp_int16.test(LOC, Int16, static_cast<int16_t>(3));
    rpp_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
    rpp_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min());
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppInt32) {
    typedef int32_t *(FuncProto)(int32_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, int32_t> rpp_int32(LOC, "ReturnPointerToPointerParam_ppInt32", c, false);
    rpp_int32.test(LOC, Int32, static_cast<int32_t>(0));
    rpp_int32.test(LOC, Int32, static_cast<int32_t>(3));
    rpp_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
    rpp_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min());
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppInt64) {
    typedef int64_t *(FuncProto)(int64_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, int64_t> rpp_int64(LOC, "ReturnPointerToPointerParam_ppInt64", c, false);
    rpp_int64.test(LOC, Int64, static_cast<int64_t>(0));
    rpp_int64.test(LOC, Int64, static_cast<int64_t>(3));
    rpp_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
    rpp_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min());
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppFloat32) {
    typedef float *(FuncProto)(float **);
    ReturnPointerToPointerParameterFunc<FuncProto, float> rpp_float32(LOC, "ReturnPointerToPointerParam_ppFloat32", c, false);
    rpp_float32.test(LOC, Float32, static_cast<float>(0));
    rpp_float32.test(LOC, Float32, static_cast<float>(3));
    rpp_float32.test(LOC, Float32, std::numeric_limits<float>::min());
    rpp_float32.test(LOC, Float32, std::numeric_limits<float>::min());
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppDouble) {
    typedef double *(FuncProto)(double **);
    ReturnPointerToPointerParameterFunc<FuncProto, double> rpp_float64(LOC, "ReturnPointerToPointerParam_ppFloat64", c, false);
    rpp_float64.test(LOC, Float64, static_cast<double>(0));
    rpp_float64.test(LOC, Float64, static_cast<double>(3));
    rpp_float64.test(LOC, Float64, std::numeric_limits<double>::min());
    rpp_float64.test(LOC, Float64, std::numeric_limits<double>::min());
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppAddress) {
    typedef uintptr_t *(FuncProto)(uintptr_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, uintptr_t> rpp_address(LOC, "ReturnPointerToPointerParam_ppAddress", c, false);
    rpp_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
    rpp_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min());
    rpp_address.test(LOC, Address, reinterpret_cast<uintptr_t>(Address));
}


// Base class for operating on two numbers together of the given types producing the given type
template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class BinaryOpFunc : public TestFunc {
public:
    BinaryOpFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _leftType(NULL)
        , _leftValue(0)
        , _rightType(NULL)
        , _rightValue(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_leftValue,_rightValue), _resultValue) << "Compiled f(" << _leftValue << ", " << _rightValue << ") returns " << _resultValue;
    }
    void test(LOCATION, const Type *leftType, left_cType leftValue, const Type *rightType, right_cType rightValue, const Type *resultType, result_cType resultValue) {
        _leftType = leftType;
        _leftValue = leftValue;
        _rightType = rightType;
        _rightValue = rightValue;
        _resultType = resultType;
        _resultValue = resultValue;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineParameter("left", _leftType);
        ctx->DefineParameter("right", _rightType);
        ctx->DefineReturnType(_resultType);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto leftSym=ctx->LookupLocal("left"); 
        Value *leftValue = fx()->Load(LOC, entry, leftSym);
        auto rightSym=ctx->LookupLocal("right"); 
        Value *rightValue = fx()->Load(LOC, entry, rightSym);
        Value *result = doBinaryOp(PASSLOC, entry, leftValue, rightValue);
        fx()->Return(LOC, entry, result);
        return true;
    }

protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right)=0;

    const Type *_leftType;
    left_cType _leftValue;
    const Type *_rightType;
    right_cType _rightValue;
    const Type *_resultType;
    result_cType _resultValue;
};

// Test adding two numbers together of the given types
template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class AddFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    AddFunc(LOCATION, String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->Add(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, AddInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    AddFunc<FuncProto, int8_t, int8_t, int8_t> add_int8s(LOC, "add_int8s", c, false);
    add_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    add_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(3));
    add_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(3));
    add_int8s.test(LOC, Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(0));
    add_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(0));
    add_int8s.test(LOC, Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(-6));
    add_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::min(), Int8, static_cast<int8_t>(0), Int8, std::numeric_limits<int8_t>::min());
    add_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, std::numeric_limits<int8_t>::min(), Int8, std::numeric_limits<int8_t>::min());
    add_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::max(), Int8, static_cast<int8_t>(0), Int8, std::numeric_limits<int8_t>::max());
    add_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, std::numeric_limits<int8_t>::max(), Int8, std::numeric_limits<int8_t>::max());
    add_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::min(), Int8, static_cast<int8_t>(-1), Int8, std::numeric_limits<int8_t>::max());
    add_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::max(), Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::min());
}
TEST(omrgenExtension, AddInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    AddFunc<FuncProto, int16_t, int16_t, int16_t> add_int16s(LOC, "add_int16s", c, false);
    add_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    add_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(3));
    add_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(3));
    add_int16s.test(LOC, Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(0));
    add_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(0));
    add_int16s.test(LOC, Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(-6));
    add_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::min(), Int16, static_cast<int16_t>(0), Int16, std::numeric_limits<int16_t>::min());
    add_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, std::numeric_limits<int16_t>::min(), Int16, std::numeric_limits<int16_t>::min());
    add_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::max(), Int16, static_cast<int16_t>(0), Int16, std::numeric_limits<int16_t>::max());
    add_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, std::numeric_limits<int16_t>::max(), Int16, std::numeric_limits<int16_t>::max());
    add_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::min(), Int16, static_cast<int16_t>(-1), Int16, std::numeric_limits<int16_t>::max());
    add_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::max(), Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::min());
}
TEST(omrgenExtension, AddInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    AddFunc<FuncProto, int32_t, int32_t, int32_t> add_int32s(LOC, "add_int32s", c, false);
    add_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    add_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(3));
    add_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(3));
    add_int32s.test(LOC, Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(0));
    add_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(0));
    add_int32s.test(LOC, Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(-6));
    add_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::min(), Int32, static_cast<int32_t>(0), Int32, std::numeric_limits<int32_t>::min());
    add_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, std::numeric_limits<int32_t>::min(), Int32, std::numeric_limits<int32_t>::min());
    add_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Int32, static_cast<int32_t>(0), Int32, std::numeric_limits<int32_t>::max());
    add_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, std::numeric_limits<int32_t>::max(), Int32, std::numeric_limits<int32_t>::max());
    add_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::min(), Int32, static_cast<int32_t>(-1), Int32, std::numeric_limits<int32_t>::max());
    add_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::min());
}
TEST(omrgenExtension, AddInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    AddFunc<FuncProto, int64_t, int64_t, int64_t> add_int64s(LOC, "add_int64s", c, false);
    add_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    add_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(3));
    add_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(3));
    add_int64s.test(LOC, Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(0));
    add_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(0));
    add_int64s.test(LOC, Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(-6));
    add_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::min(), Int64, static_cast<int64_t>(0), Int64, std::numeric_limits<int64_t>::min());
    add_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, std::numeric_limits<int64_t>::min(), Int64, std::numeric_limits<int64_t>::min());
    add_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::max(), Int64, static_cast<int64_t>(0), Int64, std::numeric_limits<int64_t>::max());
    add_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, std::numeric_limits<int64_t>::max(), Int64, std::numeric_limits<int64_t>::max());
    add_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::min(), Int64, static_cast<int64_t>(-1), Int64, std::numeric_limits<int64_t>::max());
    add_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::max(), Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::min());
}
TEST(omrgenExtension, AddFloat32s) {
    typedef float (FuncProto)(float, float);
    AddFunc<FuncProto, float, float, float> add_floats(LOC, "add_float", c, false);
    add_floats.test(LOC, Float32, static_cast<float>(0), Float32, static_cast<float>(0), Float32, static_cast<float>(0));
    add_floats.test(LOC, Float32, static_cast<float>(0), Float32, static_cast<float>(3), Float32, static_cast<float>(3));
    add_floats.test(LOC, Float32, static_cast<float>(3), Float32, static_cast<float>(0), Float32, static_cast<float>(3));
    add_floats.test(LOC, Float32, static_cast<float>(-3), Float32, static_cast<float>(3), Float32, static_cast<float>(0));
    add_floats.test(LOC, Float32, static_cast<float>(3), Float32, static_cast<float>(-3), Float32, static_cast<float>(0));
    add_floats.test(LOC, Float32, static_cast<float>(-3), Float32, static_cast<float>(-3), Float32, static_cast<float>(-6));
    add_floats.test(LOC, Float32, std::numeric_limits<float>::min(), Float32, static_cast<float>(0), Float32, std::numeric_limits<float>::min());
    add_floats.test(LOC, Float32, static_cast<float>(0), Float32, std::numeric_limits<float>::min(), Float32, std::numeric_limits<float>::min());
    add_floats.test(LOC, Float32, std::numeric_limits<float>::max(), Float32, static_cast<float>(0), Float32, std::numeric_limits<float>::max());
    add_floats.test(LOC, Float32, static_cast<float>(0), Float32, std::numeric_limits<float>::max(), Float32, std::numeric_limits<float>::max());
}
TEST(omrgenExtension, AddFloat64s) {
    typedef double (FuncProto)(double, double);
    AddFunc<FuncProto, double, double, double> add_doubles(LOC, "add_double", c, false);
    add_doubles.test(LOC, Float64, static_cast<double>(0), Float64, static_cast<double>(0), Float64, static_cast<double>(0));
    add_doubles.test(LOC, Float64, static_cast<double>(0), Float64, static_cast<double>(3), Float64, static_cast<double>(3));
    add_doubles.test(LOC, Float64, static_cast<double>(3), Float64, static_cast<double>(0), Float64, static_cast<double>(3));
    add_doubles.test(LOC, Float64, static_cast<double>(-3), Float64, static_cast<double>(3), Float64, static_cast<double>(0));
    add_doubles.test(LOC, Float64, static_cast<double>(3), Float64, static_cast<double>(-3), Float64, static_cast<double>(0));
    add_doubles.test(LOC, Float64, static_cast<double>(-3), Float64, static_cast<double>(-3), Float64, static_cast<double>(-6));
    add_doubles.test(LOC, Float64, std::numeric_limits<double>::min(), Float64, static_cast<double>(0), Float64, std::numeric_limits<double>::min());
    add_doubles.test(LOC, Float64, static_cast<double>(0), Float64, std::numeric_limits<double>::min(), Float64, std::numeric_limits<double>::min());
    add_doubles.test(LOC, Float64, std::numeric_limits<double>::max(), Float64, static_cast<double>(0), Float64, std::numeric_limits<double>::max());
    add_doubles.test(LOC, Float64, static_cast<double>(0), Float64, std::numeric_limits<double>::max(), Float64, std::numeric_limits<double>::max());
}
TEST(omrgenExtension, AddAddressAndInt) {
    if (c->platformWordSize() == 32) {
        typedef intptr_t (FuncProto)(intptr_t, int32_t);
        AddFunc<FuncProto, intptr_t, int32_t, intptr_t> add_addressint32s(LOC, "add_addressint32s", c, false);
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(NULL), Int32, static_cast<int32_t>(0), Address, static_cast<intptr_t>(NULL));
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(NULL), Int32, static_cast<int32_t>(4), Address, static_cast<intptr_t>(4));
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(0xdeadbeef), Int32, static_cast<int32_t>(0), Address, static_cast<intptr_t>(0xdeadbeef));
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(0xdeadbeef), Int32, static_cast<int32_t>(16), Address, static_cast<intptr_t>(0xdeadbeef+16));
        add_addressint32s.test(LOC, Address, std::numeric_limits<intptr_t>::max(), Int32, static_cast<int32_t>(0), Address, std::numeric_limits<intptr_t>::max());
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(0), Int32, std::numeric_limits<int32_t>::max(), Address, static_cast<intptr_t>(std::numeric_limits<int32_t>::max()));
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(0), Int32, std::numeric_limits<int32_t>::min(), Address, static_cast<intptr_t>(std::numeric_limits<int32_t>::min()));
    } else if (c->platformWordSize() == 64) {
        typedef intptr_t (FuncProto)(intptr_t, int64_t);
        AddFunc<FuncProto, intptr_t, int64_t, intptr_t> add_addressint64s(LOC, "add_addressint64s", c, false);
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(NULL), Int64, static_cast<int64_t>(0), Address, static_cast<intptr_t>(NULL));
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(NULL), Int64, static_cast<int64_t>(4), Address, static_cast<intptr_t>(4));
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(0xdeadbeef), Int64, static_cast<int64_t>(0), Address, static_cast<intptr_t>(0xdeadbeef));
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(0xdeadbeef), Int64, static_cast<int64_t>(16), Address, static_cast<intptr_t>(0xdeadbeef+16));
        add_addressint64s.test(LOC, Address, std::numeric_limits<intptr_t>::max(), Int64, static_cast<int64_t>(0), Address, std::numeric_limits<intptr_t>::max());
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(0), Int64, std::numeric_limits<int64_t>::max(), Address, static_cast<intptr_t>(std::numeric_limits<int64_t>::max()));
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(0), Int64, std::numeric_limits<int64_t>::min(), Address, static_cast<intptr_t>(std::numeric_limits<int64_t>::min()));

    }
}
TEST(omrgenExtension, AddIntAndAddress) {
    if (c->platformWordSize() == 32) {
        typedef intptr_t (FuncProto)(int32_t, intptr_t);
        AddFunc<FuncProto, int32_t, intptr_t, intptr_t> add_int32addresses(LOC, "add_int32addresses", c, false);
        add_int32addresses.test(LOC, Int32, static_cast<int32_t>(0), Address, static_cast<intptr_t>(NULL), Address, static_cast<intptr_t>(NULL));
        add_int32addresses.test(LOC, Int32, static_cast<int32_t>(4), Address, static_cast<intptr_t>(NULL), Address, static_cast<intptr_t>(4));
        add_int32addresses.test(LOC, Int32, static_cast<int32_t>(0), Address, static_cast<intptr_t>(0xdeadbeef), Address, static_cast<intptr_t>(0xdeadbeef));
        add_int32addresses.test(LOC, Int32, static_cast<int32_t>(16), Address, static_cast<intptr_t>(0xdeadbeef), Address, static_cast<intptr_t>(0xdeadbeef+16));
        add_int32addresses.test(LOC, Int32, static_cast<int32_t>(0), Address, std::numeric_limits<intptr_t>::max(), Address, std::numeric_limits<intptr_t>::max());
        add_int32addresses.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Address, static_cast<intptr_t>(0), Address, static_cast<intptr_t>(std::numeric_limits<int32_t>::max()));
        add_int32addresses.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Address, static_cast<intptr_t>(0), Address, static_cast<intptr_t>(std::numeric_limits<int32_t>::min()));
    } else if (c->platformWordSize() == 64) {
        typedef intptr_t (FuncRevProto)(intptr_t, int64_t);
        AddFunc<FuncRevProto, intptr_t, int64_t, intptr_t> add_int64addresses(LOC, "add_int64addresses", c, false);
        add_int64addresses.test(LOC, Int64, static_cast<int64_t>(0), Address, static_cast<intptr_t>(NULL), Address, static_cast<intptr_t>(NULL));
        add_int64addresses.test(LOC, Int64, static_cast<int64_t>(4), Address, static_cast<intptr_t>(NULL), Address, static_cast<intptr_t>(4));
        add_int64addresses.test(LOC, Int64, static_cast<int64_t>(0), Address, static_cast<intptr_t>(0xdeadbeef), Address, static_cast<intptr_t>(0xdeadbeef));
        add_int64addresses.test(LOC, Int64, static_cast<int64_t>(16), Address, static_cast<intptr_t>(0xdeadbeef), Address, static_cast<intptr_t>(0xdeadbeef+16));
        add_int64addresses.test(LOC, Int64, static_cast<int64_t>(0), Address, std::numeric_limits<intptr_t>::max(), Address, std::numeric_limits<intptr_t>::max());
        add_int64addresses.test(LOC, Int64, std::numeric_limits<int64_t>::max(), Address, static_cast<intptr_t>(0), Address, static_cast<intptr_t>(std::numeric_limits<int64_t>::max()));
        add_int64addresses.test(LOC, Int64, std::numeric_limits<int64_t>::min(), Address, static_cast<intptr_t>(0), Address, static_cast<intptr_t>(std::numeric_limits<int64_t>::min()));
    }
}

// Test anding two numbers together of the given types
template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class AndFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    AndFunc(LOCATION, String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->And(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, AndInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    AndFunc<FuncProto, int8_t, int8_t, int8_t> and_int8s(LOC, "and_int8s", c, false);
    and_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    and_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(0));
    and_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    and_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(3));
    and_int8s.test(LOC, Int8, static_cast<int8_t>(12), Int8, static_cast<int8_t>(4), Int8, static_cast<int8_t>(4));
    and_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::min(), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    and_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, std::numeric_limits<int8_t>::min(), Int8, static_cast<int8_t>(0));
    and_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::min(), Int8, std::numeric_limits<int8_t>::min(), Int8, std::numeric_limits<int8_t>::min());
    and_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::max(), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    and_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, std::numeric_limits<int8_t>::max(), Int8, static_cast<int8_t>(0));
    and_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::max(), Int8, std::numeric_limits<int8_t>::max(), Int8, std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, AndInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    AndFunc<FuncProto, int16_t, int16_t, int16_t> and_int16s(LOC, "and_int16s", c, false);
    and_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    and_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(0));
    and_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    and_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(3));
    and_int16s.test(LOC, Int16, static_cast<int16_t>(12), Int16, static_cast<int16_t>(4), Int16, static_cast<int16_t>(4));
    and_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::min(), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    and_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, std::numeric_limits<int16_t>::min(), Int16, static_cast<int16_t>(0));
    and_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::min(), Int16, std::numeric_limits<int16_t>::min(), Int16, std::numeric_limits<int16_t>::min());
    and_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::max(), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    and_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, std::numeric_limits<int16_t>::max(), Int16, static_cast<int16_t>(0));
    and_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::max(), Int16, std::numeric_limits<int16_t>::max(), Int16, std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, AndInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    AndFunc<FuncProto, int32_t, int32_t, int32_t> and_int32s(LOC, "and_int32s", c, false);
    and_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    and_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(0));
    and_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    and_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(3));
    and_int32s.test(LOC, Int32, static_cast<int32_t>(12), Int32, static_cast<int32_t>(4), Int32, static_cast<int32_t>(4));
    and_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::min(), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    and_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, std::numeric_limits<int32_t>::min(), Int32, static_cast<int32_t>(0));
    and_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::min(), Int32, std::numeric_limits<int32_t>::min(), Int32, std::numeric_limits<int32_t>::min());
    and_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    and_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, std::numeric_limits<int32_t>::max(), Int32, static_cast<int32_t>(0));
    and_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Int32, std::numeric_limits<int32_t>::max(), Int32, std::numeric_limits<int32_t>::max());
}
TEST(omrgenExtension, AndInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    AndFunc<FuncProto, int64_t, int64_t, int64_t> and_int64s(LOC, "and_int64s", c, false);
    and_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    and_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(0));
    and_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    and_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(3));
    and_int64s.test(LOC, Int64, static_cast<int64_t>(12), Int64, static_cast<int64_t>(4), Int64, static_cast<int64_t>(4));
    and_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::min(), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    and_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, std::numeric_limits<int64_t>::min(), Int64, static_cast<int64_t>(0));
    and_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::min(), Int64, std::numeric_limits<int64_t>::min(), Int64, std::numeric_limits<int64_t>::min());
    and_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::max(), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    and_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, std::numeric_limits<int64_t>::max(), Int64, static_cast<int64_t>(0));
    and_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::max(), Int64, std::numeric_limits<int64_t>::max(), Int64, std::numeric_limits<int64_t>::max());
}


// Test subtracting two numbers of the given types
template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class SubFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    SubFunc(LOCATION, String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->Sub(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, SubInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    SubFunc<FuncProto, int8_t, int8_t, int8_t> sub_int8s(LOC, "sub_int8s", c, false);
    sub_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    sub_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(-3));
    sub_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(3));
    sub_int8s.test(LOC, Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(-6));
    sub_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(6));
    sub_int8s.test(LOC, Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(0));
    sub_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::min(), Int8, static_cast<int8_t>(0), Int8, std::numeric_limits<int8_t>::min());
    sub_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::max(), Int8, static_cast<int8_t>(0), Int8, std::numeric_limits<int8_t>::max());
    sub_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::min(), Int8, static_cast<int8_t>(-1), Int8, std::numeric_limits<int8_t>::min()+1);
    sub_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::max(), Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::max()-1);
}
TEST(omrgenExtension, SubInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    SubFunc<FuncProto, int16_t, int16_t, int16_t> sub_int16s(LOC, "sub_int16s", c, false);
    sub_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    sub_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(-3));
    sub_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(3));
    sub_int16s.test(LOC, Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(-6));
    sub_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(6));
    sub_int16s.test(LOC, Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(0));
    sub_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::min(), Int16, static_cast<int16_t>(0), Int16, std::numeric_limits<int16_t>::min());
    sub_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::max(), Int16, static_cast<int16_t>(0), Int16, std::numeric_limits<int16_t>::max());
    sub_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::min(), Int16, static_cast<int16_t>(-1), Int16, std::numeric_limits<int16_t>::min()+1);
    sub_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::max(), Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::max()-1);
}
TEST(omrgenExtension, SubInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    SubFunc<FuncProto, int32_t, int32_t, int32_t> sub_int32s(LOC, "sub_int32s", c, false);
    sub_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    sub_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(-3));
    sub_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(3));
    sub_int32s.test(LOC, Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(-6));
    sub_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(6));
    sub_int32s.test(LOC, Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(0));
    sub_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::min(), Int32, static_cast<int32_t>(0), Int32, std::numeric_limits<int32_t>::min());
    sub_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Int32, static_cast<int32_t>(0), Int32, std::numeric_limits<int32_t>::max());
    sub_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::min(), Int32, static_cast<int32_t>(-1), Int32, std::numeric_limits<int32_t>::min()+1);
    sub_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::max()-1);
}
TEST(omrgenExtension, SubInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    SubFunc<FuncProto, int64_t, int64_t, int64_t> sub_int64s(LOC, "sub_int64s", c, false);
    sub_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    sub_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(-3));
    sub_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(3));
    sub_int64s.test(LOC, Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(-6));
    sub_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(6));
    sub_int64s.test(LOC, Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(0));
    sub_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::min(), Int64, static_cast<int64_t>(0), Int64, std::numeric_limits<int64_t>::min());
    sub_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::max(), Int64, static_cast<int64_t>(0), Int64, std::numeric_limits<int64_t>::max());
    sub_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::min(), Int64, static_cast<int64_t>(-1), Int64, std::numeric_limits<int64_t>::min()+1);
    sub_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::max(), Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::max()-1);
}
TEST(omrgenExtension, SubFloat32s) {
    typedef float (FuncProto)(float, float);
    SubFunc<FuncProto, float, float, float> sub_float(LOC, "sub_float", c, false);
    sub_float.test(LOC, Float32, static_cast<float>(0), Float32, static_cast<float>(0), Float32, static_cast<float>(0));
    sub_float.test(LOC, Float32, static_cast<float>(0), Float32, static_cast<float>(3), Float32, static_cast<float>(-3));
    sub_float.test(LOC, Float32, static_cast<float>(3), Float32, static_cast<float>(0), Float32, static_cast<float>(3));
    sub_float.test(LOC, Float32, static_cast<float>(-3), Float32, static_cast<float>(3), Float32, static_cast<float>(-6));
    sub_float.test(LOC, Float32, static_cast<float>(3), Float32, static_cast<float>(-3), Float32, static_cast<float>(6));
    sub_float.test(LOC, Float32, static_cast<float>(-3), Float32, static_cast<float>(-3), Float32, static_cast<float>(0));
    sub_float.test(LOC, Float32, std::numeric_limits<float>::min(), Float32, static_cast<float>(0), Float32, std::numeric_limits<float>::min());
    sub_float.test(LOC, Float32, std::numeric_limits<float>::max(), Float32, static_cast<float>(0), Float32, std::numeric_limits<float>::max());
    sub_float.test(LOC, Float32, std::numeric_limits<float>::min(), Float32, static_cast<float>(-1), Float32, std::numeric_limits<float>::min()+1);
    sub_float.test(LOC, Float32, std::numeric_limits<float>::max(), Float32, static_cast<float>(1), Float32, std::numeric_limits<float>::max()-1);
}
TEST(omrgenExtension, SubFloat64s) {
    typedef double (FuncProto)(double, double);
    SubFunc<FuncProto, double, double, double> sub_double(LOC, "sub_double", c, false);
    sub_double.test(LOC, Float64, static_cast<double>(0), Float64, static_cast<double>(0), Float64, static_cast<double>(0));
    sub_double.test(LOC, Float64, static_cast<double>(0), Float64, static_cast<double>(3), Float64, static_cast<double>(-3));
    sub_double.test(LOC, Float64, static_cast<double>(3), Float64, static_cast<double>(0), Float64, static_cast<double>(3));
    sub_double.test(LOC, Float64, static_cast<double>(-3), Float64, static_cast<double>(3), Float64, static_cast<double>(-6));
    sub_double.test(LOC, Float64, static_cast<double>(3), Float64, static_cast<double>(-3), Float64, static_cast<double>(6));
    sub_double.test(LOC, Float64, static_cast<double>(-3), Float64, static_cast<double>(-3), Float64, static_cast<double>(0));
    sub_double.test(LOC, Float64, std::numeric_limits<double>::min(), Float64, static_cast<double>(0), Float64, std::numeric_limits<double>::min());
    sub_double.test(LOC, Float64, std::numeric_limits<double>::max(), Float64, static_cast<double>(0), Float64, std::numeric_limits<double>::max());
    sub_double.test(LOC, Float64, std::numeric_limits<double>::min(), Float64, static_cast<double>(-1), Float64, std::numeric_limits<double>::min()+1);
    sub_double.test(LOC, Float64, std::numeric_limits<double>::max(), Float64, static_cast<double>(1), Float64, std::numeric_limits<double>::max()-1);
}
TEST(omrgenExtension, SubAddressAndInt) {
    if (c->platformWordSize() == 32) {
        typedef intptr_t (FuncProto)(intptr_t, int32_t);
        SubFunc<FuncProto, intptr_t, int32_t, intptr_t> add_addressint32s(LOC, "add_addressint32s", c, false);
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(NULL), Int32, static_cast<int32_t>(0), Address, static_cast<intptr_t>(NULL));
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(NULL), Int32, static_cast<int32_t>(4), Address, static_cast<intptr_t>(-4));
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(0xdeadbeef), Int32, static_cast<int32_t>(0), Address, static_cast<intptr_t>(0xdeadbeef));
        add_addressint32s.test(LOC, Address, static_cast<intptr_t>(0xdeadbeef), Int32, static_cast<int32_t>(16), Address, static_cast<intptr_t>(0xdeadbeef-16));
        add_addressint32s.test(LOC, Address, std::numeric_limits<intptr_t>::max(), Int32, static_cast<int32_t>(0), Address, std::numeric_limits<intptr_t>::max());
        add_addressint32s.test(LOC, Address, std::numeric_limits<intptr_t>::min(), Int32, static_cast<int32_t>(0), Address, std::numeric_limits<intptr_t>::min());
    } else if (c->platformWordSize() == 64) {
        typedef intptr_t (FuncProto)(intptr_t, int64_t);
        SubFunc<FuncProto, intptr_t, int64_t, intptr_t> add_addressint64s(LOC, "add_addressint64s", c, false);
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(NULL), Int64, static_cast<int64_t>(0), Address, static_cast<intptr_t>(NULL));
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(NULL), Int64, static_cast<int64_t>(4), Address, static_cast<intptr_t>(-4));
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(0xdeadbeef), Int64, static_cast<int64_t>(0), Address, static_cast<intptr_t>(0xdeadbeef));
        add_addressint64s.test(LOC, Address, static_cast<intptr_t>(0xdeadbeef), Int64, static_cast<int64_t>(16), Address, static_cast<intptr_t>(0xdeadbeef-16));
        add_addressint64s.test(LOC, Address, std::numeric_limits<intptr_t>::max(), Int64, static_cast<int64_t>(0), Address, std::numeric_limits<intptr_t>::max());
        add_addressint64s.test(LOC, Address, std::numeric_limits<intptr_t>::min(), Int64, static_cast<int64_t>(0), Address, std::numeric_limits<intptr_t>::min());

    }
}


template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class DivFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    DivFunc(LOCATION, String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->Div(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, DivInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    DivFunc<FuncProto, int8_t, int8_t, int8_t> div_int8s(LOC, "div_int8s", c, false);
    //div_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    div_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(0));
    //div_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    div_int8s.test(LOC, Int8, static_cast<int8_t>(1), Int8, static_cast<int8_t>(1), Int8, static_cast<int8_t>(1));
    div_int8s.test(LOC, Int8, static_cast<int8_t>(-1), Int8, static_cast<int8_t>(1), Int8, static_cast<int8_t>(-1));
    div_int8s.test(LOC, Int8, static_cast<int8_t>(1), Int8, static_cast<int8_t>(-1), Int8, static_cast<int8_t>(-1));
    div_int8s.test(LOC, Int8, static_cast<int8_t>(-1), Int8, static_cast<int8_t>(-1), Int8, static_cast<int8_t>(1));
    div_int8s.test(LOC, Int8, static_cast<int8_t>(9), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(3));
    div_int8s.test(LOC, Int8, static_cast<int8_t>(-9), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(-3));
    div_int8s.test(LOC, Int8, static_cast<int8_t>(9), Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(-3));
    div_int8s.test(LOC, Int8, static_cast<int8_t>(-9), Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(3));
    div_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::min(), Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::min());
    div_int8s.test(LOC, Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::min(), Int8, static_cast<int8_t>(0));
    div_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::max(), Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::max());
    div_int8s.test(LOC, Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::max(), Int8, static_cast<int8_t>(0));
}
TEST(omrgenExtension, DivInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    DivFunc<FuncProto, int16_t, int16_t, int16_t> div_int16s(LOC, "div_int16s", c, false);
    //div_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    div_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(0));
    //div_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    div_int16s.test(LOC, Int16, static_cast<int16_t>(1), Int16, static_cast<int16_t>(1), Int16, static_cast<int16_t>(1));
    div_int16s.test(LOC, Int16, static_cast<int16_t>(-1), Int16, static_cast<int16_t>(1), Int16, static_cast<int16_t>(-1));
    div_int16s.test(LOC, Int16, static_cast<int16_t>(1), Int16, static_cast<int16_t>(-1), Int16, static_cast<int16_t>(-1));
    div_int16s.test(LOC, Int16, static_cast<int16_t>(-1), Int16, static_cast<int16_t>(-1), Int16, static_cast<int16_t>(1));
    div_int16s.test(LOC, Int16, static_cast<int16_t>(9), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(3));
    div_int16s.test(LOC, Int16, static_cast<int16_t>(-9), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(-3));
    div_int16s.test(LOC, Int16, static_cast<int16_t>(9), Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(-3));
    div_int16s.test(LOC, Int16, static_cast<int16_t>(-9), Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(3));
    div_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::min(), Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::min());
    div_int16s.test(LOC, Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::min(), Int16, static_cast<int16_t>(0));
    div_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::max(), Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::max());
    div_int16s.test(LOC, Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::max(), Int16, static_cast<int16_t>(0));
}
TEST(omrgenExtension, DivInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    DivFunc<FuncProto, int32_t, int32_t, int32_t> div_int32s(LOC, "div_int32s", c, false);
    //div_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    div_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(0));
    //div_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    div_int32s.test(LOC, Int32, static_cast<int32_t>(1), Int32, static_cast<int32_t>(1), Int32, static_cast<int32_t>(1));
    div_int32s.test(LOC, Int32, static_cast<int32_t>(-1), Int32, static_cast<int32_t>(1), Int32, static_cast<int32_t>(-1));
    div_int32s.test(LOC, Int32, static_cast<int32_t>(1), Int32, static_cast<int32_t>(-1), Int32, static_cast<int32_t>(-1));
    div_int32s.test(LOC, Int32, static_cast<int32_t>(-1), Int32, static_cast<int32_t>(-1), Int32, static_cast<int32_t>(1));
    div_int32s.test(LOC, Int32, static_cast<int32_t>(9), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(3));
    div_int32s.test(LOC, Int32, static_cast<int32_t>(-9), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(-3));
    div_int32s.test(LOC, Int32, static_cast<int32_t>(9), Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(-3));
    div_int32s.test(LOC, Int32, static_cast<int32_t>(-9), Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(3));
    div_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::min(), Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::min());
    div_int32s.test(LOC, Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::min(), Int32, static_cast<int32_t>(0));
    div_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::max());
    div_int32s.test(LOC, Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::max(), Int32, static_cast<int32_t>(0));
}
TEST(omrgenExtension, DivInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    DivFunc<FuncProto, int64_t, int64_t, int64_t> div_int64s(LOC, "div_int64s", c, false);
    //div_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    div_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(0));
    //div_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    div_int64s.test(LOC, Int64, static_cast<int64_t>(1), Int64, static_cast<int64_t>(1), Int64, static_cast<int64_t>(1));
    div_int64s.test(LOC, Int64, static_cast<int64_t>(-1), Int64, static_cast<int64_t>(1), Int64, static_cast<int64_t>(-1));
    div_int64s.test(LOC, Int64, static_cast<int64_t>(1), Int64, static_cast<int64_t>(-1), Int64, static_cast<int64_t>(-1));
    div_int64s.test(LOC, Int64, static_cast<int64_t>(-1), Int64, static_cast<int64_t>(-1), Int64, static_cast<int64_t>(1));
    div_int64s.test(LOC, Int64, static_cast<int64_t>(9), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(3));
    div_int64s.test(LOC, Int64, static_cast<int64_t>(-9), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(-3));
    div_int64s.test(LOC, Int64, static_cast<int64_t>(9), Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(-3));
    div_int64s.test(LOC, Int64, static_cast<int64_t>(-9), Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(3));
    div_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::min(), Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::min());
    div_int64s.test(LOC, Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::min(), Int64, static_cast<int64_t>(0));
    div_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::max(), Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::max());
    div_int64s.test(LOC, Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::max(), Int64, static_cast<int64_t>(0));
}
TEST(omrgenExtension, DivFloat32s) {
    typedef float (FuncProto)(float, float);
    DivFunc<FuncProto, float, float, float> div_floats(LOC, "div_float", c, false);
    //div_floats.test(LOC, Float32, static_cast<float>(0), Float32, static_cast<float>(0), Float32, static_cast<float>(0));
    div_floats.test(LOC, Float32, static_cast<float>(0), Float32, static_cast<float>(3), Float32, static_cast<float>(0));
    //div_floats.test(LOC, Float32, static_cast<float>(3), Float32, static_cast<float>(0), Float32, static_cast<float>(0));
    div_floats.test(LOC, Float32, static_cast<float>(1), Float32, static_cast<float>(1), Float32, static_cast<float>(1));
    div_floats.test(LOC, Float32, static_cast<float>(-1), Float32, static_cast<float>(1), Float32, static_cast<float>(-1));
    div_floats.test(LOC, Float32, static_cast<float>(1), Float32, static_cast<float>(-1), Float32, static_cast<float>(-1));
    div_floats.test(LOC, Float32, static_cast<float>(-1), Float32, static_cast<float>(-1), Float32, static_cast<float>(1));
    div_floats.test(LOC, Float32, static_cast<float>(9), Float32, static_cast<float>(3), Float32, static_cast<float>(3));
    div_floats.test(LOC, Float32, static_cast<float>(-9), Float32, static_cast<float>(3), Float32, static_cast<float>(-3));
    div_floats.test(LOC, Float32, static_cast<float>(9), Float32, static_cast<float>(-3), Float32, static_cast<float>(-3));
    div_floats.test(LOC, Float32, static_cast<float>(-9), Float32, static_cast<float>(-3), Float32, static_cast<float>(3));
    div_floats.test(LOC, Float32, std::numeric_limits<float>::min(), Float32, static_cast<float>(1), Float32, std::numeric_limits<float>::min());
    div_floats.test(LOC, Float32, static_cast<float>(1), Float32, std::numeric_limits<float>::min(), Float32, static_cast<float>(1/std::numeric_limits<float>::min()));
    div_floats.test(LOC, Float32, std::numeric_limits<float>::max(), Float32, static_cast<float>(1), Float32, std::numeric_limits<float>::max());
    div_floats.test(LOC, Float32, static_cast<float>(1), Float32, std::numeric_limits<float>::max(), Float32, static_cast<float>(1/std::numeric_limits<float>::max()));
}
TEST(omrgenExtension, DivFloat64s) {
    typedef double (FuncProto)(double, double);
    DivFunc<FuncProto, double, double, double> div_doubles(LOC, "div_double", c, false);
    //div_doubles.test(LOC, Float64, static_cast<double>(0), Float64, static_cast<double>(0), Float64, static_cast<double>(0));
    div_doubles.test(LOC, Float64, static_cast<double>(0), Float64, static_cast<double>(3), Float64, static_cast<double>(0));
    //div_doubles.test(LOC, Float64, static_cast<double>(3), Float64, static_cast<double>(0), Float64, static_cast<double>(0));
    div_doubles.test(LOC, Float64, static_cast<double>(1), Float64, static_cast<double>(1), Float64, static_cast<double>(1));
    div_doubles.test(LOC, Float64, static_cast<double>(-1), Float64, static_cast<double>(1), Float64, static_cast<double>(-1));
    div_doubles.test(LOC, Float64, static_cast<double>(1), Float64, static_cast<double>(-1), Float64, static_cast<double>(-1));
    div_doubles.test(LOC, Float64, static_cast<double>(-1), Float64, static_cast<double>(-1), Float64, static_cast<double>(1));
    div_doubles.test(LOC, Float64, static_cast<double>(9), Float64, static_cast<double>(3), Float64, static_cast<double>(3));
    div_doubles.test(LOC, Float64, static_cast<double>(-9), Float64, static_cast<double>(3), Float64, static_cast<double>(-3));
    div_doubles.test(LOC, Float64, static_cast<double>(9), Float64, static_cast<double>(-3), Float64, static_cast<double>(-3));
    div_doubles.test(LOC, Float64, static_cast<double>(-9), Float64, static_cast<double>(-3), Float64, static_cast<double>(3));
    div_doubles.test(LOC, Float64, std::numeric_limits<double>::min(), Float64, static_cast<double>(1), Float64, std::numeric_limits<double>::min());
    div_doubles.test(LOC, Float64, static_cast<double>(1), Float64, std::numeric_limits<double>::min(), Float64, static_cast<double>(1/std::numeric_limits<double>::min()));
    div_doubles.test(LOC, Float64, std::numeric_limits<double>::max(), Float64, static_cast<double>(1), Float64, std::numeric_limits<double>::max());
    div_doubles.test(LOC, Float64, static_cast<double>(1), Float64, std::numeric_limits<double>::max(), Float64, static_cast<double>(1/std::numeric_limits<double>::max()));
}


template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class MulFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    MulFunc(LOCATION, String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->Mul(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, MulInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    MulFunc<FuncProto, int8_t, int8_t, int8_t> mul_int8s(LOC, "mul_int8s", c, false);
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(0));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(0), Int8, static_cast<int8_t>(0));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(1), Int8, static_cast<int8_t>(1), Int8, static_cast<int8_t>(1));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(-1), Int8, static_cast<int8_t>(1), Int8, static_cast<int8_t>(-1));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(1), Int8, static_cast<int8_t>(-1), Int8, static_cast<int8_t>(-1));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(-1), Int8, static_cast<int8_t>(-1), Int8, static_cast<int8_t>(1));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(9));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(-9));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(3), Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(-9));
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(-3), Int8, static_cast<int8_t>(9));
    mul_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::min(), Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::min());
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::min(), Int8, std::numeric_limits<int8_t>::min());
    mul_int8s.test(LOC, Int8, std::numeric_limits<int8_t>::max(), Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::max());
    mul_int8s.test(LOC, Int8, static_cast<int8_t>(1), Int8, std::numeric_limits<int8_t>::max(), Int8, std::numeric_limits<int8_t>::max());
}
TEST(omrgenExtension, MulInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    MulFunc<FuncProto, int16_t, int16_t, int16_t> mul_int16s(LOC, "mul_int16s", c, false);
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(0));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(0), Int16, static_cast<int16_t>(0));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(1), Int16, static_cast<int16_t>(1), Int16, static_cast<int16_t>(1));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(-1), Int16, static_cast<int16_t>(1), Int16, static_cast<int16_t>(-1));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(1), Int16, static_cast<int16_t>(-1), Int16, static_cast<int16_t>(-1));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(-1), Int16, static_cast<int16_t>(-1), Int16, static_cast<int16_t>(1));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(9));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(-9));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(3), Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(-9));
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(-3), Int16, static_cast<int16_t>(9));
    mul_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::min(), Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::min());
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::min(), Int16, std::numeric_limits<int16_t>::min());
    mul_int16s.test(LOC, Int16, std::numeric_limits<int16_t>::max(), Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::max());
    mul_int16s.test(LOC, Int16, static_cast<int16_t>(1), Int16, std::numeric_limits<int16_t>::max(), Int16, std::numeric_limits<int16_t>::max());
}
TEST(omrgenExtension, MulInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    MulFunc<FuncProto, int32_t, int32_t, int32_t> mul_int32s(LOC, "mul_int32s", c, false);
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(0));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(0), Int32, static_cast<int32_t>(0));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(1), Int32, static_cast<int32_t>(1), Int32, static_cast<int32_t>(1));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(-1), Int32, static_cast<int32_t>(1), Int32, static_cast<int32_t>(-1));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(1), Int32, static_cast<int32_t>(-1), Int32, static_cast<int32_t>(-1));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(-1), Int32, static_cast<int32_t>(-1), Int32, static_cast<int32_t>(1));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(9));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(-9));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(3), Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(-9));
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(-3), Int32, static_cast<int32_t>(9));
    mul_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::min(), Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::min());
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::min(), Int32, std::numeric_limits<int32_t>::min());
    mul_int32s.test(LOC, Int32, std::numeric_limits<int32_t>::max(), Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::max());
    mul_int32s.test(LOC, Int32, static_cast<int32_t>(1), Int32, std::numeric_limits<int32_t>::max(), Int32, std::numeric_limits<int32_t>::max());
}
TEST(omrgenExtension, MulInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    MulFunc<FuncProto, int64_t, int64_t, int64_t> mul_int64s(LOC, "mul_int64s", c, false);
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(0));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(0), Int64, static_cast<int64_t>(0));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(1), Int64, static_cast<int64_t>(1), Int64, static_cast<int64_t>(1));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(-1), Int64, static_cast<int64_t>(1), Int64, static_cast<int64_t>(-1));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(1), Int64, static_cast<int64_t>(-1), Int64, static_cast<int64_t>(-1));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(-1), Int64, static_cast<int64_t>(-1), Int64, static_cast<int64_t>(1));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(9));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(-9));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(3), Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(-9));
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(-3), Int64, static_cast<int64_t>(9));
    mul_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::min(), Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::min());
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::min(), Int64, std::numeric_limits<int64_t>::min());
    mul_int64s.test(LOC, Int64, std::numeric_limits<int64_t>::max(), Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::max());
    mul_int64s.test(LOC, Int64, static_cast<int64_t>(1), Int64, std::numeric_limits<int64_t>::max(), Int64, std::numeric_limits<int64_t>::max());
}
TEST(omrgenExtension, MulFloat32s) {
    typedef float (FuncProto)(float, float);
    MulFunc<FuncProto, float, float, float> mul_floats(LOC, "mul_float", c, false);
    mul_floats.test(LOC, Float32, static_cast<float>(0), Float32, static_cast<float>(0), Float32, static_cast<float>(0));
    mul_floats.test(LOC, Float32, static_cast<float>(0), Float32, static_cast<float>(3), Float32, static_cast<float>(0));
    mul_floats.test(LOC, Float32, static_cast<float>(3), Float32, static_cast<float>(0), Float32, static_cast<float>(0));
    mul_floats.test(LOC, Float32, static_cast<float>(1), Float32, static_cast<float>(1), Float32, static_cast<float>(1));
    mul_floats.test(LOC, Float32, static_cast<float>(-1), Float32, static_cast<float>(1), Float32, static_cast<float>(-1));
    mul_floats.test(LOC, Float32, static_cast<float>(1), Float32, static_cast<float>(-1), Float32, static_cast<float>(-1));
    mul_floats.test(LOC, Float32, static_cast<float>(-1), Float32, static_cast<float>(-1), Float32, static_cast<float>(1));
    mul_floats.test(LOC, Float32, static_cast<float>(3), Float32, static_cast<float>(3), Float32, static_cast<float>(9));
    mul_floats.test(LOC, Float32, static_cast<float>(-3), Float32, static_cast<float>(3), Float32, static_cast<float>(-9));
    mul_floats.test(LOC, Float32, static_cast<float>(3), Float32, static_cast<float>(-3), Float32, static_cast<float>(-9));
    mul_floats.test(LOC, Float32, static_cast<float>(-3), Float32, static_cast<float>(-3), Float32, static_cast<float>(9));
    mul_floats.test(LOC, Float32, std::numeric_limits<float>::min(), Float32, static_cast<float>(1), Float32, std::numeric_limits<float>::min());
    mul_floats.test(LOC, Float32, static_cast<float>(1), Float32, std::numeric_limits<float>::min(), Float32, std::numeric_limits<float>::min());
    mul_floats.test(LOC, Float32, std::numeric_limits<float>::max(), Float32, static_cast<float>(1), Float32, std::numeric_limits<float>::max());
    mul_floats.test(LOC, Float32, static_cast<float>(1), Float32, std::numeric_limits<float>::max(), Float32, std::numeric_limits<float>::max());
}
TEST(omrgenExtension, MulFloat64s) {
    typedef double (FuncProto)(double, double);
    MulFunc<FuncProto, double, double, double> mul_doubles(LOC, "mul_double", c, false);
    mul_doubles.test(LOC, Float64, static_cast<double>(0), Float64, static_cast<double>(0), Float64, static_cast<double>(0));
    mul_doubles.test(LOC, Float64, static_cast<double>(0), Float64, static_cast<double>(3), Float64, static_cast<double>(0));
    mul_doubles.test(LOC, Float64, static_cast<double>(3), Float64, static_cast<double>(0), Float64, static_cast<double>(0));
    mul_doubles.test(LOC, Float64, static_cast<double>(1), Float64, static_cast<double>(1), Float64, static_cast<double>(1));
    mul_doubles.test(LOC, Float64, static_cast<double>(-1), Float64, static_cast<double>(1), Float64, static_cast<double>(-1));
    mul_doubles.test(LOC, Float64, static_cast<double>(1), Float64, static_cast<double>(-1), Float64, static_cast<double>(-1));
    mul_doubles.test(LOC, Float64, static_cast<double>(-1), Float64, static_cast<double>(-1), Float64, static_cast<double>(1));
    mul_doubles.test(LOC, Float64, static_cast<double>(3), Float64, static_cast<double>(3), Float64, static_cast<double>(9));
    mul_doubles.test(LOC, Float64, static_cast<double>(-3), Float64, static_cast<double>(3), Float64, static_cast<double>(-9));
    mul_doubles.test(LOC, Float64, static_cast<double>(3), Float64, static_cast<double>(-3), Float64, static_cast<double>(-9));
    mul_doubles.test(LOC, Float64, static_cast<double>(-3), Float64, static_cast<double>(-3), Float64, static_cast<double>(9));
    mul_doubles.test(LOC, Float64, std::numeric_limits<double>::min(), Float64, static_cast<double>(1), Float64, std::numeric_limits<double>::min());
    mul_doubles.test(LOC, Float64, static_cast<double>(1), Float64, std::numeric_limits<double>::min(), Float64, std::numeric_limits<double>::min());
    mul_doubles.test(LOC, Float64, std::numeric_limits<double>::max(), Float64, static_cast<double>(1), Float64, std::numeric_limits<double>::max());
    mul_doubles.test(LOC, Float64, static_cast<double>(1), Float64, std::numeric_limits<double>::max(), Float64, std::numeric_limits<double>::max());
}

class GotoOpFunc : public TestFunc {
public:
    GotoOpFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        typedef int8_t (FuncPrototype)(int8_t);
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_value), _resultValue) << "Compiled f(" << _value << ") returns " << _resultValue;
    }
    void test(LOCATION, int8_t value, int8_t resultValue) {
        _value = value;
        _resultValue = resultValue;
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineParameter("value", Int8);
        ctx->DefineReturnType(Int8);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto valueSym=ctx->LookupLocal("value"); 
        Value *value = fx()->Load(LOC, entry, valueSym);
        Builder *mergeBuilder = cx()->OrphanBuilder(LOC, entry);
        Builder *otherBuilder = cx()->OrphanBuilder(LOC, entry);
        bx()->IfCmpEqualZero(LOC, entry, otherBuilder, value);
        bx()->Goto(LOC, entry, mergeBuilder);
        bx()->Goto(LOC, otherBuilder, mergeBuilder);
        fx()->Return(LOC, mergeBuilder, bx()->ConstInt8(LOC, mergeBuilder, 3));
        return true;
    }

protected:
    int8_t _value;
    int8_t _resultValue;
};

TEST(omrgenExtension, Goto) {
    typedef int8_t (FuncProto)(int8_t);
    GotoOpFunc gotoFunc(LOC, "gotoFunc", c, false);
    gotoFunc.compile(LOC);
    gotoFunc.test(LOC, static_cast<int8_t>(0), static_cast<int8_t>(3));
    gotoFunc.test(LOC, static_cast<int8_t>(1), static_cast<int8_t>(3));
    gotoFunc.test(LOC, static_cast<int8_t>(-1), static_cast<int8_t>(3));
    gotoFunc.test(LOC, std::numeric_limits<int8_t>::min(), static_cast<int8_t>(3));
    gotoFunc.test(LOC, std::numeric_limits<int8_t>::max(), static_cast<int8_t>(3));
}

// Base test class for IfCmp<condition> opcodes to evalute whether fall-through or taken path occurs
// Compiled code returns int8_t: 0 if fall-through path, 1 if taken path
template<typename FuncPrototype, typename condition_cType>
class IfCmpToZeroBaseFunc : public TestFunc {
public:
    IfCmpToZeroBaseFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _conditionType(NULL)
        , _conditionValue(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_conditionValue), _resultValue) << "Compiled f(" << _conditionValue << ") returns " << _resultValue;
    }
    void compile(LOCATION, const Type *conditionType) {
        _conditionType = conditionType;
        this->TestFunc::compile(PASSLOC);
    }

    void test(LOCATION, const Type *conditionType, condition_cType conditionValue, int8_t resultValue) {
        _conditionValue = conditionValue;
        _resultValue = resultValue;
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineParameter("condition", _conditionType);
        ctx->DefineReturnType(Int8);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto conditionSym=ctx->LookupLocal("condition"); 
        Value *conditionValue = fx()->Load(LOC, entry, conditionSym);
        Builder *otherPath = cx()->OrphanBuilder(LOC, entry);
        doIfCmpZero(PASSLOC, entry, otherPath, conditionValue);
        fx()->Return(LOC, entry, bx()->ConstInt8(PASSLOC, entry, 0));
        fx()->Return(LOC, otherPath, bx()->ConstInt8(PASSLOC, otherPath, 1));
        return true;
    }

protected:
    virtual void doIfCmpZero(LOCATION, Builder *b, Builder *target, Value *condition)=0;

    const Type *_conditionType;
    condition_cType _conditionValue;
    int8_t _resultValue;
};

template<typename FuncPrototype, typename condition_cType>
class IfCmpZeroFunc : public IfCmpToZeroBaseFunc<FuncPrototype, condition_cType> {
public:
    IfCmpZeroFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpToZeroBaseFunc<FuncPrototype, condition_cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpZero(LOCATION, Builder *b, Builder *target, Value *condition) {
        return this->bx()->IfCmpEqualZero(PASSLOC, b, target, condition);
    }
};

TEST(omrgenExtension, IfCmpZeroInt8s) {
    typedef int8_t (FuncProto)(int8_t);
    IfCmpZeroFunc<FuncProto, int8_t> cmpzero_int8(LOC, "cmpzero_int8", c, false);
    cmpzero_int8.compile(LOC, Int8);
    cmpzero_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpzero_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpzero_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmpzero_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpzero_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpZeroInt16s) {
    typedef int8_t (FuncProto)(int16_t);
    IfCmpZeroFunc<FuncProto, int16_t> cmpzero_int16(LOC, "cmpzero_int16", c, false);
    cmpzero_int16.compile(LOC, Int16);
    cmpzero_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpzero_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpzero_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmpzero_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpzero_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpZeroInt32s) {
    typedef int8_t (FuncProto)(int32_t);
    IfCmpZeroFunc<FuncProto, int32_t> cmpzero_int32(LOC, "cmpzero_int32", c, false);
    cmpzero_int32.compile(LOC, Int32);
    cmpzero_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpzero_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpzero_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmpzero_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpzero_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpZeroInt64s) {
    typedef int8_t (FuncProto)(int64_t);
    IfCmpZeroFunc<FuncProto, int64_t> cmpzero_int64(LOC, "cmpzero_int64", c, false);
    cmpzero_int64.compile(LOC, Int64);
    cmpzero_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpzero_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpzero_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmpzero_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpzero_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpZeroFloat32s) {
    typedef int8_t (FuncProto)(float);
    IfCmpZeroFunc<FuncProto, float> cmpzero_float(LOC, "cmpzero_float", c, false);
    cmpzero_float.compile(LOC, Float32);
    cmpzero_float.test(LOC, Float32, static_cast<float>(0), static_cast<int8_t>(1));
    cmpzero_float.test(LOC, Float32, static_cast<float>(1), static_cast<int8_t>(0));
    cmpzero_float.test(LOC, Float32, static_cast<float>(-1), static_cast<int8_t>(0));
    cmpzero_float.test(LOC, Float32, std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmpzero_float.test(LOC, Float32, std::numeric_limits<float>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpZeroFloat64s) {
    typedef int8_t (FuncProto)(double);
    IfCmpZeroFunc<FuncProto, double> cmpzero_double(LOC, "cmpzero_double", c, false);
    cmpzero_double.compile(LOC, Float64);
    cmpzero_double.test(LOC, Float64, static_cast<double>(0), static_cast<int8_t>(1));
    cmpzero_double.test(LOC, Float64, static_cast<double>(1), static_cast<int8_t>(0));
    cmpzero_double.test(LOC, Float64, static_cast<double>(-1), static_cast<int8_t>(0));
    cmpzero_double.test(LOC, Float64, std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmpzero_double.test(LOC, Float64, std::numeric_limits<double>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpZeroAddresses) {
    typedef int8_t (FuncProto)(uintptr_t);
    IfCmpZeroFunc<FuncProto, uintptr_t> cmpzero_uintptr(LOC, "cmpzero_uintptr", c, false);
    cmpzero_uintptr.compile(LOC, Address);
    cmpzero_uintptr.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmpzero_uintptr.test(LOC, Address, static_cast<uintptr_t>(1), static_cast<int8_t>(0));
    cmpzero_uintptr.test(LOC, Address, static_cast<uintptr_t>(-1), static_cast<int8_t>(0));
    cmpzero_uintptr.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmpzero_uintptr.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
}

template<typename FuncPrototype, typename condition_cType>
class IfCmpNotZeroFunc : public IfCmpToZeroBaseFunc<FuncPrototype, condition_cType> {
public:
    IfCmpNotZeroFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpToZeroBaseFunc<FuncPrototype, condition_cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpZero(LOCATION, Builder *b, Builder *target, Value *condition) {
        return this->bx()->IfCmpNotEqualZero(PASSLOC, b, target, condition);
    }
};

TEST(omrgenExtension, IfCmpNotZeroInt8s) {
    typedef int8_t (FuncProto)(int8_t);
    IfCmpNotZeroFunc<FuncProto, int8_t> cmpnotzero_int8(LOC, "cmpnotzero_int8", c, false);
    cmpnotzero_int8.compile(LOC, Int8);
    cmpnotzero_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpnotzero_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpnotzero_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpnotzero_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpNotZeroInt16s) {
    typedef int8_t (FuncProto)(int16_t);
    IfCmpNotZeroFunc<FuncProto, int16_t> cmpnotzero_int16(LOC, "cmpnotzero_int16", c, false);
    cmpnotzero_int16.compile(LOC, Int16);
    cmpnotzero_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpnotzero_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpnotzero_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpnotzero_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpNotZeroInt32s) {
    typedef int8_t (FuncProto)(int32_t);
    IfCmpNotZeroFunc<FuncProto, int32_t> cmpnotzero_int32(LOC, "cmpnotzero_int32", c, false);
    cmpnotzero_int32.compile(LOC, Int32);
    cmpnotzero_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpnotzero_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpnotzero_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpnotzero_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpNotZeroInt64s) {
    typedef int8_t (FuncProto)(int64_t);
    IfCmpNotZeroFunc<FuncProto, int64_t> cmpnotzero_int64(LOC, "cmpnotzero_int64", c, false);
    cmpnotzero_int64.compile(LOC, Int64);
    cmpnotzero_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpnotzero_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpnotzero_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpnotzero_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpNotZeroFloat32s) {
    typedef int8_t (FuncProto)(float);
    IfCmpNotZeroFunc<FuncProto, float> cmpnotzero_float(LOC, "cmpnotzero_float", c, false);
    cmpnotzero_float.compile(LOC, Float32);
    cmpnotzero_float.test(LOC, Float32, static_cast<float>(0), static_cast<int8_t>(0));
    cmpnotzero_float.test(LOC, Float32, static_cast<float>(1), static_cast<int8_t>(1));
    cmpnotzero_float.test(LOC, Float32, static_cast<float>(-1), static_cast<int8_t>(1));
    cmpnotzero_float.test(LOC, Float32, std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpnotzero_float.test(LOC, Float32, std::numeric_limits<float>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpNotZeroFloat64s) {
    typedef int8_t (FuncProto)(double);
    IfCmpNotZeroFunc<FuncProto, double> cmpnotzero_double(LOC, "cmpnotzero_double", c, false);
    cmpnotzero_double.compile(LOC, Float64);
    cmpnotzero_double.test(LOC, Float64, static_cast<double>(0), static_cast<int8_t>(0));
    cmpnotzero_double.test(LOC, Float64, static_cast<double>(1), static_cast<int8_t>(1));
    cmpnotzero_double.test(LOC, Float64, static_cast<double>(-1), static_cast<int8_t>(1));
    cmpnotzero_double.test(LOC, Float64, std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpnotzero_double.test(LOC, Float64, std::numeric_limits<double>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpNotZeroAddresses) {
    typedef int8_t (FuncProto)(uintptr_t);
    IfCmpNotZeroFunc<FuncProto, uintptr_t> cmpnotzero_uintptr(LOC, "cmpnotzero_uintptr", c, false);
    cmpnotzero_uintptr.compile(LOC, Address);
    cmpnotzero_uintptr.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmpnotzero_uintptr.test(LOC, Address, static_cast<uintptr_t>(1), static_cast<int8_t>(1));
    cmpnotzero_uintptr.test(LOC, Address, static_cast<uintptr_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_uintptr.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmpnotzero_uintptr.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
}

// Base test class for IfCmp<condition> opcodes to evalute whether fall-through or taken path occurs
// Compiled code returns int8_t: 0 if fall-through path, 1 if taken path
template<typename FuncPrototype, typename cType>
class IfCmpOpBaseFunc : public TestFunc {
public:
    IfCmpOpBaseFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _type(NULL)
        , _leftValue(0)
        , _rightValue(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_leftValue,_rightValue), _resultValue) << "Compiled f(" << _leftValue << ", " << _rightValue << ") returns " << _resultValue;
    }
    void compile(LOCATION, const Type *type) {
        _type = type;
        this->TestFunc::compile(PASSLOC);
    }
    void test(LOCATION, const Type *type, cType leftValue, cType rightValue, int8_t resultValue) {
        _type = type;
        _leftValue = leftValue;
        _rightValue = rightValue;
        _resultValue = resultValue;
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineParameter("left", _type);
        ctx->DefineParameter("right", _type);
        ctx->DefineReturnType(Int8);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto leftSym=ctx->LookupLocal("left"); 
        Value *leftValue = fx()->Load(LOC, entry, leftSym);
        auto rightSym=ctx->LookupLocal("right"); 
        Value *rightValue = fx()->Load(LOC, entry, rightSym);
        Builder *otherPath = cx()->OrphanBuilder(LOC, entry);
        doIfCmpOp(PASSLOC, entry, otherPath, leftValue, rightValue);
        fx()->Return(LOC, entry, bx()->ConstInt8(LOC, entry, 0));
        fx()->Return(LOC, otherPath, bx()->ConstInt8(LOC, otherPath, 1));
        return true;
    }

protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right)=0;

    const Type *_type;
    cType _leftValue;
    cType _rightValue;
    int8_t _resultValue;
};

template<typename FuncPrototype, typename cType>
class IfCmpEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpEqualFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpEqualFunc<FuncProto, int8_t> cmpequal_int8(LOC, "cmpequal_int8", c, false);
    cmpequal_int8.compile(LOC, Int8);
    cmpequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpequal_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmpequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpEqualFunc<FuncProto, int16_t> cmpequal_int16(LOC, "cmpequal_int16", c, false);
    cmpequal_int16.compile(LOC, Int16);
    cmpequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpequal_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmpequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpEqualFunc<FuncProto, int32_t> cmpequal_int32(LOC, "cmpequal_int32", c, false);
    cmpequal_int32.compile(LOC, Int32);
    cmpequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpequal_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmpequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpEqualFunc<FuncProto, int64_t> cmpequal_int64(LOC, "cmpequal_int64", c, false);
    cmpequal_int64.compile(LOC, Int64);
    cmpequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpequal_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmpequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpEqualFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpEqualFunc<FuncProto, float> cmpequal_float(LOC, "cmpequal_float", c, false);
    cmpequal_float.compile(LOC, Float32);
    cmpequal_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(1));
    cmpequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(0));
    cmpequal_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(0));
    cmpequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(1));
    cmpequal_float.test(LOC, Float32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(0));
    cmpequal_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpequal_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    cmpequal_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmpequal_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpEqualFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpEqualFunc<FuncProto, double> cmpequal_double(LOC, "cmpequal_double", c, false);
    cmpequal_double.compile(LOC, Float64);
    cmpequal_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(1));
    cmpequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(0));
    cmpequal_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(0));
    cmpequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(1));
    cmpequal_double.test(LOC, Float64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(0));
    cmpequal_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpequal_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    cmpequal_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmpequal_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpEqualAddresses) {
    typedef int8_t (FuncProto)(intptr_t, intptr_t);
    IfCmpEqualFunc<FuncProto, intptr_t> cmpequal_address(LOC, "cmpequal_address", c, false);
    cmpequal_address.compile(LOC, Address);
    cmpequal_address.test(LOC, Address, static_cast<intptr_t>(0), static_cast<intptr_t>(0), static_cast<int8_t>(1));
    cmpequal_address.test(LOC, Address, static_cast<intptr_t>(4), static_cast<intptr_t>(0), static_cast<int8_t>(0));
    cmpequal_address.test(LOC, Address, static_cast<intptr_t>(0), static_cast<intptr_t>(4), static_cast<int8_t>(0));
    cmpequal_address.test(LOC, Address, static_cast<intptr_t>(4), static_cast<intptr_t>(4), static_cast<int8_t>(1));
    cmpequal_address.test(LOC, Address, std::numeric_limits<intptr_t>::min(), std::numeric_limits<intptr_t>::min(), static_cast<int8_t>(1));
    cmpequal_address.test(LOC, Address, std::numeric_limits<intptr_t>::min(), std::numeric_limits<intptr_t>::max(), static_cast<int8_t>(0));
    cmpequal_address.test(LOC, Address, std::numeric_limits<intptr_t>::max(), std::numeric_limits<intptr_t>::min(), static_cast<int8_t>(0));
    cmpequal_address.test(LOC, Address, std::numeric_limits<intptr_t>::max(), std::numeric_limits<intptr_t>::max(), static_cast<int8_t>(1));
}

template<typename FuncPrototype, typename cType>
class IfCmpNotEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpNotEqualFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpNotEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpNotEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpNotEqualFunc<FuncProto, int8_t> cmpnotequal_int8(LOC, "cmpnotequal_int8", c, false);
    cmpnotequal_int8.compile(LOC, Int8);
    cmpnotequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpnotequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpnotequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpnotequal_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpnotequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpNotEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpNotEqualFunc<FuncProto, int16_t> cmpnotequal_int16(LOC, "cmpnotequal_int16", c, false);
    cmpnotequal_int16.compile(LOC, Int16);
    cmpnotequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpnotequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpnotequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpnotequal_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpnotequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpNotEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpNotEqualFunc<FuncProto, int32_t> cmpnotequal_int32(LOC, "cmpnotequal_int32", c, false);
    cmpnotequal_int32.compile(LOC, Int32);
    cmpnotequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpnotequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpnotequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpnotequal_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpnotequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpNotEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpNotEqualFunc<FuncProto, int64_t> cmpnotequal_int64(LOC, "cmpnotequal_int64", c, false);
    cmpnotequal_int64.compile(LOC, Int64);
    cmpnotequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpnotequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpnotequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpnotequal_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpnotequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpNotEqualFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpNotEqualFunc<FuncProto, float> cmpnotequal_float(LOC, "cmpnotequal_float", c, false);
    cmpnotequal_float.compile(LOC, Float32);
    cmpnotequal_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(0));
    cmpnotequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(1));
    cmpnotequal_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(1));
    cmpnotequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpnotequal_float.test(LOC, Float32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(1));
    cmpnotequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(1));
    cmpnotequal_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmpnotequal_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    cmpnotequal_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpnotequal_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpNotEqualFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpNotEqualFunc<FuncProto, double> cmpnotequal_double(LOC, "cmpnotequal_double", c, false);
    cmpnotequal_double.compile(LOC, Float64);
    cmpnotequal_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(0));
    cmpnotequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(1));
    cmpnotequal_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(1));
    cmpnotequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpnotequal_double.test(LOC, Float64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(1));
    cmpnotequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(1));
    cmpnotequal_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmpnotequal_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    cmpnotequal_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpnotequal_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpNotEqualAddresses) {
    typedef int8_t (FuncProto)(intptr_t, intptr_t);
    IfCmpNotEqualFunc<FuncProto, intptr_t> cmpnotequal_address(LOC, "cmpnotequal_address", c, false);
    cmpnotequal_address.compile(LOC, Address);
    cmpnotequal_address.test(LOC, Address, static_cast<intptr_t>(0), static_cast<intptr_t>(0), static_cast<int8_t>(0));
    cmpnotequal_address.test(LOC, Address, static_cast<intptr_t>(4), static_cast<intptr_t>(0), static_cast<int8_t>(1));
    cmpnotequal_address.test(LOC, Address, static_cast<intptr_t>(0), static_cast<intptr_t>(4), static_cast<int8_t>(1));
    cmpnotequal_address.test(LOC, Address, static_cast<intptr_t>(4), static_cast<intptr_t>(4), static_cast<int8_t>(0));
    cmpnotequal_address.test(LOC, Address, std::numeric_limits<intptr_t>::min(), std::numeric_limits<intptr_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_address.test(LOC, Address, std::numeric_limits<intptr_t>::min(), std::numeric_limits<intptr_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_address.test(LOC, Address, std::numeric_limits<intptr_t>::max(), std::numeric_limits<intptr_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_address.test(LOC, Address, std::numeric_limits<intptr_t>::max(), std::numeric_limits<intptr_t>::max(), static_cast<int8_t>(0));
}

template<typename FuncPrototype, typename cType>
class IfCmpGreaterThanFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpGreaterThanFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpGreaterThan(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpGreaterThanInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpGreaterThanFunc<FuncProto, int8_t> cmpgreaterthan_int8(LOC, "cmpgreaterthan_int8", c, false);
    cmpgreaterthan_int8.compile(LOC, Int8);
    cmpgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpGreaterThanInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpGreaterThanFunc<FuncProto, int16_t> cmpgreaterthan_int16(LOC, "cmpgreaterthan_int16", c, false);
    cmpgreaterthan_int16.compile(LOC, Int16);
    cmpgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpGreaterThanInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpGreaterThanFunc<FuncProto, int32_t> cmpgreaterthan_int32(LOC, "cmpgreaterthan_int32", c, false);
    cmpgreaterthan_int32.compile(LOC, Int32);
    cmpgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpGreaterThanInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpGreaterThanFunc<FuncProto, int64_t> cmpgreaterthan_int64(LOC, "cmpgreaterthan_int64", c, false);
    cmpgreaterthan_int64.compile(LOC, Int64);
    cmpgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpGreaterThanFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpGreaterThanFunc<FuncProto, float> cmpgreaterthan_float(LOC, "cmpgreaterthan_float", c, false);
    cmpgreaterthan_float.compile(LOC, Float32);
    cmpgreaterthan_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(0));
    cmpgreaterthan_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(1));
    cmpgreaterthan_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterthan_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterthan_float.test(LOC, Float32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterthan_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpGreaterThanFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpGreaterThanFunc<FuncProto, double> cmpgreaterthan_double(LOC, "cmpgreaterthan_double", c, false);
    cmpgreaterthan_double.compile(LOC, Float64);
    cmpgreaterthan_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(0));
    cmpgreaterthan_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(1));
    cmpgreaterthan_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterthan_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterthan_double.test(LOC, Float64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterthan_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpGreaterThanAddresses) {
    typedef int8_t (FuncProto)(uintptr_t, uintptr_t);
    IfCmpGreaterThanFunc<FuncProto, uintptr_t> cmpgreaterthan_address(LOC, "cmpgreaterthan_address", c, false);
    cmpgreaterthan_address.compile(LOC, Address);
    cmpgreaterthan_address.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_address.test(LOC, Address, static_cast<uintptr_t>(4), static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_address.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<uintptr_t>(4), static_cast<int8_t>(0));
    cmpgreaterthan_address.test(LOC, Address, static_cast<uintptr_t>(4), static_cast<uintptr_t>(4), static_cast<int8_t>(0));
    cmpgreaterthan_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_address.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_address.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
}

template<typename FuncPrototype, typename cType>
class IfCmpGreaterOrEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpGreaterOrEqualFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpGreaterOrEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpGreaterOrEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpGreaterOrEqualFunc<FuncProto, int8_t> cmpgreaterorequal_int8(LOC, "cmpgreaterorequal_int8", c, false);
    cmpgreaterorequal_int8.compile(LOC, Int8);
    cmpgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpGreaterOrEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpGreaterOrEqualFunc<FuncProto, int16_t> cmpgreaterorequal_int16(LOC, "cmpgreaterorequal_int16", c, false);
    cmpgreaterorequal_int16.compile(LOC, Int16);
    cmpgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpGreaterOrEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpGreaterOrEqualFunc<FuncProto, int32_t> cmpgreaterorequal_int32(LOC, "cmpgreaterorequal_int32", c, false);
    cmpgreaterorequal_int32.compile(LOC, Int32);
    cmpgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpGreaterOrEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpGreaterOrEqualFunc<FuncProto, int64_t> cmpgreaterorequal_int64(LOC, "cmpgreaterorequal_int64", c, false);
    cmpgreaterorequal_int64.compile(LOC, Int64);
    cmpgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpGreaterOrEqualFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpGreaterOrEqualFunc<FuncProto, float> cmpgreaterorequal_float(LOC, "cmpgreaterorequal_float", c, false);
    cmpgreaterorequal_float.compile(LOC, Float32);
    cmpgreaterorequal_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_float.test(LOC, Float32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpGreaterOrEqualFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpGreaterOrEqualFunc<FuncProto, double> cmpgreaterorequal_double(LOC, "cmpgreaterorequal_double", c, false);
    cmpgreaterorequal_double.compile(LOC, Float64);
    cmpgreaterorequal_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_double.test(LOC, Float64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpGreaterOrEqualAddresses) {
    typedef int8_t (FuncProto)(uintptr_t, uintptr_t);
    IfCmpGreaterOrEqualFunc<FuncProto, uintptr_t> cmpgreaterorequal_address(LOC, "cmpgreaterorequal_address", c, false);
    cmpgreaterorequal_address.compile(LOC, Address);
    cmpgreaterorequal_address.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_address.test(LOC, Address, static_cast<uintptr_t>(4), static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_address.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<uintptr_t>(4), static_cast<int8_t>(0));
    cmpgreaterorequal_address.test(LOC, Address, static_cast<uintptr_t>(4), static_cast<uintptr_t>(4), static_cast<int8_t>(1));
    cmpgreaterorequal_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_address.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_address.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
}

template<typename FuncPrototype, typename cType>
class IfCmpLessThanFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpLessThanFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpLessThan(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpLessThanInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpLessThanFunc<FuncProto, int8_t> cmplessthan_int8(LOC, "cmplessthan_int8", c, false);
    cmplessthan_int8.compile(LOC, Int8);
    cmplessthan_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmplessthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmplessthan_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmplessthan_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmplessthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmplessthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpLessThanInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpLessThanFunc<FuncProto, int16_t> cmplessthan_int16(LOC, "cmplessthan_int16", c, false);
    cmplessthan_int16.compile(LOC, Int16);
    cmplessthan_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmplessthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmplessthan_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmplessthan_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmplessthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmplessthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpLessThanInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpLessThanFunc<FuncProto, int32_t> cmplessthan_int32(LOC, "cmplessthan_int32", c, false);
    cmplessthan_int32.compile(LOC, Int32);
    cmplessthan_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmplessthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmplessthan_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmplessthan_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmplessthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmplessthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpLessThanInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpLessThanFunc<FuncProto, int64_t> cmplessthan_int64(LOC, "cmplessthan_int64", c, false);
    cmplessthan_int64.compile(LOC, Int64);
    cmplessthan_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmplessthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmplessthan_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmplessthan_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmplessthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmplessthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpLessThanFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpLessThanFunc<FuncProto, float> cmplessthan_float(LOC, "cmplessthan_float", c, false);
    cmplessthan_float.compile(LOC, Float32);
    cmplessthan_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(0));
    cmplessthan_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(0));
    cmplessthan_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessthan_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(0));
    cmplessthan_float.test(LOC, Float32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessthan_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(0));
    cmplessthan_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmplessthan_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    cmplessthan_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmplessthan_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpLessThanFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpLessThanFunc<FuncProto, double> cmplessthan_double(LOC, "cmplessthan_double", c, false);
    cmplessthan_double.compile(LOC, Float64);
    cmplessthan_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(0));
    cmplessthan_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(0));
    cmplessthan_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessthan_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(0));
    cmplessthan_double.test(LOC, Float64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessthan_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(0));
    cmplessthan_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmplessthan_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    cmplessthan_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmplessthan_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpLessThanAddresses) {
    typedef int8_t (FuncProto)(uintptr_t, uintptr_t);
    IfCmpLessThanFunc<FuncProto, uintptr_t> cmplessthan_address(LOC, "cmplessthan_address", c, false);
    cmplessthan_address.compile(LOC, Address);
    cmplessthan_address.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmplessthan_address.test(LOC, Address, static_cast<uintptr_t>(4), static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmplessthan_address.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<uintptr_t>(4), static_cast<int8_t>(1));
    cmplessthan_address.test(LOC, Address, static_cast<uintptr_t>(4), static_cast<uintptr_t>(4), static_cast<int8_t>(0));
    cmplessthan_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmplessthan_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
    cmplessthan_address.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmplessthan_address.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
}

template<typename FuncPrototype, typename cType>
class IfCmpLessOrEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpLessOrEqualFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpLessOrEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpLessOrEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpLessOrEqualFunc<FuncProto, int8_t> cmplessorequal_int8(LOC, "cmplessorequal_int8", c, false);
    cmplessorequal_int8.compile(LOC, Int8);
    cmplessorequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmplessorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmplessorequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmplessorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpLessOrEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpLessOrEqualFunc<FuncProto, int16_t> cmplessorequal_int16(LOC, "cmplessorequal_int16", c, false);
    cmplessorequal_int16.compile(LOC, Int16);
    cmplessorequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmplessorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmplessorequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmplessorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpLessOrEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpLessOrEqualFunc<FuncProto, int32_t> cmplessorequal_int32(LOC, "cmplessorequal_int32", c, false);
    cmplessorequal_int32.compile(LOC, Int32);
    cmplessorequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmplessorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmplessorequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmplessorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpLessOrEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpLessOrEqualFunc<FuncProto, int64_t> cmplessorequal_int64(LOC, "cmplessorequal_int64", c, false);
    cmplessorequal_int64.compile(LOC, Int64);
    cmplessorequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmplessorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmplessorequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmplessorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpLessOrEqualFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpLessOrEqualFunc<FuncProto, float> cmplessorequal_float(LOC, "cmplessorequal_float", c, false);
    cmplessorequal_float.compile(LOC, Float32);
    cmplessorequal_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(1));
    cmplessorequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(0));
    cmplessorequal_float.test(LOC, Float32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessorequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessorequal_float.test(LOC, Float32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessorequal_float.test(LOC, Float32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(0));
    cmplessorequal_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmplessorequal_float.test(LOC, Float32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    cmplessorequal_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmplessorequal_float.test(LOC, Float32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpLessOrEqualFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpLessOrEqualFunc<FuncProto, double> cmplessorequal_double(LOC, "cmplessorequal_double", c, false);
    cmplessorequal_double.compile(LOC, Float64);
    cmplessorequal_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(1));
    cmplessorequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(0));
    cmplessorequal_double.test(LOC, Float64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessorequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessorequal_double.test(LOC, Float64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessorequal_double.test(LOC, Float64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(0));
    cmplessorequal_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmplessorequal_double.test(LOC, Float64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    cmplessorequal_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmplessorequal_double.test(LOC, Float64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpLessOrEqualAddresses) {
    typedef int8_t (FuncProto)(uintptr_t, uintptr_t);
    IfCmpLessOrEqualFunc<FuncProto, uintptr_t> cmplessorequal_address(LOC, "cmplessorequal_address", c, false);
    cmplessorequal_address.compile(LOC, Address);
    cmplessorequal_address.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmplessorequal_address.test(LOC, Address, static_cast<uintptr_t>(4), static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmplessorequal_address.test(LOC, Address, static_cast<uintptr_t>(0), static_cast<uintptr_t>(4), static_cast<int8_t>(1));
    cmplessorequal_address.test(LOC, Address, static_cast<uintptr_t>(4), static_cast<uintptr_t>(4), static_cast<int8_t>(1));
    cmplessorequal_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_address.test(LOC, Address, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_address.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_address.test(LOC, Address, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
}

template<typename FuncPrototype, typename cType>
class IfCmpUnsignedGreaterThanFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpUnsignedGreaterThanFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpUnsignedGreaterThan(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpUnsignedGreaterThanInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpUnsignedGreaterThanFunc<FuncProto, int8_t> cmpunsignedgreaterthan_int8(LOC, "cmpunsignedgreaterthan_int8", c, false);
    cmpunsignedgreaterthan_int8.compile(LOC, Int8);
    cmpunsignedgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpUnsignedGreaterThanInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpUnsignedGreaterThanFunc<FuncProto, int16_t> cmpunsignedgreaterthan_int16(LOC, "cmpunsignedgreaterthan_int16", c, false);
    cmpunsignedgreaterthan_int16.compile(LOC, Int16);
    cmpunsignedgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpUnsignedGreaterThanInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpUnsignedGreaterThanFunc<FuncProto, int32_t> cmpunsignedgreaterthan_int32(LOC, "cmpunsignedgreaterthan_int32", c, false);
    cmpunsignedgreaterthan_int32.compile(LOC, Int32);
    cmpunsignedgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpUnsignedGreaterThanInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpUnsignedGreaterThanFunc<FuncProto, int64_t> cmpunsignedgreaterthan_int64(LOC, "cmpunsignedgreaterthan_int64", c, false);
    cmpunsignedgreaterthan_int64.compile(LOC, Int64);
    cmpunsignedgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
}
// no unsigned comparisons for Float32,Float64,Address

template<typename FuncPrototype, typename cType>
class IfCmpUnsignedGreaterOrEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpUnsignedGreaterOrEqualFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpUnsignedGreaterOrEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpUnsignedGreaterOrEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpUnsignedGreaterOrEqualFunc<FuncProto, int8_t> cmpunsignedgreaterorequal_int8(LOC, "cmpunsignedgreaterorequal_int8", c, false);
    cmpunsignedgreaterorequal_int8.compile(LOC, Int8);
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpUnsignedGreaterOrEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpUnsignedGreaterOrEqualFunc<FuncProto, int16_t> cmpunsignedgreaterorequal_int16(LOC, "cmpunsignedgreaterorequal_int16", c, false);
    cmpunsignedgreaterorequal_int16.compile(LOC, Int16);
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpUnsignedGreaterOrEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpUnsignedGreaterOrEqualFunc<FuncProto, int32_t> cmpunsignedgreaterorequal_int32(LOC, "cmpunsignedgreaterorequal_int32", c, false);
    cmpunsignedgreaterorequal_int32.compile(LOC, Int32);
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpUnsignedGreaterOrEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpUnsignedGreaterOrEqualFunc<FuncProto, int64_t> cmpunsignedgreaterorequal_int64(LOC, "cmpunsignedgreaterorequal_int64", c, false);
    cmpunsignedgreaterorequal_int64.compile(LOC, Int64);
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
}
// no unsigned comparisons for Float32,Float64,Address

template<typename FuncPrototype, typename cType>
class IfCmpUnsignedLessThanFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpUnsignedLessThanFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpUnsignedLessThan(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpUnsignedLessThanInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpUnsignedLessThanFunc<FuncProto, int8_t> cmpunsignedlessthan_int8(LOC, "cmpunsignedlessthan_int8", c, false);
    cmpunsignedlessthan_int8.compile(LOC, Int8);
    cmpunsignedlessthan_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessthan_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpUnsignedLessThanInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpUnsignedLessThanFunc<FuncProto, int16_t> cmpunsignedlessthan_int16(LOC, "cmpunsignedlessthan_int16", c, false);
    cmpunsignedlessthan_int16.compile(LOC, Int16);
    cmpunsignedlessthan_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessthan_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpUnsignedLessThanInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpUnsignedLessThanFunc<FuncProto, int32_t> cmpunsignedlessthan_int32(LOC, "cmpunsignedlessthan_int32", c, false);
    cmpunsignedlessthan_int32.compile(LOC, Int32);
    cmpunsignedlessthan_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessthan_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
}
TEST(omrgenExtension, IfCmpUnsignedLessThanInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpUnsignedLessThanFunc<FuncProto, int64_t> cmpunsignedlessthan_int64(LOC, "cmpunsignedlessthan_int64", c, false);
    cmpunsignedlessthan_int64.compile(LOC, Int64);
    cmpunsignedlessthan_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessthan_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
}
// no unsigned comparisons for Float32,Float64,Address

template<typename FuncPrototype, typename cType>
class IfCmpUnsignedLessOrEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpUnsignedLessOrEqualFunc(LOCATION, String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(PASSLOC, name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpUnsignedLessOrEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpUnsignedLessOrEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpUnsignedLessOrEqualFunc<FuncProto, int8_t> cmpunsignedlessorequal_int8(LOC, "cmpunsignedlessorequal_int8", c, false);
    cmpunsignedlessorequal_int8.compile(LOC, Int8);
    cmpunsignedlessorequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int8.test(LOC, Int8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8.test(LOC, Int8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int8.test(LOC, Int8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8.test(LOC, Int8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpUnsignedLessOrEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpUnsignedLessOrEqualFunc<FuncProto, int16_t> cmpunsignedlessorequal_int16(LOC, "cmpunsignedlessorequal_int16", c, false);
    cmpunsignedlessorequal_int16.compile(LOC, Int16);
    cmpunsignedlessorequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int16.test(LOC, Int16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16.test(LOC, Int16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int16.test(LOC, Int16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16.test(LOC, Int16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpUnsignedLessOrEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpUnsignedLessOrEqualFunc<FuncProto, int32_t> cmpunsignedlessorequal_int32(LOC, "cmpunsignedlessorequal_int32", c, false);
    cmpunsignedlessorequal_int32.compile(LOC, Int32);
    cmpunsignedlessorequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int32.test(LOC, Int32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32.test(LOC, Int32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int32.test(LOC, Int32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32.test(LOC, Int32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
}
TEST(omrgenExtension, IfCmpUnsignedLessOrEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpUnsignedLessOrEqualFunc<FuncProto, int64_t> cmpunsignedlessorequal_int64(LOC, "cmpunsignedlessorequal_int64", c, false);
    cmpunsignedlessorequal_int64.compile(LOC, Int64);
    cmpunsignedlessorequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int64.test(LOC, Int64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64.test(LOC, Int64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int64.test(LOC, Int64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64.test(LOC, Int64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
}
// no unsigned comparisons for Float32,Float64,Address

// Test class to validate simple Goto opcode

// Test class for nested IfCmp opcodes to evalute more complicated conditional structures
// Since all the basic IfCmp opcodes work (earlier tests) just use IfCmpGreaterThan with Int64
// Compiled code returns int8_t and a different number for each of the possible paths.
class NestedControlFlowFunc : public TestFunc {
public:
    NestedControlFlowFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _numLeaves(0)
        , _numInternal(0)
        , _internalNodes(NULL)
        , _resultValue(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        typedef int64_t (FuncPrototype)(int64_t);
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);

        int64_t v=0;
        for (int64_t l=0;l < _numLeaves;l++) {
            int64_t nextLeaf = static_cast<int64_t>(1) << (l+1);
            for (;v < nextLeaf;v++) {
                EXPECT_EQ(f(v), l) << "Compiled f(" << v << ") returns " << l;
            }
        }
    }
    void compileAndRun(LOCATION, int8_t numLeaves, int8_t numInternal, int64_t *internalNodes) {
        _numLeaves = numLeaves;
        _numInternal = numInternal;
        _internalNodes = internalNodes;
        this->TestFunc::compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineParameter("path", Int64);
        ctx->DefineReturnType(Int64);
        return true;
    }

    // used to assemble the conditional nest from leaves to root
    typedef struct Node {
        Builder *_b;
        int64_t _label;
    } Node;

    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto pathSym=ctx->LookupLocal("path"); 
        Value *pathValue = fx()->Load(LOC, entry, pathSym);

        Allocator *mem = comp->mem();
        List<Node *> nodes(NULL, mem);
        for (int8_t l=0;l < _numLeaves;l++) {
            Builder *leafBuilder = cx()->OrphanBuilder(LOC, entry);
            int64_t label = static_cast<int64_t>(1) << l;
            Value *leafValue = bx()->ConstInt64(LOC, leafBuilder, l);
            fx()->Return(LOC, leafBuilder, leafValue);
            Node *leafNode = mem->allocate<Node>(1);
            leafNode->_b = leafBuilder;
            leafNode->_label = label;
            nodes.push_back(leafNode);
        }
        Builder *lastBuilder = NULL;
        for (int8_t i=0;i < _numInternal;i++) {
            int64_t nextLabel = _internalNodes[i];
            for (auto it=nodes.fwdIterator();it.hasTwoItems();it++) {
                Node *n1 = it.item();
                Node *n2 = it.secondItem();
                if (nextLabel == (n1->_label | n2->_label)) {
                    lastBuilder = cx()->OrphanBuilder(LOC, entry);
                    Value *labelValue = bx()->ConstInt64(LOC, lastBuilder, n2->_label);
                    Value *maskedValue = bx()->And(LOC, lastBuilder, pathValue, labelValue);
                    bx()->IfCmpNotEqualZero(LOC, lastBuilder, n2->_b, maskedValue);
                    bx()->Goto(LOC, lastBuilder, n1->_b);

                    Node *newNode = mem->allocate<Node>(1);
                    newNode->_b = lastBuilder;
                    newNode->_label = nextLabel;
                    nodes.insertBefore(newNode, it);
                    nodes.removeTwo(it);
                    break;
                }
            }
        }
        bx()->Goto(LOC, entry, lastBuilder);
        return true;
    }

protected:
    int8_t _numLeaves;
    int8_t _numInternal;
    int64_t *_internalNodes;
    int64_t _path;
    int64_t _resultValue;
};

TEST(omrgenExtension, NestedControlFlow2) {
    NestedControlFlowFunc nest2(LOC, "nest2", c, false);
    int64_t nodes[1] = { 3 };
    nest2.compileAndRun(LOC, 2, 1, nodes);
}
TEST(omrgenExtension, NestedControlFlow3L) {
    NestedControlFlowFunc nest3l(LOC, "nest3l", c, false);
    int64_t nodes[2] = { 3, 7 };
    nest3l.compileAndRun(LOC, 3, 2, nodes);
}
TEST(omrgenExtension, NestedControlFlow3R) {
    NestedControlFlowFunc nest3r(LOC, "nest3r", c, false);
    int64_t nodes[2] = { 6, 7 };
    nest3r.compileAndRun(LOC, 3, 2, nodes);
}
TEST(omrgenExtension, NestedControlFlow6) {
    NestedControlFlowFunc nest6(LOC, "nest6", c, false);
    int64_t nodes[5] = { 6, 14, 48, 62, 63 };
    nest6.compileAndRun(LOC, 6, 5, nodes);
}

// Base class for IndexAt
template<typename FuncPrototype, typename index_cType>
class IndexAtFunc : public TestFunc {
public:
    IndexAtFunc(LOCATION, String name, Compiler *compiler, bool log)
        : TestFunc(PASSLOC, compiler, log)
        , _elementType(NULL)
        , _baseValue(0)
        , _baseType(0)
        , _indexType(NULL)
        , _indexValue(0) {

        DefineName(name);
        DefineFile(__FILE__);
        DefineLine(LINETOSTR(__LINE__));
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_baseValue,_indexValue), _resultValue) << "Compiled f(" << _baseValue << ", " << (int64_t)_indexValue << ") returns " << _resultValue;
    }
    void test(LOCATION, const Type *elementType, uintptr_t baseValue, const Type *indexType, index_cType indexValue, uintptr_t resultValue) {
        _elementType = elementType;
        _baseValue = baseValue;
        _indexType = indexType;
        _indexValue = indexValue;
        _resultValue = resultValue;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _baseType = bx()->PointerTo(PASSLOC, comp, _elementType);
        ctx->DefineParameter("base", _baseType);
        ctx->DefineParameter("index", _indexType);
        ctx->DefineReturnType(_baseType);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto baseSym=ctx->LookupLocal("base"); 
        Value *baseValue = fx()->Load(LOC, entry, baseSym);
        auto indexSym=ctx->LookupLocal("index"); 
        Value *indexValue = fx()->Load(LOC, entry, indexSym);
        Value *result = bx()->IndexAt(PASSLOC, entry, baseValue, indexValue);
        fx()->Return(LOC, entry, result);
        return true;
    }

protected:
    const Type *_elementType;
    const Type *_baseType;
    uintptr_t _baseValue;
    const Type *_indexType;
    index_cType _indexValue;
    uintptr_t _resultValue;
};

TEST(omrgenExtension, IndexAtInt8byInt8) {
    typedef uintptr_t (FuncProto)(uintptr_t, int8_t);
    IndexAtFunc<FuncProto, int8_t> indexat_int8_int8(LOC, "indexat_int8_int8", c, false);
    char array[4];
    indexat_int8_int8.test(LOC, Int8, static_cast<uintptr_t>(0), Int8, static_cast<int8_t>(0), static_cast<uintptr_t>(0));
    indexat_int8_int8.test(LOC, Int8, static_cast<uintptr_t>(4), Int8, static_cast<int8_t>(4), static_cast<uintptr_t>(8));
    indexat_int8_int8.test(LOC, Int8, static_cast<uintptr_t>(8), Int8, static_cast<int8_t>(-4), static_cast<uintptr_t>(4));
    indexat_int8_int8.test(LOC, Int8, reinterpret_cast<uintptr_t>(array), Int8, static_cast<int8_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int8_int8.test(LOC, Int8, static_cast<uintptr_t>(0), Int8, std::numeric_limits<int8_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int8_t>::max()));
}
TEST(omrgenExtension, IndexAtInt8byInt16) {
    typedef uintptr_t (FuncProto)(uintptr_t, int16_t);
    IndexAtFunc<FuncProto, int16_t> indexat_int8_int16(LOC, "indexat_int8_int16", c, false);
    char array[4];
    indexat_int8_int16.test(LOC, Int8, static_cast<uintptr_t>(0), Int16, static_cast<int16_t>(0), static_cast<uintptr_t>(0));
    indexat_int8_int16.test(LOC, Int8, static_cast<uintptr_t>(4), Int16, static_cast<int16_t>(4), static_cast<uintptr_t>(8));
    indexat_int8_int16.test(LOC, Int8, static_cast<uintptr_t>(8), Int16, static_cast<int16_t>(-4), static_cast<uintptr_t>(4));
    indexat_int8_int16.test(LOC, Int8, reinterpret_cast<uintptr_t>(array), Int16, static_cast<int16_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int8_int16.test(LOC, Int8, static_cast<uintptr_t>(0), Int16, std::numeric_limits<int16_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int16_t>::max()));
}
TEST(omrgenExtension, IndexAtInt8byInt32) {
    typedef uintptr_t (FuncProto)(uintptr_t, int32_t);
    IndexAtFunc<FuncProto, int32_t> indexat_int8_int32(LOC, "indexat_int8_int32", c, false);
    char array[4];
    indexat_int8_int32.test(LOC, Int8, static_cast<uintptr_t>(0), Int32, static_cast<int32_t>(0), static_cast<uintptr_t>(0));
    indexat_int8_int32.test(LOC, Int8, static_cast<uintptr_t>(4), Int32, static_cast<int32_t>(4), static_cast<uintptr_t>(8));
    indexat_int8_int32.test(LOC, Int8, static_cast<uintptr_t>(8), Int32, static_cast<int32_t>(-4), static_cast<uintptr_t>(4));
    indexat_int8_int32.test(LOC, Int8, reinterpret_cast<uintptr_t>(array), Int32, static_cast<int32_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int8_int32.test(LOC, Int8, static_cast<uintptr_t>(0), Int32, std::numeric_limits<int32_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int32_t>::max()));
}
TEST(omrgenExtension, IndexAtInt8byInt64) {
    typedef uintptr_t (FuncProto)(uintptr_t, int64_t);
    IndexAtFunc<FuncProto, int64_t> indexat_int8_int64(LOC, "indexat_int8_int64", c, false);
    char array[4];
    indexat_int8_int64.test(LOC, Int8, static_cast<uintptr_t>(0), Int64, static_cast<int64_t>(0), static_cast<uintptr_t>(0));
    indexat_int8_int64.test(LOC, Int8, static_cast<uintptr_t>(4), Int64, static_cast<int64_t>(4), static_cast<uintptr_t>(8));
    indexat_int8_int64.test(LOC, Int8, static_cast<uintptr_t>(8), Int64, static_cast<int64_t>(-4), static_cast<uintptr_t>(4));
    indexat_int8_int64.test(LOC, Int8, reinterpret_cast<uintptr_t>(array), Int64, static_cast<int64_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int8_int64.test(LOC, Int8, static_cast<uintptr_t>(0), Int64, std::numeric_limits<int64_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int64_t>::max()));
}
TEST(omrgenExtension, IndexAtInt16byInt8) {
    typedef uintptr_t (FuncProto)(uintptr_t, int8_t);
    IndexAtFunc<FuncProto, int8_t> indexat_int16_int8(LOC, "indexat_int16_int8", c, false);
    uint16_t array[4];
    indexat_int16_int8.test(LOC, Int16, static_cast<uintptr_t>(0), Int8, static_cast<int8_t>(0), static_cast<uintptr_t>(0));
    indexat_int16_int8.test(LOC, Int16, static_cast<uintptr_t>(4), Int8, static_cast<int8_t>(4), static_cast<uintptr_t>(12));
    indexat_int16_int8.test(LOC, Int16, static_cast<uintptr_t>(8), Int8, static_cast<int8_t>(-4), static_cast<uintptr_t>(0));
    indexat_int16_int8.test(LOC, Int16, reinterpret_cast<uintptr_t>(array), Int8, static_cast<int8_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int16_int8.test(LOC, Int16, static_cast<uintptr_t>(0), Int8, std::numeric_limits<int8_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int8_t>::max()) << 1);
}
TEST(omrgenExtension, IndexAtInt16byInt16) {
    typedef uintptr_t (FuncProto)(uintptr_t, int16_t);
    IndexAtFunc<FuncProto, int16_t> indexat_int16_int16(LOC, "indexat_int16_int16", c, false);
    uint16_t array[4];
    indexat_int16_int16.test(LOC, Int16, static_cast<uintptr_t>(0), Int16, static_cast<int16_t>(0), static_cast<uintptr_t>(0));
    indexat_int16_int16.test(LOC, Int16, static_cast<uintptr_t>(4), Int16, static_cast<int16_t>(4), static_cast<uintptr_t>(12));
    indexat_int16_int16.test(LOC, Int16, static_cast<uintptr_t>(8), Int16, static_cast<int16_t>(-4), static_cast<uintptr_t>(0));
    indexat_int16_int16.test(LOC, Int16, reinterpret_cast<uintptr_t>(array), Int16, static_cast<int16_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int16_int16.test(LOC, Int16, static_cast<uintptr_t>(0), Int16, std::numeric_limits<int16_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int16_t>::max()) << 1);
}
TEST(omrgenExtension, IndexAtInt16byInt32) {
    typedef uintptr_t (FuncProto)(uintptr_t, int32_t);
    IndexAtFunc<FuncProto, int32_t> indexat_int16_int32(LOC, "indexat_int16_int32", c, false);
    uint16_t array[4];
    indexat_int16_int32.test(LOC, Int16, static_cast<uintptr_t>(0), Int32, static_cast<int32_t>(0), static_cast<uintptr_t>(0));
    indexat_int16_int32.test(LOC, Int16, static_cast<uintptr_t>(4), Int32, static_cast<int32_t>(4), static_cast<uintptr_t>(12));
    indexat_int16_int32.test(LOC, Int16, static_cast<uintptr_t>(8), Int32, static_cast<int32_t>(-4), static_cast<uintptr_t>(0));
    indexat_int16_int32.test(LOC, Int16, reinterpret_cast<uintptr_t>(array), Int32, static_cast<int32_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int16_int32.test(LOC, Int16, static_cast<uintptr_t>(0), Int32, std::numeric_limits<int32_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int32_t>::max()) << 1);
}
TEST(omrgenExtension, IndexAtInt16byInt64) {
    typedef uintptr_t (FuncProto)(uintptr_t, int64_t);
    IndexAtFunc<FuncProto, int64_t> indexat_int16_int64(LOC, "indexat_int16_int64", c, false);
    uint16_t array[4];
    indexat_int16_int64.test(LOC, Int16, static_cast<uintptr_t>(0), Int64, static_cast<int64_t>(0), static_cast<uintptr_t>(0));
    indexat_int16_int64.test(LOC, Int16, static_cast<uintptr_t>(4), Int64, static_cast<int64_t>(4), static_cast<uintptr_t>(12));
    indexat_int16_int64.test(LOC, Int16, static_cast<uintptr_t>(8), Int64, static_cast<int64_t>(-4), static_cast<uintptr_t>(0));
    indexat_int16_int64.test(LOC, Int16, reinterpret_cast<uintptr_t>(array), Int64, static_cast<int64_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int16_int64.test(LOC, Int16, static_cast<uintptr_t>(0), Int64, std::numeric_limits<int64_t>::max() >> 1, static_cast<uintptr_t>((std::numeric_limits<int64_t>::max() >> 1) << 1));
}
TEST(omrgenExtension, IndexAtInt32byInt8) {
    typedef uintptr_t (FuncProto)(uintptr_t, int8_t);
    IndexAtFunc<FuncProto, int8_t> indexat_int32_int8(LOC, "indexat_int32_int8", c, false);
    uint32_t array[4];
    indexat_int32_int8.test(LOC, Int32, static_cast<uintptr_t>(0), Int8, static_cast<int8_t>(0), static_cast<uintptr_t>(0));
    indexat_int32_int8.test(LOC, Int32, static_cast<uintptr_t>(4), Int8, static_cast<int8_t>(4), static_cast<uintptr_t>(20));
    indexat_int32_int8.test(LOC, Int32, static_cast<uintptr_t>(20), Int8, static_cast<int8_t>(-4), static_cast<uintptr_t>(4));
    indexat_int32_int8.test(LOC, Int32, reinterpret_cast<uintptr_t>(array), Int8, static_cast<int8_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int32_int8.test(LOC, Int32, static_cast<uintptr_t>(0), Int8, std::numeric_limits<int8_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int8_t>::max()) << 2);
}
TEST(omrgenExtension, IndexAtInt32byInt16) {
    typedef uintptr_t (FuncProto)(uintptr_t, int16_t);
    IndexAtFunc<FuncProto, int16_t> indexat_int32_int16(LOC, "indexat_int32_int16", c, false);
    uint32_t array[4];
    indexat_int32_int16.test(LOC, Int32, static_cast<uintptr_t>(0), Int16, static_cast<int16_t>(0), static_cast<uintptr_t>(0));
    indexat_int32_int16.test(LOC, Int32, static_cast<uintptr_t>(4), Int16, static_cast<int16_t>(4), static_cast<uintptr_t>(20));
    indexat_int32_int16.test(LOC, Int32, static_cast<uintptr_t>(20), Int16, static_cast<int16_t>(-4), static_cast<uintptr_t>(4));
    indexat_int32_int16.test(LOC, Int32, reinterpret_cast<uintptr_t>(array), Int16, static_cast<int16_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int32_int16.test(LOC, Int32, static_cast<uintptr_t>(0), Int16, std::numeric_limits<int16_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int16_t>::max()) << 2);
}
TEST(omrgenExtension, IndexAtInt32byInt32) {
    typedef uintptr_t (FuncProto)(uintptr_t, int32_t);
    IndexAtFunc<FuncProto, int32_t> indexat_int32_int32(LOC, "indexat_int32_int32", c, false);
    uint32_t array[4];
    indexat_int32_int32.test(LOC, Int32, static_cast<uintptr_t>(0), Int32, static_cast<int32_t>(0), static_cast<uintptr_t>(0));
    indexat_int32_int32.test(LOC, Int32, static_cast<uintptr_t>(4), Int32, static_cast<int32_t>(4), static_cast<uintptr_t>(20));
    indexat_int32_int32.test(LOC, Int32, static_cast<uintptr_t>(20), Int32, static_cast<int32_t>(-4), static_cast<uintptr_t>(4));
    indexat_int32_int32.test(LOC, Int32, reinterpret_cast<uintptr_t>(array), Int32, static_cast<int32_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int32_int32.test(LOC, Int32, static_cast<uintptr_t>(0), Int32, std::numeric_limits<int32_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int32_t>::max()) << 2);
}
TEST(omrgenExtension, IndexAtInt32byInt64) {
    typedef uintptr_t (FuncProto)(uintptr_t, int64_t);
    IndexAtFunc<FuncProto, int64_t> indexat_int32_int64(LOC, "indexat_int32_int64", c, false);
    uint32_t array[4];
    indexat_int32_int64.test(LOC, Int32, static_cast<uintptr_t>(0), Int64, static_cast<int64_t>(0), static_cast<uintptr_t>(0));
    indexat_int32_int64.test(LOC, Int32, static_cast<uintptr_t>(4), Int64, static_cast<int64_t>(4), static_cast<uintptr_t>(20));
    indexat_int32_int64.test(LOC, Int32, static_cast<uintptr_t>(20), Int64, static_cast<int64_t>(-4), static_cast<uintptr_t>(4));
    indexat_int32_int64.test(LOC, Int32, reinterpret_cast<uintptr_t>(array), Int64, static_cast<int64_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int32_int64.test(LOC, Int32, static_cast<uintptr_t>(0), Int64, std::numeric_limits<int64_t>::max() >> 2, static_cast<uintptr_t>((std::numeric_limits<int64_t>::max() >> 2) << 2));
}
TEST(omrgenExtension, IndexAtInt64byInt8) {
    typedef uintptr_t (FuncProto)(uintptr_t, int8_t);
    IndexAtFunc<FuncProto, int8_t> indexat_int64_int8(LOC, "indexat_int64_int8", c, false);
    uint64_t array[4];
    indexat_int64_int8.test(LOC, Int64, static_cast<uintptr_t>(0), Int8, static_cast<int8_t>(0), static_cast<uintptr_t>(0));
    indexat_int64_int8.test(LOC, Int64, static_cast<uintptr_t>(4), Int8, static_cast<int8_t>(4), static_cast<uintptr_t>(36));
    indexat_int64_int8.test(LOC, Int64, static_cast<uintptr_t>(36), Int8, static_cast<int8_t>(-4), static_cast<uintptr_t>(4));
    indexat_int64_int8.test(LOC, Int64, reinterpret_cast<uintptr_t>(array), Int8, static_cast<int8_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int64_int8.test(LOC, Int64, static_cast<uintptr_t>(0), Int8, std::numeric_limits<int8_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int8_t>::max()) << 3);
}
TEST(omrgenExtension, IndexAtInt64byInt16) {
    typedef uintptr_t (FuncProto)(uintptr_t, int16_t);
    IndexAtFunc<FuncProto, int16_t> indexat_int64_int16(LOC, "indexat_int64_int16", c, false);
    uint64_t array[4];
    indexat_int64_int16.test(LOC, Int64, static_cast<uintptr_t>(0), Int16, static_cast<int16_t>(0), static_cast<uintptr_t>(0));
    indexat_int64_int16.test(LOC, Int64, static_cast<uintptr_t>(4), Int16, static_cast<int16_t>(4), static_cast<uintptr_t>(36));
    indexat_int64_int16.test(LOC, Int64, static_cast<uintptr_t>(36), Int16, static_cast<int16_t>(-4), static_cast<uintptr_t>(4));
    indexat_int64_int16.test(LOC, Int64, reinterpret_cast<uintptr_t>(array), Int16, static_cast<int16_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int64_int16.test(LOC, Int64, static_cast<uintptr_t>(0), Int16, std::numeric_limits<int16_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int16_t>::max()) << 3);
}
TEST(omrgenExtension, IndexAtInt64byInt32) {
    typedef uintptr_t (FuncProto)(uintptr_t, int32_t);
    IndexAtFunc<FuncProto, int32_t> indexat_int64_int32(LOC, "indexat_int64_int32", c, false);
    uint64_t array[4];
    indexat_int64_int32.test(LOC, Int64, static_cast<uintptr_t>(0), Int32, static_cast<int32_t>(0), static_cast<uintptr_t>(0));
    indexat_int64_int32.test(LOC, Int64, static_cast<uintptr_t>(4), Int32, static_cast<int32_t>(4), static_cast<uintptr_t>(36));
    indexat_int64_int32.test(LOC, Int64, static_cast<uintptr_t>(36), Int32, static_cast<int32_t>(-4), static_cast<uintptr_t>(4));
    indexat_int64_int32.test(LOC, Int64, reinterpret_cast<uintptr_t>(array), Int32, static_cast<int32_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int64_int32.test(LOC, Int64, static_cast<uintptr_t>(0), Int32, std::numeric_limits<int32_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int32_t>::max()) << 3);
}
TEST(omrgenExtension, IndexAtInt64byInt64) {
    typedef uintptr_t (FuncProto)(uintptr_t, int64_t);
    IndexAtFunc<FuncProto, int64_t> indexat_int64_int64(LOC, "indexat_int64_int64", c, false);
    uint64_t array[4];
    indexat_int64_int64.test(LOC, Int64, static_cast<uintptr_t>(0), Int64, static_cast<int64_t>(0), static_cast<uintptr_t>(0));
    indexat_int64_int64.test(LOC, Int64, static_cast<uintptr_t>(4), Int64, static_cast<int64_t>(4), static_cast<uintptr_t>(36));
    indexat_int64_int64.test(LOC, Int64, static_cast<uintptr_t>(36), Int64, static_cast<int64_t>(-4), static_cast<uintptr_t>(4));
    indexat_int64_int64.test(LOC, Int64, reinterpret_cast<uintptr_t>(array), Int64, static_cast<int64_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int64_int64.test(LOC, Int64, static_cast<uintptr_t>(0), Int64, std::numeric_limits<int64_t>::max() >> 3, static_cast<uintptr_t>((std::numeric_limits<int64_t>::max() >> 3) << 3));
}
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
#include "jb2cg/omrgen.hpp"

using namespace OMR::JB2;

static Compiler *getCompiler();
static Base::BaseExtension *getBase();

static Compiler *c = getCompiler();
static Config *config = getCompiler()->config();
static CoreExtension *cx = NULL;
static Base::BaseExtension *bx = NULL;
static Func::FunctionExtension *fx = NULL;
//static JB::JBExtension *jbx = NULL;
static omrgen::OMRExtension *ogx = NULL;
static const TypeID tInt8 = getBase()->tInt8;
static const TypeID tInt16 = getBase()->tInt16;
static const TypeID tInt32 = getBase()->tInt32;
static const TypeID tInt64 = getBase()->tInt64;
static const TypeID tFloat32 = getBase()->tFloat32;
static const TypeID tFloat64 = getBase()->tFloat64;
static const TypeID tAddress = getBase()->tAddress;
static const TypeID tWord = getBase()->tWord;

static Compiler *getCompiler() {
    static bool initialized = false;
    static Compiler global("Global Compiler");
    if (!initialized) {
        initialized=true;

        c = &global;
        cx = getCompiler()->lookupExtension<CoreExtension>();
        bx = c->loadExtension<Base::BaseExtension>(LOC);
        fx = c->loadExtension<Func::FunctionExtension>(LOC);
        ogx = c->loadExtension<omrgen::OMRExtension>(LOC, OMRGEN_NAME);
        //jbx = c->loadExtension<JB::JBExtension>(LOC);
    }
    return c;
}

static Base::BaseExtension *getBase() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        Compiler *c = getCompiler();
        bx = c->lookupExtension<Base::BaseExtension>();
    }
    return bx;
}


int main(int argc, char** argv) {
    void *handle = dlopen(OMR_JB2_CORELIB, RTLD_LAZY);
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
    TestSetup(MEM_LOCATION(a), Compiler *compiler, bool log, String name)
        : Func::Function(MEM_PASSLOC(a), compiler, name)
        , _compiler(compiler)
        , _config(compiler->config())
        , _lgr(std::cout, String("  "))
        , _wrt(log ? &_lgr : NULL) {

        if (log) {
            _config->setTraceStrategy();
            _config->setTracePrototypeIR();
            _config->setTraceVisitor();
            _config->setTraceBuildIL();
            _config->setLogger(&_lgr);
        }
    }
    virtual ~TestSetup() {
        _config->setTraceBuildIL(false); // reset for next test because config is global
        _config->setTraceVisitor(false); // reset for next test because config is global
        _config->setTracePrototypeIR(false);
        _config->setTraceStrategy(false); // reset for next test because config is global
        _config->setLogger(NULL); // reset for next test because config is global
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
    //JB::JBExtension *jbx() const { return ::jbx; }

private:
    Compiler *_compiler;
    Config *_config;
    TextLogger _lgr;
    TextLogger *_wrt;
};

class TestFunc : public TestSetup {
public:
    TestFunc(MEM_LOCATION(a), Compiler *compiler, String name, String file, String line, bool log)
        : TestSetup(MEM_PASSLOC(a), compiler, log, name) {
        DefineName(name);
        DefineFile(file);
        DefineLine(line);
    }

    void compile(LOCATION) {
        const StrategyID codegenStrategy = cx()->strategyCodegen;
        _body = fx()->compile(PASSLOC, this, codegenStrategy, wrt());
        EXPECT_NE(_body, nullptr) << "Compiled body ok";
        EXPECT_EQ((int)_body->rc(), (int)compiler()->CompileSuccessful) << "Compiled function ok";
        if (_body->rc() != compiler()->CompileSuccessful)
            std::cerr << "Compiler return code " << (int)_body->rc() << " is " << compiler()->returnCodeName(_body->rc()).c_str() << "\n";
    }

protected:
    CompiledBody *body() const { return _body; }

private:
    CompiledBody *_body;
};

// Return void
class ReturnVoidFunc : public TestFunc {
public:
    ReturnVoidFunc(MEM_LOCATION(a), Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, "ReturnVoidFunc", __FILE__, LINETOSTR(__LINE__), log) {
    }
    
    void run(LOCATION) {
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        ctx->DefineReturnType(comp->ir()->NoType);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        fx()->Return(LOC, entry);
        return true;
    }
};


TEST(omrgenExtension, ReturnVoid) {
    ReturnVoidFunc *rvf = new (c->mem()) ReturnVoidFunc(MEM_LOC(c->mem()), c, false);
    rvf->compile(LOC);
    delete rvf;
}

// Return primitive value
template<typename FuncPrototype, typename cType>
class ReturnValueFunc : public TestFunc {
public:
    ReturnValueFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _tid(NoTypeID)
        , _type(NULL)
        , _value(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(), _value) << "Compiled f() returns " << _value;
    }
    void test(LOCATION, TypeID tid, cType value) {
        _tid = tid;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _type = comp->ir()->typedict()->Lookup(_tid);
        ctx->DefineReturnType(_type);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        Literal *lv = _type->literal(PASSLOC, reinterpret_cast<const LiteralBytes *>(&_value));
        Value *c = bx()->Const(LOC, entry, lv);
        fx()->Return(LOC, entry, c);
        return true;
    }

private:
    TypeID _tid;
    const Type *_type;
    cType _value;
};

TEST(omrgenExtension, ReturnConstInt8) {
    typedef int8_t (FuncProto)();
    ReturnValueFunc<FuncProto, int8_t> *rpt_int8 = new (c->mem()) ReturnValueFunc<FuncProto, int8_t>(MEM_LOC(c->mem()), "ReturnConstInt8", c, false);
    rpt_int8->test(LOC, tInt8, 0);
    rpt_int8->test(LOC, tInt8, 3);
    rpt_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    rpt_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    delete rpt_int8;
}
TEST(omrgenExtension, ReturnConstInt16) {
    typedef int16_t (FuncProto)();
    ReturnValueFunc<FuncProto, int16_t> *rpt_int16 = new (c->mem()) ReturnValueFunc<FuncProto, int16_t>(MEM_LOC(c->mem()), "ReturnConstInt16", c, false);
    rpt_int16->test(LOC, tInt16, 0);
    rpt_int16->test(LOC, tInt16, 3);
    rpt_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    rpt_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    delete rpt_int16;
}
TEST(omrgenExtension, ReturnConstInt32) {
    typedef int32_t (FuncProto)();
    ReturnValueFunc<FuncProto, int32_t> *rpt_int32 = new (c->mem()) ReturnValueFunc<FuncProto, int32_t>(MEM_LOC(c->mem()), "ReturnConstInt32", c, false);
    rpt_int32->test(LOC, tInt32, 0);
    rpt_int32->test(LOC, tInt32, 3);
    rpt_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    rpt_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    delete rpt_int32;
}
TEST(omrgenExtension, ReturnConstInt64) {
    typedef int64_t (FuncProto)();
    ReturnValueFunc<FuncProto, int64_t> *rpt_int64 = new (c->mem()) ReturnValueFunc<FuncProto, int64_t>(MEM_LOC(c->mem()), "ReturnConstInt64", c, false);
    rpt_int64->test(LOC, tInt64, 0);
    rpt_int64->test(LOC, tInt64, 3);
    rpt_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    rpt_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    delete rpt_int64;
}
TEST(omrgenExtension, ReturnConstFloat32) {
    typedef float (FuncProto)();
    ReturnValueFunc<FuncProto, float> *rpt_float32 = new (c->mem()) ReturnValueFunc<FuncProto, float>(MEM_LOC(c->mem()), "ReturnConstFloat32", c, false);
    rpt_float32->test(LOC, tFloat32, 0);
    rpt_float32->test(LOC, tFloat32, 3);
    rpt_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    rpt_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    delete rpt_float32;
}
TEST(omrgenExtension, ReturnConstFloat64) {
    typedef double (FuncProto)();
    ReturnValueFunc<FuncProto, double> *rpt_float64 = new (c->mem()) ReturnValueFunc<FuncProto, double>(MEM_LOC(c->mem()), "ReturnConstFloat64", c, false);
    rpt_float64->test(LOC, tFloat64, 0);
    rpt_float64->test(LOC, tFloat64, 3);
    rpt_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    rpt_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    delete rpt_float64;
}
TEST(omrgenExtension, ReturnConstAddress) {
    typedef uintptr_t (FuncProto)();
    ReturnValueFunc<FuncProto, uintptr_t> *rpt_address = new (c->mem()) ReturnValueFunc<FuncProto, uintptr_t>(MEM_LOC(c->mem()), "ReturnConstAddress", c, false);
    rpt_address->test(LOC, tAddress, 0);
    rpt_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    rpt_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    delete rpt_address;
}

// Test returning the value of a paramater
template<typename FuncPrototype, typename cType>
class ReturnParameterFunc : public TestFunc {
public:
    ReturnParameterFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _tid(NoTypeID)
        , _type(NULL)
        , _value(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_value), _value) << "Compiled f(" << _value << ") returns " << _value;
    }
    void test(LOCATION, const TypeID tid, cType value) {
        _tid = tid;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _type = comp->ir()->typedict()->Lookup(_tid);
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
    TypeID _tid;
    const Type *_type;
    cType _value;
};

TEST(omrgenExtension, ReturnParamInt8) {
    typedef int8_t (FuncProto)(int8_t);
    ReturnParameterFunc<FuncProto, int8_t> *rpp_int8 = new (c->mem()) ReturnParameterFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "ReturnParamInt8", c, false);
    rpp_int8->test(LOC, tInt8, 0);
    rpp_int8->test(LOC, tInt8, 3);
    rpp_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    rpp_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    delete rpp_int8;
}
TEST(omrgenExtension, ReturnParamInt16) {
    typedef int16_t (FuncProto)(int16_t);
    ReturnParameterFunc<FuncProto, int16_t> *rpp_int16 = new (c->mem()) ReturnParameterFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "ReturnParamInt16", c, false);
    rpp_int16->test(LOC, tInt16, 0);
    rpp_int16->test(LOC, tInt16, 3);
    rpp_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    rpp_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    delete rpp_int16;
}
TEST(omrgenExtension, ReturnParamInt32) {
    typedef int32_t (FuncProto)(int32_t);
    ReturnParameterFunc<FuncProto, int32_t> *rpp_int32 = new (c->mem()) ReturnParameterFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "ReturnParamInt32", c, false);
    rpp_int32->test(LOC, tInt32, 0);
    rpp_int32->test(LOC, tInt32, 3);
    rpp_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    rpp_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    delete rpp_int32;
}
TEST(omrgenExtension, ReturnParamInt64) {
    typedef int64_t (FuncProto)(int64_t);
    ReturnParameterFunc<FuncProto, int64_t> *rpp_int64 = new (c->mem()) ReturnParameterFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "ReturnParamInt64", c, false);
    rpp_int64->test(LOC, tInt64, 0);
    rpp_int64->test(LOC, tInt64, 3);
    rpp_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    rpp_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    delete rpp_int64;
}
TEST(omrgenExtension, ReturnParamFloat32) {
    typedef float (FuncProto)(float);
    ReturnParameterFunc<FuncProto, float> *rpp_float32 = new (c->mem()) ReturnParameterFunc<FuncProto,float>(MEM_LOC(c->mem()), "ReturnParamFloat32", c, false);
    rpp_float32->test(LOC, tFloat32, 0);
    rpp_float32->test(LOC, tFloat32, 3);
    rpp_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    rpp_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    delete rpp_float32;
}
TEST(omrgenExtension, ReturnParamFloat64) {
    typedef double (FuncProto)(double);
    ReturnParameterFunc<FuncProto, double> *rpp_float64 = new (c->mem()) ReturnParameterFunc<FuncProto,double>(MEM_LOC(c->mem()), "ReturnParamFloat64", c, false);
    rpp_float64->test(LOC, tFloat64, 0);
    rpp_float64->test(LOC, tFloat64, 3);
    rpp_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    rpp_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    delete rpp_float64;
}
TEST(omrgenExtension, ReturnParamAddress) {
    typedef uintptr_t (FuncProto)(uintptr_t);
    ReturnParameterFunc<FuncProto, uintptr_t> *rpp_address = new (c->mem()) ReturnParameterFunc<FuncProto,uintptr_t>(MEM_LOC(c->mem()), "ReturnParamAddress", c, false);
    rpp_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    rpp_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    rpp_address->test(LOC, tAddress, reinterpret_cast<uintptr_t>(&rpp_address));
    delete rpp_address;
}

// Test returning the value of a local variable where a parameter value has been stored
template<typename FuncPrototype, typename cType>
class ReturnLocalFunc : public TestFunc {
public:
    ReturnLocalFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _tid(NoTypeID)
        , _type(NULL)
        , _valueSym(NULL)
        , _tempSym(NULL)
        , _value(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_value), _value) << "Compiled f(" << _value << ") returns " << _value;
    }
    void test(LOCATION, const TypeID tid, cType value) {
        _tid = tid;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _type = comp->ir()->typedict()->Lookup(_tid);
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
    TypeID _tid;
    const Type *_type;
    Func::ParameterSymbol *_valueSym;
    Func::LocalSymbol *_tempSym;
    cType _value;
};

TEST(omrgenExtension, ReturnLocalInt8) {
    typedef int8_t (FuncProto)(int8_t);
    ReturnLocalFunc<FuncProto, int8_t> *rlp_int8 = new (c->mem()) ReturnLocalFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "ReturnLocalInt8", c, false);
    rlp_int8->test(LOC, tInt8, 0);
    rlp_int8->test(LOC, tInt8, 3);
    rlp_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    rlp_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    delete rlp_int8;
}
TEST(omrgenExtension, ReturnLocalInt16) {
    typedef int16_t (FuncProto)(int16_t);
    ReturnLocalFunc<FuncProto, int16_t> *rlp_int16 = new (c->mem()) ReturnLocalFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "ReturnLocalInt16", c, false);
    rlp_int16->test(LOC, tInt16, 0);
    rlp_int16->test(LOC, tInt16, 3);
    rlp_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    rlp_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    delete rlp_int16;
}
TEST(omrgenExtension, ReturnLocalInt32) {
    typedef int32_t (FuncProto)(int32_t);
    ReturnLocalFunc<FuncProto, int32_t> *rlp_int32 = new (c->mem()) ReturnLocalFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "ReturnLocalInt32", c, false);
    rlp_int32->test(LOC, tInt32, 0);
    rlp_int32->test(LOC, tInt32, 3);
    rlp_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    rlp_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    delete rlp_int32;
}
TEST(omrgenExtension, ReturnLocalInt64) {
    typedef int64_t (FuncProto)(int64_t);
    ReturnLocalFunc<FuncProto, int64_t> *rlp_int64 = new (c->mem()) ReturnLocalFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "ReturnLocalInt64", c, false);
    rlp_int64->test(LOC, tInt64, 0);
    rlp_int64->test(LOC, tInt64, 3);
    rlp_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    rlp_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    delete rlp_int64;
}
TEST(omrgenExtension, ReturnLocalFloat32) {
    typedef float (FuncProto)(float);
    ReturnLocalFunc<FuncProto, float> *rlp_float32 = new (c->mem()) ReturnLocalFunc<FuncProto,float>(MEM_LOC(c->mem()), "ReturnLocalFloat32", c, false);
    rlp_float32->test(LOC, tFloat32, 0);
    rlp_float32->test(LOC, tFloat32, 3);
    rlp_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    rlp_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    delete rlp_float32;
}
TEST(omrgenExtension, ReturnLocalFloat64) {
    typedef double (FuncProto)(double);
    ReturnLocalFunc<FuncProto, double> *rlp_float64 = new (c->mem()) ReturnLocalFunc<FuncProto,double>(MEM_LOC(c->mem()), "ReturnLocalFloat64", c, false);
    rlp_float64->test(LOC, tFloat64, 0);
    rlp_float64->test(LOC, tFloat64, 3);
    rlp_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    rlp_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    delete rlp_float64;
}
TEST(omrgenExtension, ReturnLocalAddress) {
    typedef uintptr_t (FuncProto)(uintptr_t);
    ReturnLocalFunc<FuncProto, uintptr_t> *rlp_address = new (c->mem()) ReturnLocalFunc<FuncProto,uintptr_t>(MEM_LOC(c->mem()), "ReturnLocalAddress", c, false);
    rlp_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    rlp_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    rlp_address->test(LOC, tAddress, reinterpret_cast<uintptr_t>(&rlp_address));
    delete rlp_address;
}

// Test returning a converted parameter value
template<typename FuncPrototype, typename cTypeFrom, typename cTypeTo>
class ConvertToFunc : public TestFunc {
public:
    ConvertToFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _tidFrom(NoTypeID)
        , _tidTo(NoTypeID)
        , _typeFrom(NULL)
        , _typeTo(NULL)
        , _valueSym(NULL)
        , _valueFrom(0)
        , _expectedResult(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_valueFrom), _expectedResult) << "Compiled f(" << _valueFrom << ") returns " << _expectedResult;
    }
    void test(LOCATION, bool doCompile, const TypeID tidFrom, const TypeID tidTo, cTypeFrom value, cTypeTo expectedResult) {
        _tidFrom = tidFrom;
        _tidTo = tidTo;
        _valueFrom = value;
        _expectedResult = expectedResult;
        if (doCompile)
            compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _typeFrom = comp->ir()->typedict()->Lookup(_tidFrom);
        _typeTo = comp->ir()->typedict()->Lookup(_tidTo);
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
    TypeID _tidFrom;
    TypeID _tidTo;
    const Type *_typeFrom;
    const Type *_typeTo;
    Func::ParameterSymbol *_valueSym;
    cTypeTo _valueFrom;
    cTypeFrom _expectedResult;
};

// test converting Int8 to other primitive types
TEST(omrgenExtension, ConvertInt8ToInt8) {
    typedef int8_t (FuncProtoInt8)(int8_t);
    ConvertToFunc<FuncProtoInt8, int8_t, int8_t> *ct_int8_int8 = new (c->mem()) ConvertToFunc<FuncProtoInt8,int8_t,int8_t>(MEM_LOC(c->mem()), "ConvertInt8ToInt8", c, false);
    ct_int8_int8->test(LOC, true, tInt8, tInt8, 0, 0);
    ct_int8_int8->test(LOC, false, tInt8, tInt8, 3, 3);
    ct_int8_int8->test(LOC, false, tInt8, tInt8, std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_int8_int8->test(LOC, false, tInt8, tInt8, std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
    delete ct_int8_int8;
}
TEST(omrgenExtension, ConvertInt8ToInt16) {
    typedef int16_t (FuncProtoInt16)(int8_t);
    ConvertToFunc<FuncProtoInt16, int8_t, int16_t> *ct_int8_int16 = new (c->mem()) ConvertToFunc<FuncProtoInt16,int8_t,int16_t>(MEM_LOC(c->mem()), "ConvertInt8ToInt16", c, false);
    ct_int8_int16->test(LOC, true, tInt8, tInt16, 0, (int16_t)0);
    ct_int8_int16->test(LOC, false, tInt8, tInt16, 3, (int16_t)3);
    ct_int8_int16->test(LOC, false, tInt8, tInt16, std::numeric_limits<int8_t>::min(), (int16_t) std::numeric_limits<int8_t>::min());
    ct_int8_int16->test(LOC, false, tInt8, tInt16, std::numeric_limits<int8_t>::max(), (int16_t) std::numeric_limits<int8_t>::max());
    delete ct_int8_int16;
}
TEST(omrgenExtension, ConvertInt8ToInt32) {
    typedef int32_t (FuncProtoInt32)(int8_t);
    ConvertToFunc<FuncProtoInt32, int8_t, int32_t> *ct_int8_int32 = new (c->mem()) ConvertToFunc<FuncProtoInt32,int8_t,int32_t>(MEM_LOC(c->mem()), "ConvertInt8ToInt32", c, false);
    ct_int8_int32->test(LOC, true, tInt8, tInt32, 0, (int32_t)0);
    ct_int8_int32->test(LOC, false, tInt8, tInt32, 3, (int32_t)3);
    ct_int8_int32->test(LOC, false, tInt8, tInt32, std::numeric_limits<int8_t>::min(), (int32_t) std::numeric_limits<int8_t>::min());
    ct_int8_int32->test(LOC, false, tInt8, tInt32, std::numeric_limits<int8_t>::max(), (int32_t) std::numeric_limits<int8_t>::max());
    delete ct_int8_int32;
}
TEST(omrgenExtension, ConvertInt8ToInt64) {
    typedef int64_t (FuncProtoInt64)(int8_t);
    ConvertToFunc<FuncProtoInt64, int8_t, int64_t> *ct_int8_int64 = new (c->mem()) ConvertToFunc<FuncProtoInt64,int8_t,int64_t>(MEM_LOC(c->mem()), "ConvertInt8ToInt64", c, false);
    ct_int8_int64->test(LOC, true, tInt8, tInt64, 0, (int64_t)0);
    ct_int8_int64->test(LOC, false, tInt8, tInt64, 3, (int64_t)3);
    ct_int8_int64->test(LOC, false, tInt8, tInt64, std::numeric_limits<int8_t>::min(), (int64_t) std::numeric_limits<int8_t>::min());
    ct_int8_int64->test(LOC, false, tInt8, tInt64, std::numeric_limits<int8_t>::max(), (int64_t) std::numeric_limits<int8_t>::max());
    delete ct_int8_int64;
}
TEST(omrgenExtension, ConvertInt8ToFloat32) {
    typedef float (FuncProtoFloat32)(int8_t);
    ConvertToFunc<FuncProtoFloat32, int8_t, float> *ct_int8_float32 = new (c->mem()) ConvertToFunc<FuncProtoFloat32,int8_t,float>(MEM_LOC(c->mem()), "ConvertInt8ToFloat32", c, false);
    ct_int8_float32->test(LOC, true, tInt8, tFloat32, 0, (float)0);
    ct_int8_float32->test(LOC, false, tInt8, tFloat32, 3, (float)3);
    ct_int8_float32->test(LOC, false, tInt8, tFloat32, std::numeric_limits<int8_t>::min(), (float) std::numeric_limits<int8_t>::min());
    ct_int8_float32->test(LOC, false, tInt8, tFloat32, std::numeric_limits<int8_t>::max(), (float) std::numeric_limits<int8_t>::max());
    delete ct_int8_float32;
}
TEST(omrgenExtension, ConvertInt8ToFloat64) {
    typedef double (FuncProtoFloat64)(int8_t);
    ConvertToFunc<FuncProtoFloat64, int8_t, double> *ct_int8_float64 = new (c->mem()) ConvertToFunc<FuncProtoFloat64,int8_t,double>(MEM_LOC(c->mem()), "ConvertInt8ToFloat64", c, false);
    ct_int8_float64->test(LOC, true, tInt8, tFloat64, 0, (float)0);
    ct_int8_float64->test(LOC, false, tInt8, tFloat64, 3, (float)3);
    ct_int8_float64->test(LOC, false, tInt8, tFloat64, std::numeric_limits<int8_t>::min(), (double) std::numeric_limits<int8_t>::min());
    ct_int8_float64->test(LOC, false, tInt8, tFloat64, std::numeric_limits<int8_t>::max(), (double) std::numeric_limits<int8_t>::max());
    delete ct_int8_float64;
}

// test converting Int16 to other primitive types
TEST(omrgenExtension, ConvertInt16ToInt8) {
    typedef int8_t (FuncProtoInt8)(int16_t);
    ConvertToFunc<FuncProtoInt8, int16_t, int8_t> *ct_int16_int8 = new (c->mem()) ConvertToFunc<FuncProtoInt8,int16_t,int8_t>(MEM_LOC(c->mem()), "ConvertInt16ToInt8", c, false);
    ct_int16_int8->test(LOC, true, tInt16, tInt8, 0, (int8_t)0);
    ct_int16_int8->test(LOC, false, tInt16, tInt8, 3, (int8_t)3);
    ct_int16_int8->test(LOC, false, tInt16, tInt8, std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_int16_int8->test(LOC, false, tInt16, tInt8, std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
    delete ct_int16_int8;
}
TEST(omrgenExtension, ConvertInt16ToInt16) {
    typedef int16_t (FuncProtoInt16)(int16_t);
    ConvertToFunc<FuncProtoInt16, int16_t, int16_t> *ct_int16_int16 = new (c->mem()) ConvertToFunc<FuncProtoInt16,int16_t,int16_t>(MEM_LOC(c->mem()), "ConvertInt16ToInt16", c, false);
    ct_int16_int16->test(LOC, true, tInt16, tInt16, 0, (int16_t)0);
    ct_int16_int16->test(LOC, false, tInt16, tInt16, 3, (int16_t)3);
    ct_int16_int16->test(LOC, false, tInt16, tInt16, std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_int16_int16->test(LOC, false, tInt16, tInt16, std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
    delete ct_int16_int16;
}
TEST(omrgenExtension, ConvertInt16ToInt32) {
    typedef int32_t (FuncProtoInt32)(int16_t);
    ConvertToFunc<FuncProtoInt32, int16_t, int32_t> *ct_int16_int32 = new (c->mem()) ConvertToFunc<FuncProtoInt32,int16_t,int32_t>(MEM_LOC(c->mem()), "ConvertInt16ToInt32", c, false);
    ct_int16_int32->test(LOC, true, tInt16, tInt32, 0, (int32_t)0);
    ct_int16_int32->test(LOC, false, tInt16, tInt32, 3, (int32_t)3);
    ct_int16_int32->test(LOC, false, tInt16, tInt32, std::numeric_limits<int16_t>::min(), (int32_t) std::numeric_limits<int16_t>::min());
    ct_int16_int32->test(LOC, false, tInt16, tInt32, std::numeric_limits<int16_t>::max(), (int32_t) std::numeric_limits<int16_t>::max());
    delete ct_int16_int32;
}
TEST(omrgenExtension, ConvertInt16ToInt64) {
    typedef int64_t (FuncProtoInt64)(int16_t);
    ConvertToFunc<FuncProtoInt64, int16_t, int64_t> *ct_int16_int64 = new (c->mem()) ConvertToFunc<FuncProtoInt64,int16_t,int64_t>(MEM_LOC(c->mem()), "ConvertInt16ToInt64", c, false);
    ct_int16_int64->test(LOC, true, tInt16, tInt64, 0, (int64_t)0);
    ct_int16_int64->test(LOC, false, tInt16, tInt64, 3, (int64_t)3);
    ct_int16_int64->test(LOC, false, tInt16, tInt64, std::numeric_limits<int16_t>::min(), (int64_t) std::numeric_limits<int16_t>::min());
    ct_int16_int64->test(LOC, false, tInt16, tInt64, std::numeric_limits<int16_t>::max(), (int64_t) std::numeric_limits<int16_t>::max());
    delete ct_int16_int64;
}
TEST(omrgenExtension, ConvertInt16ToFloat32) {
    typedef float (FuncProtoFloat32)(int16_t);
    ConvertToFunc<FuncProtoFloat32, int16_t, float> *ct_int16_float32 = new (c->mem()) ConvertToFunc<FuncProtoFloat32,int16_t,float>(MEM_LOC(c->mem()), "ConvertInt16ToFloat32", c, false);
    ct_int16_float32->test(LOC, true, tInt16, tFloat32, 0, (float)0);
    ct_int16_float32->test(LOC, false, tInt16, tFloat32, 3, (float)3);
    ct_int16_float32->test(LOC, false, tInt16, tFloat32, std::numeric_limits<int16_t>::min(), (float)std::numeric_limits<int16_t>::min());
    ct_int16_float32->test(LOC, false, tInt16, tFloat32, std::numeric_limits<int16_t>::max(), (float)std::numeric_limits<int16_t>::max());
    delete ct_int16_float32;
}
TEST(omrgenExtension, ConvertInt16ToFloat64) {
    typedef double (FuncProtoFloat64)(int16_t);
    ConvertToFunc<FuncProtoFloat64, int16_t, float> *ct_int16_float64 = new (c->mem()) ConvertToFunc<FuncProtoFloat64,int16_t,float>(MEM_LOC(c->mem()), "ConvertInt16ToFloat64", c, false);
    ct_int16_float64->test(LOC, true, tInt16, tFloat64, 0, (double)0);
    ct_int16_float64->test(LOC, false, tInt16, tFloat64, 3, (double)3);
    ct_int16_float64->test(LOC, false, tInt16, tFloat64, std::numeric_limits<int16_t>::min(), (double)std::numeric_limits<int16_t>::min());
    ct_int16_float64->test(LOC, false, tInt16, tFloat64, std::numeric_limits<int16_t>::max(), (double)std::numeric_limits<int16_t>::max());
    delete ct_int16_float64;
}

// test converting Int32 to other primitive types
TEST(omrgenExtension, ConvertInt32ToInt8) {
    typedef int8_t (FuncProtoInt8)(int32_t);
    ConvertToFunc<FuncProtoInt8, int32_t, int8_t> *ct_int32_int8 = new (c->mem()) ConvertToFunc<FuncProtoInt8,int32_t,int8_t>(MEM_LOC(c->mem()), "ConvertInt32ToInt8", c, false);
    ct_int32_int8->test(LOC, true, tInt32, tInt8, (int32_t)0, (int8_t)0);
    ct_int32_int8->test(LOC, false, tInt32, tInt8, (int32_t)3, (int8_t)3);
    ct_int32_int8->test(LOC, false, tInt32, tInt8, (int32_t)std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_int32_int8->test(LOC, false, tInt32, tInt8, (int32_t)std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
    delete ct_int32_int8;
}
TEST(omrgenExtension, ConvertInt32ToInt16) {
    typedef int16_t (FuncProtoInt16)(int32_t);
    ConvertToFunc<FuncProtoInt16, int32_t, int16_t> *ct_int32_int16 = new (c->mem()) ConvertToFunc<FuncProtoInt16,int32_t,int16_t>(MEM_LOC(c->mem()), "ConvertInt32ToInt16", c, false);
    ct_int32_int16->test(LOC, true, tInt32, tInt16, (int32_t)0, (int16_t)0);
    ct_int32_int16->test(LOC, false, tInt32, tInt16, (int32_t)3, (int16_t)3);
    ct_int32_int16->test(LOC, false, tInt32, tInt16, (int32_t)std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_int32_int16->test(LOC, false, tInt32, tInt16, (int32_t)std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
    delete ct_int32_int16;
}
TEST(omrgenExtension, ConvertInt32ToInt32) {
    typedef int32_t (FuncProtoInt32)(int32_t);
    ConvertToFunc<FuncProtoInt32, int32_t, int32_t> *ct_int32_int32 = new (c->mem()) ConvertToFunc<FuncProtoInt32,int32_t,int32_t>(MEM_LOC(c->mem()), "ConvertInt32ToInt32", c, false);
    ct_int32_int32->test(LOC, true, tInt32, tInt32, (int32_t)0, (int32_t)0);
    ct_int32_int32->test(LOC, false, tInt32, tInt32, (int32_t)3, (int32_t)3);
    ct_int32_int32->test(LOC, false, tInt32, tInt32, (int32_t)std::numeric_limits<int32_t>::min(), (int32_t) std::numeric_limits<int32_t>::min());
    ct_int32_int32->test(LOC, false, tInt32, tInt32, (int32_t)std::numeric_limits<int32_t>::max(), (int32_t) std::numeric_limits<int32_t>::max());
    delete ct_int32_int32;
}
TEST(omrgenExtension, ConvertInt32ToInt64) {
    typedef int64_t (FuncProtoInt64)(int32_t);
    ConvertToFunc<FuncProtoInt64, int32_t, int64_t> *ct_int32_int64 = new (c->mem()) ConvertToFunc<FuncProtoInt64,int32_t,int64_t>(MEM_LOC(c->mem()), "ConvertInt32ToInt64", c, false);
    ct_int32_int64->test(LOC, true, tInt32, tInt64, (int32_t)0, (int64_t)0);
    ct_int32_int64->test(LOC, false, tInt32, tInt64, (int32_t)3, (int64_t)3);
    ct_int32_int64->test(LOC, false, tInt32, tInt64, (int32_t)std::numeric_limits<int32_t>::min(), (int64_t) std::numeric_limits<int32_t>::min());
    ct_int32_int64->test(LOC, false, tInt32, tInt64, (int32_t)std::numeric_limits<int32_t>::max(), (int64_t) std::numeric_limits<int32_t>::max());
    delete ct_int32_int64;
}
TEST(omrgenExtension, ConvertInt32ToFloat32) {
    typedef float (FuncProtoFloat32)(int32_t);
    ConvertToFunc<FuncProtoFloat32, int32_t, float> *ct_int32_float32 = new (c->mem()) ConvertToFunc<FuncProtoFloat32,int32_t,float>(MEM_LOC(c->mem()), "ConvertInt32ToFloat32", c, false);
    ct_int32_float32->test(LOC, true, tInt32, tFloat32, (int32_t)0, (float)0);
    ct_int32_float32->test(LOC, false, tInt32, tFloat32, (int32_t)3, (float)3);
    ct_int32_float32->test(LOC, false, tInt32, tFloat32, (int32_t)std::numeric_limits<int32_t>::min(), (float)std::numeric_limits<int32_t>::min());
    ct_int32_float32->test(LOC, false, tInt32, tFloat32, (int32_t)std::numeric_limits<int32_t>::max(), (float)std::numeric_limits<int32_t>::max());
    delete ct_int32_float32;
}
TEST(omrgenExtension, ConvertInt32ToFloat64) {
    typedef double (FuncProtoFloat64)(int32_t);
    ConvertToFunc<FuncProtoFloat64, int32_t, double> *ct_int32_float64 = new (c->mem()) ConvertToFunc<FuncProtoFloat64,int32_t,double>(MEM_LOC(c->mem()), "ConvertInt32ToFloat64", c, false);
    ct_int32_float64->test(LOC, true, tInt32, tFloat64, (int32_t)0, (double)0);
    ct_int32_float64->test(LOC, false, tInt32, tFloat64, (int32_t)3, (double)3);
    ct_int32_float64->test(LOC, false, tInt32, tFloat64, (int32_t)std::numeric_limits<int32_t>::min(), (double)std::numeric_limits<int32_t>::min());
    ct_int32_float64->test(LOC, false, tInt32, tFloat64, (int32_t)std::numeric_limits<int32_t>::max(), (double)std::numeric_limits<int32_t>::max());
    delete ct_int32_float64;
}

// test converting Int64 to other primitive types
TEST(omrgenExtension, ConvertInt64ToInt8) {
    typedef int8_t (FuncProtoInt8)(int64_t);
    ConvertToFunc<FuncProtoInt8, int64_t, int8_t> *ct_int64_int8 = new (c->mem()) ConvertToFunc<FuncProtoInt8,int64_t,int8_t>(MEM_LOC(c->mem()), "ConvertInt64ToInt8", c, false);
    ct_int64_int8->test(LOC, true, tInt64, tInt8, (int64_t)0, (int8_t)0);
    ct_int64_int8->test(LOC, false, tInt64, tInt8, (int64_t)3, (int8_t)3);
    ct_int64_int8->test(LOC, false, tInt64, tInt8, (int64_t)std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_int64_int8->test(LOC, false, tInt64, tInt8, (int64_t)std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
    delete ct_int64_int8;
}
TEST(omrgenExtension, ConvertInt64ToInt16) {
    typedef int16_t (FuncProtoInt16)(int64_t);
    ConvertToFunc<FuncProtoInt16, int64_t, int16_t> *ct_int64_int16 = new (c->mem()) ConvertToFunc<FuncProtoInt16,int64_t,int16_t>(MEM_LOC(c->mem()), "ConvertInt64ToInt16", c, false);
    ct_int64_int16->test(LOC, true, tInt64, tInt16, (int64_t)0, (int16_t)0);
    ct_int64_int16->test(LOC, false, tInt64, tInt16, (int64_t)3, (int16_t)3);
    ct_int64_int16->test(LOC, false, tInt64, tInt16, (int64_t)std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_int64_int16->test(LOC, false, tInt64, tInt16, (int64_t)std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
    delete ct_int64_int16;
}
TEST(omrgenExtension, ConvertInt64ToInt32) {
    typedef int32_t (FuncProtoInt32)(int64_t);
    ConvertToFunc<FuncProtoInt32, int64_t, int32_t> *ct_int64_int32 = new (c->mem()) ConvertToFunc<FuncProtoInt32,int64_t,int32_t>(MEM_LOC(c->mem()), "ConvertInt64ToInt32", c, false);
    ct_int64_int32->test(LOC, true, tInt64, tInt32, (int64_t)0, (int32_t)0);
    ct_int64_int32->test(LOC, false, tInt64, tInt32, (int64_t)3, (int32_t)3);
    ct_int64_int32->test(LOC, false, tInt64, tInt32, (int64_t)std::numeric_limits<int32_t>::min(), (int32_t) std::numeric_limits<int32_t>::min());
    ct_int64_int32->test(LOC, false, tInt64, tInt32, (int64_t)std::numeric_limits<int32_t>::max(), (int32_t) std::numeric_limits<int32_t>::max());
    delete ct_int64_int32;
}
TEST(omrgenExtension, ConvertInt64ToInt64) {
    typedef int64_t (FuncProtoInt64)(int64_t);
    ConvertToFunc<FuncProtoInt64, int64_t, int64_t> *ct_int64_int64 = new (c->mem()) ConvertToFunc<FuncProtoInt64,int64_t,int64_t>(MEM_LOC(c->mem()), "ConvertInt64ToInt64", c, false);
    ct_int64_int64->test(LOC, true, tInt64, tInt64, (int64_t)0, (int64_t)0);
    ct_int64_int64->test(LOC, false, tInt64, tInt64, (int64_t)3, (int64_t)3);
    ct_int64_int64->test(LOC, false, tInt64, tInt64, (int64_t)std::numeric_limits<int64_t>::min(), (int64_t) std::numeric_limits<int64_t>::min());
    ct_int64_int64->test(LOC, false, tInt64, tInt64, (int64_t)std::numeric_limits<int64_t>::max(), (int64_t) std::numeric_limits<int64_t>::max());
    delete ct_int64_int64;
}
TEST(omrgenExtension, ConvertInt64ToFloat32) {
    typedef float (FuncProtoFloat32)(int64_t);
    ConvertToFunc<FuncProtoFloat32, int64_t, float> *ct_int64_float32 = new (c->mem()) ConvertToFunc<FuncProtoFloat32,int64_t,float>(MEM_LOC(c->mem()), "ConvertInt64ToFloat32", c, false);
    ct_int64_float32->test(LOC, true, tInt64, tFloat32, (int64_t)0, (float)0);
    ct_int64_float32->test(LOC, false, tInt64, tFloat32, (int64_t)3, (float)3);
    ct_int64_float32->test(LOC, false, tInt64, tFloat32, (int64_t)std::numeric_limits<int64_t>::min(), (float)std::numeric_limits<int64_t>::min());
    ct_int64_float32->test(LOC, false, tInt64, tFloat32, (int64_t)std::numeric_limits<int64_t>::max(), (float)std::numeric_limits<int64_t>::max());
    delete ct_int64_float32;
}
TEST(omrgenExtension, ConvertInt64ToFloat64) {
    typedef double (FuncProtoFloat64)(int64_t);
    ConvertToFunc<FuncProtoFloat64, int64_t, double> *ct_int64_float64 = new (c->mem()) ConvertToFunc<FuncProtoFloat64,int64_t,double>(MEM_LOC(c->mem()), "ConvertInt64ToFloat64", c, false);
    ct_int64_float64->test(LOC, true, tInt64, tFloat64, (int64_t)0, (double)0);
    ct_int64_float64->test(LOC, false, tInt64, tFloat64, (int64_t)3, (double)3);
    ct_int64_float64->test(LOC, false, tInt64, tFloat64, (int64_t)std::numeric_limits<int64_t>::min(), (double)std::numeric_limits<int64_t>::min());
    ct_int64_float64->test(LOC, false, tInt64, tFloat64, (int64_t)std::numeric_limits<int64_t>::max(), (double)std::numeric_limits<int64_t>::max());
    delete ct_int64_float64;
}

// test converting Float32 to other primitive types
TEST(omrgenExtension, ConvertFloat32ToInt8) {
    typedef int8_t (FuncProtoInt8)(float);
    ConvertToFunc<FuncProtoInt8, float, int8_t> *ct_float32_int8 = new (c->mem()) ConvertToFunc<FuncProtoInt8,float,int8_t>(MEM_LOC(c->mem()), "ConvertFloat32ToInt8", c, false);
    ct_float32_int8->test(LOC, true, tFloat32, tInt8, (float)0, (int8_t)0);
    ct_float32_int8->test(LOC, false, tFloat32, tInt8, (float)3, (int8_t)3);
    ct_float32_int8->test(LOC, false, tFloat32, tInt8, (float)std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_float32_int8->test(LOC, false, tFloat32, tInt8, (float)std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
    delete ct_float32_int8;
}
TEST(omrgenExtension, ConvertFloat32ToInt16) {
    typedef int16_t (FuncProtoInt16)(float);
    ConvertToFunc<FuncProtoInt16, float, int16_t> *ct_float32_int16 = new (c->mem()) ConvertToFunc<FuncProtoInt16,float,int16_t>(MEM_LOC(c->mem()), "ConvertFloat32ToInt16", c, false);
    ct_float32_int16->test(LOC, true, tFloat32, tInt16, (float)0, (int16_t)0);
    ct_float32_int16->test(LOC, false, tFloat32, tInt16, (float)3, (int16_t)3);
    ct_float32_int16->test(LOC, false, tFloat32, tInt16, (float)std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_float32_int16->test(LOC, false, tFloat32, tInt16, (float)std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
    delete ct_float32_int16;
}
TEST(omrgenExtension, ConvertFloat32ToInt32) {
    typedef int32_t (FuncProtoInt32)(float);
    ConvertToFunc<FuncProtoInt32, float, int32_t> *ct_float32_int32 = new (c->mem()) ConvertToFunc<FuncProtoInt32,float,int32_t>(MEM_LOC(c->mem()), "ConvertFloat32ToInt32", c, false);
    ct_float32_int32->test(LOC, true, tFloat32, tInt32, (float)0, (int32_t)0);
    ct_float32_int32->test(LOC, false, tFloat32, tInt32, (float)3, (int32_t)3);
    ct_float32_int32->test(LOC, false, tFloat32, tInt32, (float)std::numeric_limits<int32_t>::min(), (int32_t)(float)std::numeric_limits<int32_t>::min());
    ct_float32_int32->test(LOC, false, tFloat32, tInt32, (float)std::numeric_limits<int32_t>::max(), (int32_t)(float)std::numeric_limits<int32_t>::max());
    delete ct_float32_int32;
}
TEST(omrgenExtension, ConvertFloat32ToInt64) {
    typedef int64_t (FuncProtoInt64)(float);
    ConvertToFunc<FuncProtoInt64, float, int64_t> *ct_float32_int64 = new (c->mem()) ConvertToFunc<FuncProtoInt64,float,int64_t>(MEM_LOC(c->mem()), "ConvertFloat32ToInt64", c, false);
    ct_float32_int64->test(LOC, true, tFloat32, tInt64, (float)0, (int64_t)0);
    ct_float32_int64->test(LOC, false, tFloat32, tInt64, (float)3, (int64_t)3);
    ct_float32_int64->test(LOC, false, tFloat32, tInt64, (float)std::numeric_limits<int32_t>::min(), (int64_t)(float)std::numeric_limits<int32_t>::min());
    ct_float32_int64->test(LOC, false, tFloat32, tInt64, (float)std::numeric_limits<int32_t>::max(), (int64_t)(float)std::numeric_limits<int32_t>::max());
    delete ct_float32_int64;
}
TEST(omrgenExtension, ConvertFloat32ToFloat32) {
    typedef float (FuncProtoFloat32)(float);
    ConvertToFunc<FuncProtoFloat32, float, float> *ct_float32_float32 = new (c->mem()) ConvertToFunc<FuncProtoFloat32,float,float>(MEM_LOC(c->mem()), "ConvertFloat32ToFloat32", c, false);
    ct_float32_float32->test(LOC, true, tFloat32, tFloat32, (float)0, (float)0);
    ct_float32_float32->test(LOC, false, tFloat32, tFloat32, (float)3, (float)3);
    ct_float32_float32->test(LOC, false, tFloat32, tFloat32, (float)std::numeric_limits<float>::min(), (float)std::numeric_limits<float>::min());
    ct_float32_float32->test(LOC, false, tFloat32, tFloat32, (float)std::numeric_limits<float>::max(), (float)std::numeric_limits<float>::max());
    delete ct_float32_float32;
}
TEST(omrgenExtension, ConvertFloat32ToFloat64) {
    typedef double (FuncProtoFloat64)(float);
    ConvertToFunc<FuncProtoFloat64, float, double> *ct_float32_float64 = new (c->mem()) ConvertToFunc<FuncProtoFloat64,float,double>(MEM_LOC(c->mem()), "ConvertFloat32ToFloat64", c, false);
    ct_float32_float64->test(LOC, true, tFloat32, tFloat64, (float)0, (double)0);
    ct_float32_float64->test(LOC, false, tFloat32, tFloat64, (float)3, (double)3);
    ct_float32_float64->test(LOC, false, tFloat32, tFloat64, (float)std::numeric_limits<float>::min(), (double)std::numeric_limits<float>::min());
    ct_float32_float64->test(LOC, false, tFloat32, tFloat64, (float)std::numeric_limits<float>::max(), (double)std::numeric_limits<float>::max());
    delete ct_float32_float64;
}

// test converting Float64 to other primitive types
TEST(omrgenExtension, ConvertFloat64ToInt8) {
    typedef int8_t (FuncProtoInt8)(double);
    ConvertToFunc<FuncProtoInt8, double, int8_t> *ct_float64_int8 = new (c->mem()) ConvertToFunc<FuncProtoInt8,double,int8_t>(MEM_LOC(c->mem()), "ConvertFloat64ToInt8", c, false);
    ct_float64_int8->test(LOC, true, tFloat64, tInt8, (double)0, (int8_t)0);
    ct_float64_int8->test(LOC, false, tFloat64, tInt8, (double)3, (int8_t)3);
    ct_float64_int8->test(LOC, false, tFloat64, tInt8, (double)std::numeric_limits<int8_t>::min(), (int8_t) std::numeric_limits<int8_t>::min());
    ct_float64_int8->test(LOC, false, tFloat64, tInt8, (double)std::numeric_limits<int8_t>::max(), (int8_t) std::numeric_limits<int8_t>::max());
    delete ct_float64_int8;
}
TEST(omrgenExtension, ConvertFloat64ToInt16) {
    typedef int16_t (FuncProtoInt16)(double);
    ConvertToFunc<FuncProtoInt16, double, int16_t> *ct_float64_int16 = new (c->mem()) ConvertToFunc<FuncProtoInt16,double,int16_t>(MEM_LOC(c->mem()), "ConvertFloat64ToInt16", c, false);
    ct_float64_int16->test(LOC, true, tFloat64, tInt16, (double)0, (int16_t)0);
    ct_float64_int16->test(LOC, false, tFloat64, tInt16, (double)3, (int16_t)3);
    ct_float64_int16->test(LOC, false, tFloat64, tInt16, (double)std::numeric_limits<int16_t>::min(), (int16_t) std::numeric_limits<int16_t>::min());
    ct_float64_int16->test(LOC, false, tFloat64, tInt16, (double)std::numeric_limits<int16_t>::max(), (int16_t) std::numeric_limits<int16_t>::max());
    delete ct_float64_int16;
}
TEST(omrgenExtension, ConvertFloat64ToInt32) {
    typedef int32_t (FuncProtoInt32)(double);
    ConvertToFunc<FuncProtoInt32, double, int32_t> *ct_float64_int32 = new (c->mem()) ConvertToFunc<FuncProtoInt32,double,int32_t>(MEM_LOC(c->mem()), "ConvertFloat64ToInt32", c, false);
    ct_float64_int32->test(LOC, true, tFloat64, tInt32, (double)0, (int32_t)0);
    ct_float64_int32->test(LOC, false, tFloat64, tInt32, (double)3, (int32_t)3);
    ct_float64_int32->test(LOC, false, tFloat64, tInt32, (double)std::numeric_limits<int32_t>::min(), (int32_t)(float)std::numeric_limits<int32_t>::min());
    ct_float64_int32->test(LOC, false, tFloat64, tInt32, (double)std::numeric_limits<int32_t>::max(), (int32_t)(float)std::numeric_limits<int32_t>::max());
    delete ct_float64_int32;
}
TEST(omrgenExtension, ConvertFloat64ToInt64) {
    typedef int64_t (FuncProtoInt64)(double);
    ConvertToFunc<FuncProtoInt64, double, int64_t> *ct_float64_int64 = new (c->mem()) ConvertToFunc<FuncProtoInt64,double,int64_t>(MEM_LOC(c->mem()), "ConvertFloat64ToInt64", c, false);
    ct_float64_int64->test(LOC, true, tFloat64, tInt64, (double)0, (int64_t)0);
    ct_float64_int64->test(LOC, false, tFloat64, tInt64, (double)3, (int64_t)3);
    ct_float64_int64->test(LOC, false, tFloat64, tInt64, (double)std::numeric_limits<int64_t>::min(), (int64_t)(float)std::numeric_limits<int64_t>::min());
    ct_float64_int64->test(LOC, false, tFloat64, tInt64, (double)std::numeric_limits<int64_t>::max(), (int64_t)(float)std::numeric_limits<int64_t>::max());
    delete ct_float64_int64;
}
TEST(omrgenExtension, ConvertFloat64ToFloat32) {
    typedef float (FuncProtoFloat32)(double);
    ConvertToFunc<FuncProtoFloat32, double, float> *ct_float64_float32 = new (c->mem()) ConvertToFunc<FuncProtoFloat32,double,float>(MEM_LOC(c->mem()), "ConvertFloat64ToFloat32", c, false);
    ct_float64_float32->test(LOC, true, tFloat64, tFloat32, (double)0, (float)0);
    ct_float64_float32->test(LOC, false, tFloat64, tFloat32, (double)3, (float)3);
    ct_float64_float32->test(LOC, false, tFloat64, tFloat32, (double)std::numeric_limits<float>::min(), (float)std::numeric_limits<float>::min());
    ct_float64_float32->test(LOC, false, tFloat64, tFloat32, (double)std::numeric_limits<float>::max(), (float)std::numeric_limits<float>::max());
    delete ct_float64_float32;
}
TEST(omrgenExtension, ConvertFloat64ToFloat64) {
    typedef double (FuncProtoFloat64)(double);
    ConvertToFunc<FuncProtoFloat64, double, double> *ct_float64_float64 = new (c->mem()) ConvertToFunc<FuncProtoFloat64,double,double>(MEM_LOC(c->mem()), "ConvertFloat64ToFloat64", c, false);
    ct_float64_float64->test(LOC, true, tFloat64, tFloat64, (double)0, (double)0);
    ct_float64_float64->test(LOC, false, tFloat64, tFloat64, (double)3, (double)3);
    ct_float64_float64->test(LOC, false, tFloat64, tFloat64, (double)std::numeric_limits<double>::min(), (double)std::numeric_limits<double>::min());
    ct_float64_float64->test(LOC, false, tFloat64, tFloat64, (double)std::numeric_limits<double>::max(), (double)std::numeric_limits<double>::max());
    delete ct_float64_float64;
}

// Test returning the value of a paramater that has a pointer type using LoadAt
template<typename FuncPrototype, typename cType>
class ReturnPointerParameterFunc : public TestFunc {
public:
    ReturnPointerParameterFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _tid(NoTypeID)
        , _type(NULL)
        , _value(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        cType *pValue = &this->_value;
        EXPECT_EQ(f(pValue), _value) << "Compiled f(" << _value << ") returns " << _value;
    }
    void test(LOCATION, const TypeID tid, cType value) {
        _tid = tid;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _type = comp->ir()->typedict()->Lookup(_tid);
        ctx->DefineParameter("value", bx()->PointerTo(PASSLOC, _type));
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
    TypeID _tid;
    const Type *_type;
    cType _value;
};

TEST(omrgenExtension, ReturnPointerParam_pInt8) {
    typedef int8_t (FuncProto)(int8_t *);
    ReturnPointerParameterFunc<FuncProto, int8_t> *rpp_int8 = new (c->mem()) ReturnPointerParameterFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "ReturnPointerParam_pInt8", c, false);
    rpp_int8->test(LOC, tInt8, static_cast<int8_t>(0));
    rpp_int8->test(LOC, tInt8, static_cast<int8_t>(3));
    rpp_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    rpp_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    delete rpp_int8;
}
TEST(omrgenExtension, ReturnPointerParam_pInt16) {
    typedef int16_t (FuncProto)(int16_t *);
    ReturnPointerParameterFunc<FuncProto, int16_t> *rpp_int16 = new (c->mem()) ReturnPointerParameterFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "ReturnPointerParam_pInt16", c, false);
    rpp_int16->test(LOC, tInt16, static_cast<int16_t>(0));
    rpp_int16->test(LOC, tInt16, static_cast<int16_t>(3));
    rpp_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    rpp_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    delete rpp_int16;
}
TEST(omrgenExtension, ReturnPointerParam_pInt32) {
    typedef int32_t (FuncProto)(int32_t *);
    ReturnPointerParameterFunc<FuncProto, int32_t> *rpp_int32 = new (c->mem()) ReturnPointerParameterFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "ReturnPointerParam_pInt32", c, false);
    rpp_int32->test(LOC, tInt32, static_cast<int32_t>(0));
    rpp_int32->test(LOC, tInt32, static_cast<int32_t>(3));
    rpp_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    rpp_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    delete rpp_int32;
}
TEST(omrgenExtension, ReturnPointerParam_pInt64) {
    typedef int64_t (FuncProto)(int64_t *);
    ReturnPointerParameterFunc<FuncProto, int64_t> *rpp_int64 = new (c->mem()) ReturnPointerParameterFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "ReturnPointerParam_pInt64", c, false);
    rpp_int64->test(LOC, tInt64, static_cast<int64_t>(0));
    rpp_int64->test(LOC, tInt64, static_cast<int64_t>(3));
    rpp_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    rpp_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    delete rpp_int64;
}
TEST(omrgenExtension, ReturnPointerParam_pFloat32) {
    typedef float (FuncProto)(float *);
    ReturnPointerParameterFunc<FuncProto, float> *rpp_float32 = new (c->mem()) ReturnPointerParameterFunc<FuncProto,float>(MEM_LOC(c->mem()), "ReturnPointerParam_pFloat32", c, false);
    rpp_float32->test(LOC, tFloat32, static_cast<float>(0));
    rpp_float32->test(LOC, tFloat32, static_cast<float>(3));
    rpp_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    rpp_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    delete rpp_float32;
}
TEST(omrgenExtension, ReturnPointerParam_pFloat64) {
    typedef double (FuncProto)(double *);
    ReturnPointerParameterFunc<FuncProto, double> *rpp_float64 = new (c->mem()) ReturnPointerParameterFunc<FuncProto,double>(MEM_LOC(c->mem()), "ReturnPointerParam_pFloat64", c, false);
    rpp_float64->test(LOC, tFloat64, static_cast<double>(0));
    rpp_float64->test(LOC, tFloat64, static_cast<double>(3));
    rpp_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    rpp_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    delete rpp_float64;
}
TEST(omrgenExtension, ReturnPointerParam_pAddress) {
    typedef uintptr_t (FuncProto)(uintptr_t *);
    ReturnPointerParameterFunc<FuncProto, uintptr_t> *rpp_address = new (c->mem()) ReturnPointerParameterFunc<FuncProto,uintptr_t>(MEM_LOC(c->mem()), "ReturnPointerParam_pAddress", c, false);
    rpp_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    rpp_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    rpp_address->test(LOC, tAddress, reinterpret_cast<uintptr_t>(&rpp_address));
    delete rpp_address;
}

// Test returning the value of a paramater that has a pointer to pointer type using LoadAt
template<typename FuncPrototype, typename cType>
class ReturnPointerToPointerParameterFunc : public TestFunc {
public:
    ReturnPointerToPointerParameterFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _tid(NoTypeID)
        , _type(NULL)
        , _value(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        cType *pValue = &this->_value;
        EXPECT_EQ(f(&pValue), pValue) << "Compiled f(" << (intptr_t)(&pValue) << ") returns " << (intptr_t)(pValue);
        EXPECT_EQ((*pValue), _value) << "and (*" << (intptr_t)(pValue) << ") is still " << _value;
    }
    void test(LOCATION, const TypeID tid, cType value) {
        _tid = tid;
        _value = value;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _type = comp->ir()->typedict()->Lookup(_tid);
        const Type *pType = bx()->PointerTo(PASSLOC, _type);
        ctx->DefineParameter("value", bx()->PointerTo(PASSLOC, pType));
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
    TypeID _tid;
    const Type *_type;
    cType _value;
};

TEST(omrgenExtension, ReturnPointerToPointerParam_ppInt8) {
    typedef int8_t *(FuncProto)(int8_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, int8_t> *rpp_int8 = new (c->mem()) ReturnPointerToPointerParameterFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "ReturnPointerToPointerParam_ppInt8", c, false);
    rpp_int8->test(LOC, tInt8, static_cast<int8_t>(0));
    rpp_int8->test(LOC, tInt8, static_cast<int8_t>(3));
    rpp_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    rpp_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    delete rpp_int8;
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppInt16) {
    typedef int16_t *(FuncProto)(int16_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, int16_t> *rpp_int16 = new (c->mem()) ReturnPointerToPointerParameterFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "ReturnPointerToPointerParam_ppInt16", c, false);
    rpp_int16->test(LOC, tInt16, static_cast<int16_t>(0));
    rpp_int16->test(LOC, tInt16, static_cast<int16_t>(3));
    rpp_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    rpp_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    delete rpp_int16;
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppInt32) {
    typedef int32_t *(FuncProto)(int32_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, int32_t> *rpp_int32 = new (c->mem()) ReturnPointerToPointerParameterFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "ReturnPointerToPointerParam_ppInt32", c, false);
    rpp_int32->test(LOC, tInt32, static_cast<int32_t>(0));
    rpp_int32->test(LOC, tInt32, static_cast<int32_t>(3));
    rpp_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    rpp_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    delete rpp_int32;
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppInt64) {
    typedef int64_t *(FuncProto)(int64_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, int64_t> *rpp_int64 = new (c->mem()) ReturnPointerToPointerParameterFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "ReturnPointerToPointerParam_ppInt64", c, false);
    rpp_int64->test(LOC, tInt64, static_cast<int64_t>(0));
    rpp_int64->test(LOC, tInt64, static_cast<int64_t>(3));
    rpp_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    rpp_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    delete rpp_int64;
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppFloat32) {
    typedef float *(FuncProto)(float **);
    ReturnPointerToPointerParameterFunc<FuncProto, float> *rpp_float32 = new (c->mem()) ReturnPointerToPointerParameterFunc<FuncProto,float>(MEM_LOC(c->mem()), "ReturnPointerToPointerParam_ppFloat32", c, false);
    rpp_float32->test(LOC, tFloat32, static_cast<float>(0));
    rpp_float32->test(LOC, tFloat32, static_cast<float>(3));
    rpp_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    rpp_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    delete rpp_float32;
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppFloat64) {
    typedef double *(FuncProto)(double **);
    ReturnPointerToPointerParameterFunc<FuncProto, double> *rpp_float64 = new (c->mem()) ReturnPointerToPointerParameterFunc<FuncProto,double>(MEM_LOC(c->mem()), "ReturnPointerToPointerParam_ppFloat64", c, false);
    rpp_float64->test(LOC, tFloat64, static_cast<double>(0));
    rpp_float64->test(LOC, tFloat64, static_cast<double>(3));
    rpp_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    rpp_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    delete rpp_float64;
}
TEST(omrgenExtension, ReturnPointerToPointerParam_ppAddress) {
    typedef uintptr_t *(FuncProto)(uintptr_t **);
    ReturnPointerToPointerParameterFunc<FuncProto, uintptr_t> *rpp_address = new (c->mem()) ReturnPointerToPointerParameterFunc<FuncProto,uintptr_t>(MEM_LOC(c->mem()), "ReturnPointerToPointerParam_ppAddress", c, false);
    rpp_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    rpp_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min());
    rpp_address->test(LOC, tAddress, reinterpret_cast<uintptr_t>(&rpp_address));
    delete rpp_address;
}


// Tests for CreateLocalArray
template<uint32_t NUMVALUES, typename value_cType>
class CreateLocalArrayFunc : public TestFunc {
public:
    CreateLocalArrayFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _valueTid(NoTypeID)
        , _pValueType(NULL)
        , _valueType(NULL)
        , _resultValue(0) {
    }
    void run(LOCATION) {
        typedef value_cType (FuncPrototype)();
        FuncPrototype *f = this->body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(), _resultValue) << "Compiled f() returns " << _resultValue;
    }
    void test(LOCATION, const TypeID valueTid) {
        value_cType sum=0;
        for (int32_t i=0;i < NUMVALUES;i++) {
            sum += i;
        }
        _valueTid = valueTid;
        _resultValue = sum;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _valueType = comp->ir()->typedict()->Lookup(_valueTid);
        _pValueType = bx()->PointerTo(LOC, _valueType);
        ctx->DefineParameter("values", _pValueType);
        ctx->DefineReturnType(_valueType);
        ctx->DefineLocal("array", _pValueType);
        ctx->DefineLocal("sum", _valueType);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto valuesSym=ctx->LookupLocal("values"); 
        auto arraySym=ctx->LookupLocal("array"); 
        int32_t numElements = NUMVALUES;
        Value *neValue = bx()->ConstInt32(LOC, entry, numElements);
        Literal *lv = bx()->Int32(comp->ir())->literal(LOC, numElements);
        fx()->Store(LOC, entry, arraySym, bx()->CreateLocalArray(LOC, entry, lv, _pValueType));
        Value *arrayValue = fx()->Load(LOC, entry, arraySym);
        for (value_cType i=0;i < NUMVALUES;i++) {
            Literal *lv = _valueType->literal(LOC, reinterpret_cast<LiteralBytes *>(&i));
            Value *ci = this->bx()->Const(LOC, entry, lv);
            bx()->StoreAt(LOC, entry, bx()->IndexAt(LOC, entry, arrayValue, ci), ci);
        }

        auto sumSym=ctx->LookupLocal("sum"); 
        fx()->Store(LOC, entry, sumSym, bx()->Const(LOC, entry, _valueType->zero(LOC)));
        for (int32_t i=0;i < NUMVALUES;i++) {
            Value *ci = bx()->ConstInt32(LOC, entry, i);
            Value *v = bx()->LoadAt(LOC, entry, bx()->IndexAt(LOC, entry, arrayValue, ci));
            fx()->Store(LOC, entry, sumSym, bx()->Add(LOC, entry, fx()->Load(LOC, entry, sumSym), v));
        }
        fx()->Return(LOC, entry, fx()->Load(LOC, entry, sumSym));
        return true;
    }

protected:
    TypeID _valueTid;
    const Type *_valueType;
    const Base::PointerType *_pValueType;
    value_cType _resultValue;
};

TEST(omrgenExtension, CreateLocalStruct_int8) {
    CreateLocalArrayFunc<1, int8_t> *cla_1_int8 = new (c->mem()) CreateLocalArrayFunc<1, int8_t>(MEM_LOC(c->mem()), "cla_1_int8", c, false);
    cla_1_int8->test(LOC, tInt8);
    delete cla_1_int8;
    CreateLocalArrayFunc<13, int8_t> *cla_13_int8 = new (c->mem()) CreateLocalArrayFunc<13, int8_t>(MEM_LOC(c->mem()), "cla_13_int8", c, false);
    cla_13_int8->test(LOC, tInt8);
    delete cla_13_int8;
}
TEST(omrgenExtension, CreateLocalStruct_int16) {
    CreateLocalArrayFunc<1, int16_t> *cla_1_int16 = new (c->mem()) CreateLocalArrayFunc<1, int16_t>(MEM_LOC(c->mem()), "cla_1_int16", c, false);
    cla_1_int16->test(LOC, tInt16);
    delete cla_1_int16;
    CreateLocalArrayFunc<13, int16_t> *cla_13_int16 = new (c->mem()) CreateLocalArrayFunc<13, int16_t>(MEM_LOC(c->mem()), "cla_13_int16", c, false);
    cla_13_int16->test(LOC, tInt16);
    delete cla_13_int16;
}
TEST(omrgenExtension, CreateLocalStruct_int32) {
    CreateLocalArrayFunc<1, int32_t> *cla_1_int32 = new (c->mem()) CreateLocalArrayFunc<1, int32_t>(MEM_LOC(c->mem()), "cla_1_int32", c, false);
    cla_1_int32->test(LOC, tInt32);
    delete cla_1_int32;
    CreateLocalArrayFunc<13, int32_t> *cla_13_int32 = new (c->mem()) CreateLocalArrayFunc<13, int32_t>(MEM_LOC(c->mem()), "cla_13_int32", c, false);
    cla_13_int32->test(LOC, tInt32);
    delete cla_13_int32;
}
TEST(omrgenExtension, CreateLocalStruct_int64) {
    CreateLocalArrayFunc<1, int64_t> *cla_1_int64 = new (c->mem()) CreateLocalArrayFunc<1, int64_t>(MEM_LOC(c->mem()), "cla_1_int64", c, false);
    cla_1_int64->test(LOC, tInt64);
    delete cla_1_int64;
    CreateLocalArrayFunc<13, int64_t> *cla_13_int64 = new (c->mem()) CreateLocalArrayFunc<13, int64_t>(MEM_LOC(c->mem()), "cla_13_int64", c, false);
    cla_13_int64->test(LOC, tInt64);
    delete cla_13_int64;
}
TEST(omrgenExtension, CreateLocalStruct_float32) {
    CreateLocalArrayFunc<1, float> *cla_1_float32 = new (c->mem()) CreateLocalArrayFunc<1, float>(MEM_LOC(c->mem()), "cla_1_float32", c, false);
    cla_1_float32->test(LOC, tFloat32);
    delete cla_1_float32;
    CreateLocalArrayFunc<13, float> *cla_13_float32 = new (c->mem()) CreateLocalArrayFunc<13, float>(MEM_LOC(c->mem()), "cla_13_float32", c, false);
    cla_13_float32->test(LOC, tFloat32);
    delete cla_13_float32;
}
TEST(omrgenExtension, CreateLocalStruct_float64) {
    CreateLocalArrayFunc<1, double> *cla_1_float64 = new (c->mem()) CreateLocalArrayFunc<1, double>(MEM_LOC(c->mem()), "cla_1_float64", c, false);
    cla_1_float64->test(LOC, tFloat64);
    delete cla_1_float64;
    CreateLocalArrayFunc<13, double> *cla_13_float64 = new (c->mem()) CreateLocalArrayFunc<13, double>(MEM_LOC(c->mem()), "cla_13_float64", c, false);
    cla_13_float64->test(LOC, tFloat64);
    delete cla_13_float64;
}


// Base class for operating on two numbers together of the given types producing the given type
template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class BinaryOpFunc : public TestFunc {
public:
    BinaryOpFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _leftTid(NoTypeID)
        , _leftType(NULL)
        , _leftValue(0)
        , _rightTid(NoTypeID)
        , _rightType(NULL)
        , _rightValue(0)
        , _resultTid(NoTypeID)
        , _resultType(NULL)
        , _resultValue(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_leftValue,_rightValue), _resultValue) << "Compiled f(" << _leftValue << ", " << _rightValue << ") returns " << _resultValue;
    }
    void test(LOCATION, const TypeID leftTid, left_cType leftValue, const TypeID rightTid, right_cType rightValue, const TypeID resultTid, result_cType resultValue) {
        _leftTid = leftTid;
        _leftValue = leftValue;
        _rightTid = rightTid;
        _rightValue = rightValue;
        _resultTid = resultTid;
        _resultValue = resultValue;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _leftType = comp->ir()->typedict()->Lookup(_leftTid);
        _rightType = comp->ir()->typedict()->Lookup(_rightTid);
        _resultType = comp->ir()->typedict()->Lookup(_resultTid);
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

    TypeID _leftTid;
    const Type *_leftType;
    left_cType _leftValue;
    TypeID _rightTid;
    const Type *_rightType;
    right_cType _rightValue;
    TypeID _resultTid;
    const Type *_resultType;
    result_cType _resultValue;
};

// Test adding two numbers together of the given types
template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class AddFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    AddFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->Add(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, AddInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    AddFunc<FuncProto, int8_t, int8_t, int8_t> *add_int8s = new (c->mem()) AddFunc<FuncProto,int8_t,int8_t,int8_t>(MEM_LOC(c->mem()), "add_int8s", c, false);
    add_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    add_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(3));
    add_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(3));
    add_int8s->test(LOC, tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0));
    add_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(0));
    add_int8s->test(LOC, tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(-6));
    add_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::min());
    add_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::min(), tInt8, std::numeric_limits<int8_t>::min());
    add_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::max());
    add_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::max(), tInt8, std::numeric_limits<int8_t>::max());
    add_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(-1), tInt8, std::numeric_limits<int8_t>::max());
    add_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::min());
    delete add_int8s;
}
TEST(omrgenExtension, AddInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    AddFunc<FuncProto, int16_t, int16_t, int16_t> *add_int16s = new (c->mem()) AddFunc<FuncProto,int16_t,int16_t,int16_t>(MEM_LOC(c->mem()), "add_int16s", c, false);
    add_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    add_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(3));
    add_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(3));
    add_int16s->test(LOC, tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0));
    add_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(0));
    add_int16s->test(LOC, tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(-6));
    add_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::min());
    add_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::min(), tInt16, std::numeric_limits<int16_t>::min());
    add_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::max());
    add_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::max(), tInt16, std::numeric_limits<int16_t>::max());
    add_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(-1), tInt16, std::numeric_limits<int16_t>::max());
    add_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::min());
    delete add_int16s;
}
TEST(omrgenExtension, AddInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    AddFunc<FuncProto, int32_t, int32_t, int32_t> *add_int32s = new (c->mem()) AddFunc<FuncProto,int32_t,int32_t,int32_t>(MEM_LOC(c->mem()), "add_int32s", c, false);
    add_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    add_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(3));
    add_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(3));
    add_int32s->test(LOC, tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0));
    add_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(0));
    add_int32s->test(LOC, tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(-6));
    add_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::min());
    add_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::min(), tInt32, std::numeric_limits<int32_t>::min());
    add_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::max());
    add_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::max(), tInt32, std::numeric_limits<int32_t>::max());
    add_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(-1), tInt32, std::numeric_limits<int32_t>::max());
    add_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::min());
    delete add_int32s;
}
TEST(omrgenExtension, AddInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    AddFunc<FuncProto, int64_t, int64_t, int64_t> *add_int64s = new (c->mem()) AddFunc<FuncProto,int64_t,int64_t,int64_t>(MEM_LOC(c->mem()), "add_int64s", c, false);
    add_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    add_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(3));
    add_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(3));
    add_int64s->test(LOC, tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0));
    add_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(0));
    add_int64s->test(LOC, tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(-6));
    add_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::min());
    add_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::min(), tInt64, std::numeric_limits<int64_t>::min());
    add_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::max());
    add_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::max(), tInt64, std::numeric_limits<int64_t>::max());
    add_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(-1), tInt64, std::numeric_limits<int64_t>::max());
    add_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::min());
    delete add_int64s;
}
TEST(omrgenExtension, AddFloat32s) {
    typedef float (FuncProto)(float, float);
    AddFunc<FuncProto, float, float, float> *add_float32s = new (c->mem()) AddFunc<FuncProto,float,float,float>(MEM_LOC(c->mem()), "add_float32s", c, false);
    add_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0));
    add_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(3));
    add_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(3));
    add_float32s->test(LOC, tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(0));
    add_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(0));
    add_float32s->test(LOC, tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(-6));
    add_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::min());
    add_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::min(), tFloat32, std::numeric_limits<float>::min());
    add_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::max());
    add_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::max(), tFloat32, std::numeric_limits<float>::max());
    delete add_float32s;
}
TEST(omrgenExtension, AddFloat64s) {
    typedef double (FuncProto)(double, double);
    AddFunc<FuncProto, double, double, double> *add_float64s = new (c->mem()) AddFunc<FuncProto,double,double,double>(MEM_LOC(c->mem()), "add_float64s", c, false);
    add_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0));
    add_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(3));
    add_float64s->test(LOC, tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(3));
    add_float64s->test(LOC, tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(0));
    add_float64s->test(LOC, tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(0));
    add_float64s->test(LOC, tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(-6));
    add_float64s->test(LOC, tFloat64, std::numeric_limits<double>::min(), tFloat64, static_cast<double>(0), tFloat64, std::numeric_limits<double>::min());
    add_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, std::numeric_limits<double>::min(), tFloat64, std::numeric_limits<double>::min());
    add_float64s->test(LOC, tFloat64, std::numeric_limits<double>::max(), tFloat64, static_cast<double>(0), tFloat64, std::numeric_limits<double>::max());
    add_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, std::numeric_limits<double>::max(), tFloat64, std::numeric_limits<double>::max());
    delete add_float64s;
}
TEST(omrgenExtension, AddAddressAndInt) {
    if (c->platformWordSize() == 32) {
        typedef intptr_t (FuncProto)(intptr_t, int32_t);
        AddFunc<FuncProto, intptr_t, int32_t, intptr_t> *add_addressint32s = new (c->mem()) AddFunc<FuncProto,intptr_t,int32_t,intptr_t>(MEM_LOC(c->mem()), "add_addressint32s", c, false);
        add_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(NULL), tInt32, static_cast<int32_t>(0), tAddress, static_cast<intptr_t>(NULL));
        add_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(NULL), tInt32, static_cast<int32_t>(4), tAddress, static_cast<intptr_t>(4));
        add_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(0xdeadbeef), tInt32, static_cast<int32_t>(0), tAddress, static_cast<intptr_t>(0xdeadbeef));
        add_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(0xdeadbeef), tInt32, static_cast<int32_t>(16), tAddress, static_cast<intptr_t>(0xdeadbeef+16));
        add_addressint32s->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tInt32, static_cast<int32_t>(0), tAddress, std::numeric_limits<intptr_t>::max());
        add_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(0), tInt32, std::numeric_limits<int32_t>::max(), tAddress, static_cast<intptr_t>(std::numeric_limits<int32_t>::max()));
        add_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(0), tInt32, std::numeric_limits<int32_t>::min(), tAddress, static_cast<intptr_t>(std::numeric_limits<int32_t>::min()));
        delete add_addressint32s;
    } else if (c->platformWordSize() == 64) {
        typedef intptr_t (FuncProto)(intptr_t, int64_t);
        AddFunc<FuncProto, intptr_t, int64_t, intptr_t> *add_addressint64s = new (c->mem()) AddFunc<FuncProto,intptr_t,int64_t,intptr_t>(MEM_LOC(c->mem()), "add_addressint64s", c, false);
        add_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(NULL), tInt64, static_cast<int64_t>(0), tAddress, static_cast<intptr_t>(NULL));
        add_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(NULL), tInt64, static_cast<int64_t>(4), tAddress, static_cast<intptr_t>(4));
        add_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(0xdeadbeef), tInt64, static_cast<int64_t>(0), tAddress, static_cast<intptr_t>(0xdeadbeef));
        add_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(0xdeadbeef), tInt64, static_cast<int64_t>(16), tAddress, static_cast<intptr_t>(0xdeadbeef+16));
        add_addressint64s->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tInt64, static_cast<int64_t>(0), tAddress, std::numeric_limits<intptr_t>::max());
        add_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(0), tInt64, std::numeric_limits<int64_t>::max(), tAddress, static_cast<intptr_t>(std::numeric_limits<int64_t>::max()));
        add_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(0), tInt64, std::numeric_limits<int64_t>::min(), tAddress, static_cast<intptr_t>(std::numeric_limits<int64_t>::min()));
        delete add_addressint64s;
    }
}
TEST(omrgenExtension, AddIntAndAddress) {
    if (c->platformWordSize() == 32) {
        typedef intptr_t (FuncProto)(int32_t, intptr_t);
        AddFunc<FuncProto, int32_t, intptr_t, intptr_t> *add_int32addresses = new (c->mem()) AddFunc<FuncProto,int32_t,intptr_t,intptr_t>(MEM_LOC(c->mem()), "add_int32addresses", c, false);
        add_int32addresses->test(LOC, tInt32, static_cast<int32_t>(0), tAddress, static_cast<intptr_t>(NULL), tAddress, static_cast<intptr_t>(NULL));
        add_int32addresses->test(LOC, tInt32, static_cast<int32_t>(4), tAddress, static_cast<intptr_t>(NULL), tAddress, static_cast<intptr_t>(4));
        add_int32addresses->test(LOC, tInt32, static_cast<int32_t>(0), tAddress, static_cast<intptr_t>(0xdeadbeef), tAddress, static_cast<intptr_t>(0xdeadbeef));
        add_int32addresses->test(LOC, tInt32, static_cast<int32_t>(16), tAddress, static_cast<intptr_t>(0xdeadbeef), tAddress, static_cast<intptr_t>(0xdeadbeef+16));
        add_int32addresses->test(LOC, tInt32, static_cast<int32_t>(0), tAddress, std::numeric_limits<intptr_t>::max(), tAddress, std::numeric_limits<intptr_t>::max());
        add_int32addresses->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tAddress, static_cast<intptr_t>(0), tAddress, static_cast<intptr_t>(std::numeric_limits<int32_t>::max()));
        add_int32addresses->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tAddress, static_cast<intptr_t>(0), tAddress, static_cast<intptr_t>(std::numeric_limits<int32_t>::min()));
        delete add_int32addresses;
    } else if (c->platformWordSize() == 64) {
        typedef intptr_t (FuncProto)(intptr_t, int64_t);
        AddFunc<FuncProto, int64_t, intptr_t, intptr_t> *add_int64addresses = new (c->mem()) AddFunc<FuncProto,int64_t,intptr_t,intptr_t>(MEM_LOC(c->mem()), "add_int64addresses", c, false);
        add_int64addresses->test(LOC, tInt64, static_cast<int64_t>(0), tAddress, static_cast<intptr_t>(NULL), tAddress, static_cast<intptr_t>(NULL));
        add_int64addresses->test(LOC, tInt64, static_cast<int64_t>(4), tAddress, static_cast<intptr_t>(NULL), tAddress, static_cast<intptr_t>(4));
        add_int64addresses->test(LOC, tInt64, static_cast<int64_t>(0), tAddress, static_cast<intptr_t>(0xdeadbeef), tAddress, static_cast<intptr_t>(0xdeadbeef));
        add_int64addresses->test(LOC, tInt64, static_cast<int64_t>(16), tAddress, static_cast<intptr_t>(0xdeadbeef), tAddress, static_cast<intptr_t>(0xdeadbeef+16));
        add_int64addresses->test(LOC, tInt64, static_cast<int64_t>(0), tAddress, std::numeric_limits<intptr_t>::max(), tAddress, std::numeric_limits<intptr_t>::max());
        add_int64addresses->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tAddress, static_cast<intptr_t>(0), tAddress, static_cast<intptr_t>(std::numeric_limits<int64_t>::max()));
        add_int64addresses->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tAddress, static_cast<intptr_t>(0), tAddress, static_cast<intptr_t>(std::numeric_limits<int64_t>::min()));
        delete add_int64addresses;
    }
}

// Test anding two numbers together of the given types
template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class AndFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    AndFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->And(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, AndInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    AndFunc<FuncProto, int8_t, int8_t, int8_t> *and_int8s = new (c->mem()) AndFunc<FuncProto,int8_t,int8_t,int8_t>(MEM_LOC(c->mem()), "and_int8s", c, false);
    and_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    and_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0));
    and_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    and_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(3));
    and_int8s->test(LOC, tInt8, static_cast<int8_t>(12), tInt8, static_cast<int8_t>(4), tInt8, static_cast<int8_t>(4));
    and_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    and_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(0));
    and_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, std::numeric_limits<int8_t>::min(), tInt8, std::numeric_limits<int8_t>::min());
    and_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    and_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(0));
    and_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, std::numeric_limits<int8_t>::max(), tInt8, std::numeric_limits<int8_t>::max());
    delete and_int8s;
}
TEST(omrgenExtension, AndInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    AndFunc<FuncProto, int16_t, int16_t, int16_t> *and_int16s = new (c->mem()) AndFunc<FuncProto,int16_t,int16_t,int16_t>(MEM_LOC(c->mem()), "and_int16s", c, false);
    and_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    and_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0));
    and_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    and_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(3));
    and_int16s->test(LOC, tInt16, static_cast<int16_t>(12), tInt16, static_cast<int16_t>(4), tInt16, static_cast<int16_t>(4));
    and_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    and_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(0));
    and_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, std::numeric_limits<int16_t>::min(), tInt16, std::numeric_limits<int16_t>::min());
    and_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    and_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(0));
    and_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, std::numeric_limits<int16_t>::max(), tInt16, std::numeric_limits<int16_t>::max());
    delete and_int16s;
}
TEST(omrgenExtension, AndInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    AndFunc<FuncProto, int32_t, int32_t, int32_t> *and_int32s = new (c->mem()) AndFunc<FuncProto,int32_t,int32_t,int32_t>(MEM_LOC(c->mem()), "and_int32s", c, false);
    and_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    and_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0));
    and_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    and_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(3));
    and_int32s->test(LOC, tInt32, static_cast<int32_t>(12), tInt32, static_cast<int32_t>(4), tInt32, static_cast<int32_t>(4));
    and_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    and_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0));
    and_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, std::numeric_limits<int32_t>::min(), tInt32, std::numeric_limits<int32_t>::min());
    and_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    and_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0));
    and_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, std::numeric_limits<int32_t>::max(), tInt32, std::numeric_limits<int32_t>::max());
    delete and_int32s;
}
TEST(omrgenExtension, AndInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    AndFunc<FuncProto, int64_t, int64_t, int64_t> *and_int64s = new (c->mem()) AndFunc<FuncProto,int64_t,int64_t,int64_t>(MEM_LOC(c->mem()), "and_int64s", c, false);
    and_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    and_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0));
    and_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    and_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(3));
    and_int64s->test(LOC, tInt64, static_cast<int64_t>(12), tInt64, static_cast<int64_t>(4), tInt64, static_cast<int64_t>(4));
    and_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    and_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(0));
    and_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, std::numeric_limits<int64_t>::min(), tInt64, std::numeric_limits<int64_t>::min());
    and_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    and_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(0));
    and_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, std::numeric_limits<int64_t>::max(), tInt64, std::numeric_limits<int64_t>::max());
    delete and_int64s;
}


// Test for Calls
template<typename FuncPrototype, typename arg_cType>
class CallVoid1ParmTestFunc : public TestFunc {
public:
    CallVoid1ParmTestFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _argTid(NoTypeID)
        , _argType(NULL)
        , _argValue(0)
        , _sentinelValue(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        clearSentinel();
        f(_argValue);
        arg_cType v = loadSentinel();
        EXPECT_EQ(v, _argValue) << "Compiled f(" << _argValue << ") sets sentinel to " << v;
    }
    void test(LOCATION, const TypeID argTid, arg_cType argValue) {
        _argTid = argTid;
        _argValue = argValue;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _argType = comp->ir()->typedict()->Lookup(_argTid);
        ctx->DefineParameter("value", _argType);
        ctx->DefineReturnType(comp->ir()->NoType);
        defineFunction(comp, ctx);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto parmSym=ctx->LookupLocal("value"); 
        Value *value = fx()->Load(LOC, entry, parmSym);
        callFunction(entry, value);
        fx()->Return(LOC, entry);
        return true;
    }

protected:
    virtual void clearSentinel() = 0;
    virtual arg_cType loadSentinel() = 0;
    virtual void defineFunction(Func::FunctionCompilation *comp, Func::FunctionContext *ctx) = 0;
    virtual void callFunction(Builder *b, Value *v) = 0;

    TypeID _argTid;
    const Type *_argType;
    arg_cType _argValue;
    int8_t *_sentinelValue;
};


template <typename FuncPrototype, typename arg_cType>
class CallVoid1Parm : public CallVoid1ParmTestFunc<FuncPrototype, arg_cType> {
private:
    static arg_cType sentinel;
#define TARGET_LINE LINETOSTR(__LINE__)
    static void target(arg_cType x) {
        sentinel= x;
    }
public:
    CallVoid1Parm(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : CallVoid1ParmTestFunc<FuncPrototype, arg_cType>(MEM_PASSLOC(a), name, compiler, log) {
    }
protected:
    virtual void clearSentinel() { sentinel = 0; }
    virtual arg_cType loadSentinel() { return sentinel;  }
    virtual void defineFunction(Func::FunctionCompilation *comp, Func::FunctionContext *ctx) { 
        _fsym = ctx->DefineFunction(LOC, comp, "target", __FILE__, TARGET_LINE, (void*)&target, comp->ir()->NoType, 1, this->_argType);
    }
    virtual void callFunction(Builder *b, Value *v) { 
        fx->Call(LOC, b, _fsym, v);
    }
    Func::FunctionSymbol *_fsym;
};

typedef void *(CallVoid1Parm_FuncProto_int8_t)(int8_t);
template<> int8_t CallVoid1Parm<CallVoid1Parm_FuncProto_int8_t, int8_t>::sentinel = 0;
TEST(omrgenExtension, Call_void_int8) {
    CallVoid1Parm<CallVoid1Parm_FuncProto_int8_t, int8_t> *call_void_int8 = new (c->mem()) CallVoid1Parm<CallVoid1Parm_FuncProto_int8_t,int8_t>(MEM_LOC(c->mem()), "call_void_int8", c, false);
    call_void_int8->test(LOC, tInt8, static_cast<int8_t>(3));
    call_void_int8->test(LOC, tInt8, static_cast<int8_t>(-1));
    call_void_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min());
    call_void_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max());
    delete call_void_int8;
}
typedef void *(CallVoid1Parm_FuncProto_int16_t)(int16_t);
template<> int16_t CallVoid1Parm<CallVoid1Parm_FuncProto_int16_t, int16_t>::sentinel = 0;
TEST(omrgenExtension, Call_void_int16) {
    CallVoid1Parm<CallVoid1Parm_FuncProto_int16_t, int16_t> *call_void_int16 = new (c->mem()) CallVoid1Parm<CallVoid1Parm_FuncProto_int16_t,int16_t>(MEM_LOC(c->mem()), "call_void_int16", c, false);
    call_void_int16->test(LOC, tInt16, static_cast<int16_t>(3));
    call_void_int16->test(LOC, tInt16, static_cast<int16_t>(-1));
    call_void_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min());
    call_void_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max());
    delete call_void_int16;
}
typedef void *(CallVoid1Parm_FuncProto_int32_t)(int32_t);
template<> int32_t CallVoid1Parm<CallVoid1Parm_FuncProto_int32_t, int32_t>::sentinel = 0;
TEST(omrgenExtension, Call_void_int32) {
    CallVoid1Parm<CallVoid1Parm_FuncProto_int32_t, int32_t> *call_void_int32 = new (c->mem()) CallVoid1Parm<CallVoid1Parm_FuncProto_int32_t,int32_t>(MEM_LOC(c->mem()), "call_void_int32", c, false);
    call_void_int32->test(LOC, tInt32, static_cast<int32_t>(3));
    call_void_int32->test(LOC, tInt32, static_cast<int32_t>(-1));
    call_void_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min());
    call_void_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max());
    delete call_void_int32;
}
typedef void *(CallVoid1Parm_FuncProto_int64_t)(int64_t);
template<> int64_t CallVoid1Parm<CallVoid1Parm_FuncProto_int64_t, int64_t>::sentinel = 0;
TEST(omrgenExtension, Call_void_int64) {
    CallVoid1Parm<CallVoid1Parm_FuncProto_int64_t, int64_t> *call_void_int64 = new (c->mem()) CallVoid1Parm<CallVoid1Parm_FuncProto_int64_t,int64_t>(MEM_LOC(c->mem()), "call_void_int64", c, false);
    call_void_int64->test(LOC, tInt64, static_cast<int64_t>(3));
    call_void_int64->test(LOC, tInt64, static_cast<int64_t>(-1));
    call_void_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min());
    call_void_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max());
    delete call_void_int64;
}
typedef void *(CallVoid1Parm_FuncProto_float)(float);
template<> float CallVoid1Parm<CallVoid1Parm_FuncProto_float, float>::sentinel = 0;
TEST(omrgenExtension, Call_void_float32) {
    CallVoid1Parm<CallVoid1Parm_FuncProto_float, float> *call_void_float32 = new (c->mem()) CallVoid1Parm<CallVoid1Parm_FuncProto_float,float>(MEM_LOC(c->mem()), "call_void_float32", c, false);
    call_void_float32->test(LOC, tFloat32, static_cast<float>(3));
    call_void_float32->test(LOC, tFloat32, static_cast<float>(-1));
    call_void_float32->test(LOC, tFloat32, std::numeric_limits<float>::min());
    call_void_float32->test(LOC, tFloat32, std::numeric_limits<float>::max());
    delete call_void_float32;
}
typedef void *(CallVoid1Parm_FuncProto_double)(double);
template<> double CallVoid1Parm<CallVoid1Parm_FuncProto_double, double>::sentinel = 0;
TEST(omrgenExtension, Call_void_float64) {
    CallVoid1Parm<CallVoid1Parm_FuncProto_double, double> *call_void_float64 = new (c->mem()) CallVoid1Parm<CallVoid1Parm_FuncProto_double,double>(MEM_LOC(c->mem()), "call_void_float64", c, false);
    call_void_float64->test(LOC, tFloat64, static_cast<double>(3));
    call_void_float64->test(LOC, tFloat64, static_cast<double>(-1));
    call_void_float64->test(LOC, tFloat64, std::numeric_limits<double>::min());
    call_void_float64->test(LOC, tFloat64, std::numeric_limits<double>::max());
    delete call_void_float64;
}
typedef void *(CallVoid1Parm_FuncProto_pvoid)(void *);
template<> void * CallVoid1Parm<CallVoid1Parm_FuncProto_pvoid, void *>::sentinel = 0;
TEST(omrgenExtension, Call_void_address) {
    CallVoid1Parm<CallVoid1Parm_FuncProto_pvoid, void *> *call_void_address = new (c->mem()) CallVoid1Parm<CallVoid1Parm_FuncProto_pvoid,void *>(MEM_LOC(c->mem()), "call_void_address", c, false);
    call_void_address->test(LOC, tAddress, reinterpret_cast<void *>(0xdeadf00d));
    call_void_address->test(LOC, tAddress, reinterpret_cast<void *>(0xcafebeef));
    call_void_address->test(LOC, tAddress, std::numeric_limits<void *>::min());
    call_void_address->test(LOC, tAddress, std::numeric_limits<void *>::max());
    delete call_void_address;
}

// Test for function pointers
typedef bool (*fpType_bool_bool)(bool);
template <typename FuncPrototype>
class CallVoid1_fp: public CallVoid1ParmTestFunc<FuncPrototype, fpType_bool_bool> {
private:
    static fpType_bool_bool sentinel;
#define TARGET_LINE LINETOSTR(__LINE__)
    static void target(fpType_bool_bool x) {
        if (((*x)(true) == false) &&
            ((*x)(false) == true))
            sentinel = x;
    }
public:
    CallVoid1_fp(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : CallVoid1ParmTestFunc<FuncPrototype, fpType_bool_bool>(MEM_PASSLOC(a), name, compiler, log) {
    }
protected:
    virtual void clearSentinel() { sentinel = NULL;}
    virtual fpType_bool_bool loadSentinel() { return sentinel; }
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Func::FunctionTypeBuilder ftb(comp->ir());
        const Type *Int8 = this->bx()->Int8(comp->ir());
        ftb.setReturnType(Int8)
           .addParameterType(Int8);
        this->_argType = ftb.create(fx, comp);
        ctx->DefineParameter("value", this->_argType);
        ctx->DefineReturnType(comp->ir()->NoType);
        defineFunction(comp, ctx);
        return true;
    }
    virtual void defineFunction(Func::FunctionCompilation *comp, Func::FunctionContext *ctx) { 
        _fsym = ctx->DefineFunction(LOC, comp, "target", __FILE__, TARGET_LINE, (void*)&target, comp->ir()->NoType, 1, this->_argType);
    }
    virtual void callFunction(Builder *b, Value *v) { 
        fx->Call(LOC, b, _fsym, v);
    }
    Func::FunctionSymbol *_fsym;
};

static bool booleanNot(bool x) { return !x; }
static bool conditionalNot(bool x) { if (!x) { return true; } return false; }
typedef void *(CallVoid1_fp_FuncProto)(fpType_bool_bool);
template<> fpType_bool_bool CallVoid1_fp<CallVoid1_fp_FuncProto>::sentinel = 0;
TEST(omrgenExtension, Call_fp_bool_bool) {
    CallVoid1_fp<CallVoid1_fp_FuncProto> *call_void_fp = new (c->mem()) CallVoid1_fp<CallVoid1_fp_FuncProto>(MEM_LOC(c->mem()), "call_void_fp", c, false);
    call_void_fp->test(LOC, tAddress, &booleanNot); // argType will be ignored
    call_void_fp->test(LOC, tAddress, &conditionalNot); // argType will be ignored
    delete call_void_fp;
}

template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class DivFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    DivFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->Div(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, DivInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    DivFunc<FuncProto, int8_t, int8_t, int8_t> *div_int8s = new (c->mem()) DivFunc<FuncProto,int8_t,int8_t,int8_t>(MEM_LOC(c->mem()), "div_int8s", c, false);
    //div_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0));
    //div_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(1));
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(-1));
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(-1));
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(1));
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(9), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(3));
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(-9), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(-3));
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(9), tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(-3));
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(-9), tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(3));
    div_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::min());
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(0));
    div_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::max());
    div_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(0));
    delete div_int8s;
}
TEST(omrgenExtension, DivInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    DivFunc<FuncProto, int16_t, int16_t, int16_t> *div_int16s = new (c->mem()) DivFunc<FuncProto,int16_t,int16_t,int16_t>(MEM_LOC(c->mem()), "div_int16s", c, false);
    //div_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0));
    //div_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(1));
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(-1));
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(-1));
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(1));
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(9), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(3));
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(-9), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(-3));
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(9), tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(-3));
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(-9), tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(3));
    div_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::min());
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(0));
    div_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::max());
    div_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(0));
    delete div_int16s;
}
TEST(omrgenExtension, DivInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    DivFunc<FuncProto, int32_t, int32_t, int32_t> *div_int32s = new (c->mem()) DivFunc<FuncProto,int32_t,int32_t,int32_t>(MEM_LOC(c->mem()), "div_int32s", c, false);
    //div_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0));
    //div_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(1));
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(-1));
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(-1));
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(1));
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(9), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(3));
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(-9), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(-3));
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(9), tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(-3));
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(-9), tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(3));
    div_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::min());
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0));
    div_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::max());
    div_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0));
    delete div_int32s;
}
TEST(omrgenExtension, DivInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    DivFunc<FuncProto, int64_t, int64_t, int64_t> *div_int64s = new (c->mem()) DivFunc<FuncProto,int64_t,int64_t,int64_t>(MEM_LOC(c->mem()), "div_int64s", c, false);
    //div_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0));
    //div_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(1));
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(-1));
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(-1));
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(1));
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(9), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(3));
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(-9), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(-3));
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(9), tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(-3));
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(-9), tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(3));
    div_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::min());
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(0));
    div_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::max());
    div_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(0));
    delete div_int64s;
}
TEST(omrgenExtension, DivFloat32s) {
    typedef float (FuncProto)(float, float);
    DivFunc<FuncProto, float, float, float> *div_float32s = new (c->mem()) DivFunc<FuncProto,float,float,float>(MEM_LOC(c->mem()), "div_float32s", c, false);
    //div_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0));
    div_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(0));
    //div_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0));
    div_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(1));
    div_float32s->test(LOC, tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(-1));
    div_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(-1));
    div_float32s->test(LOC, tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(1));
    div_float32s->test(LOC, tFloat32, static_cast<float>(9), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(3));
    div_float32s->test(LOC, tFloat32, static_cast<float>(-9), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(-3));
    div_float32s->test(LOC, tFloat32, static_cast<float>(9), tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(-3));
    div_float32s->test(LOC, tFloat32, static_cast<float>(-9), tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(3));
    div_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, static_cast<float>(1), tFloat32, std::numeric_limits<float>::min());
    div_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, std::numeric_limits<float>::min(), tFloat32, static_cast<float>(1/std::numeric_limits<float>::min()));
    div_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, static_cast<float>(1), tFloat32, std::numeric_limits<float>::max());
    div_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, std::numeric_limits<float>::max(), tFloat32, static_cast<float>(1/std::numeric_limits<float>::max()));
    delete div_float32s;
}
TEST(omrgenExtension, DivFloat64s) {
    typedef double (FuncProto)(double, double);
    DivFunc<FuncProto, double, double, double> *div_float64s = new (c->mem()) DivFunc<FuncProto,double,double,double>(MEM_LOC(c->mem()), "div_float64s", c, false);
    //div_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0));
    div_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(0));
    //div_float64s->test(LOC, tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0));
    div_float64s->test(LOC, tFloat64, static_cast<double>(1), tFloat64, static_cast<double>(1), tFloat64, static_cast<double>(1));
    div_float64s->test(LOC, tFloat64, static_cast<double>(-1), tFloat64, static_cast<double>(1), tFloat64, static_cast<double>(-1));
    div_float64s->test(LOC, tFloat64, static_cast<double>(1), tFloat64, static_cast<double>(-1), tFloat64, static_cast<double>(-1));
    div_float64s->test(LOC, tFloat64, static_cast<double>(-1), tFloat64, static_cast<double>(-1), tFloat64, static_cast<double>(1));
    div_float64s->test(LOC, tFloat64, static_cast<double>(9), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(3));
    div_float64s->test(LOC, tFloat64, static_cast<double>(-9), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(-3));
    div_float64s->test(LOC, tFloat64, static_cast<double>(9), tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(-3));
    div_float64s->test(LOC, tFloat64, static_cast<double>(-9), tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(3));
    div_float64s->test(LOC, tFloat64, std::numeric_limits<double>::min(), tFloat64, static_cast<double>(1), tFloat64, std::numeric_limits<double>::min());
    div_float64s->test(LOC, tFloat64, static_cast<double>(1), tFloat64, std::numeric_limits<double>::min(), tFloat64, static_cast<double>(1/std::numeric_limits<double>::min()));
    div_float64s->test(LOC, tFloat64, std::numeric_limits<double>::max(), tFloat64, static_cast<double>(1), tFloat64, std::numeric_limits<double>::max());
    div_float64s->test(LOC, tFloat64, static_cast<double>(1), tFloat64, std::numeric_limits<double>::max(), tFloat64, static_cast<double>(1/std::numeric_limits<double>::max()));
    delete div_float64s;
}


// Test EqualTo with two numbers together of the given types
template<typename FuncPrototype, typename left_cType, typename right_cType>
class EqualToFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, int32_t> {
public:
    EqualToFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, int32_t>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->EqualTo(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, EqualToInt8s) {
    typedef int32_t (FuncProto)(int8_t, int8_t);
    EqualToFunc<FuncProto, int8_t, int8_t> *equalto_int8s = new (c->mem()) EqualToFunc<FuncProto,int8_t,int8_t>(MEM_LOC(c->mem()), "equalto_int8s", c, false);
    equalto_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0), tInt32, static_cast<int32_t>(1));
    equalto_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(3), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(3), tInt32, static_cast<int32_t>(1));
    equalto_int8s->test(LOC, tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(1), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(-1), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(-1), tInt32, static_cast<int32_t>(1));
    equalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, std::numeric_limits<int8_t>::min(), tInt32, static_cast<int32_t>(1));
    equalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, std::numeric_limits<int8_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, std::numeric_limits<int8_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, std::numeric_limits<int8_t>::max(), tInt32, static_cast<int32_t>(1));
    delete equalto_int8s;
}
TEST(omrgenExtension, EqualToInt16s) {
    typedef int32_t (FuncProto)(int16_t, int16_t);
    EqualToFunc<FuncProto, int16_t, int16_t> *equalto_int16s = new (c->mem()) EqualToFunc<FuncProto,int16_t,int16_t>(MEM_LOC(c->mem()), "equalto_int16s", c, false);
    equalto_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0), tInt32, static_cast<int32_t>(1));
    equalto_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(3), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(3), tInt32, static_cast<int32_t>(1));
    equalto_int16s->test(LOC, tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(1), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(-1), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(-1), tInt32, static_cast<int32_t>(1));
    equalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, std::numeric_limits<int16_t>::min(), tInt32, static_cast<int32_t>(1));
    equalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, std::numeric_limits<int16_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, std::numeric_limits<int16_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, std::numeric_limits<int16_t>::max(), tInt32, static_cast<int32_t>(1));
    delete equalto_int16s;
}
TEST(omrgenExtension, EqualToInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    EqualToFunc<FuncProto, int32_t, int32_t> *equalto_int32s = new (c->mem()) EqualToFunc<FuncProto,int32_t,int32_t>(MEM_LOC(c->mem()), "equalto_int32s", c, false);
    equalto_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(1));
    equalto_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(1));
    equalto_int32s->test(LOC, tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(1));
    equalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(1));
    equalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(1));
    delete equalto_int32s;
}
TEST(omrgenExtension, EqualToInt64s) {
    typedef int32_t (FuncProto)(int64_t, int64_t);
    EqualToFunc<FuncProto, int64_t, int64_t> *equalto_int64s = new (c->mem()) EqualToFunc<FuncProto,int64_t,int64_t>(MEM_LOC(c->mem()), "equalto_int64s", c, false);
    equalto_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0), tInt32, static_cast<int32_t>(1));
    equalto_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(3), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(3), tInt32, static_cast<int32_t>(1));
    equalto_int64s->test(LOC, tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(1), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(-1), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(-1), tInt32, static_cast<int32_t>(1));
    equalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, std::numeric_limits<int64_t>::min(), tInt32, static_cast<int32_t>(1));
    equalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, std::numeric_limits<int64_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, std::numeric_limits<int64_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, std::numeric_limits<int64_t>::max(), tInt32, static_cast<int32_t>(1));
    delete equalto_int64s;
}
TEST(omrgenExtension, EqualToFloat32s) {
    typedef int32_t (FuncProto)(float, float);
    EqualToFunc<FuncProto, float, float> *equalto_float32s = new (c->mem()) EqualToFunc<FuncProto,float,float>(MEM_LOC(c->mem()), "equalto_float32s", c, false);
    equalto_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0), tInt32, static_cast<int32_t>(1));
    equalto_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(3), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(0), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(3), tInt32, static_cast<int32_t>(1));
    equalto_float32s->test(LOC, tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(1), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(-1), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(-1), tInt32, static_cast<int32_t>(1));
    equalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, static_cast<float>(0), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(1));
    equalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, static_cast<float>(0), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(0));
    equalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(1));
    delete equalto_float32s;
}
TEST(omrgenExtension, EqualToFloat64s) {
    typedef int32_t (FuncProto)(double, double);
    EqualToFunc<FuncProto, double, double> *equalto_float64s = new (c->mem()) EqualToFunc<FuncProto,double,double>(MEM_LOC(c->mem()), "equalto_float64s", c, false);
    equalto_float64s->test(LOC, tFloat64, static_cast<float>(0), tFloat64, static_cast<float>(0), tInt32, static_cast<int32_t>(1));
    equalto_float64s->test(LOC, tFloat64, static_cast<float>(0), tFloat64, static_cast<float>(3), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, static_cast<float>(3), tFloat64, static_cast<float>(0), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, static_cast<float>(3), tFloat64, static_cast<float>(3), tInt32, static_cast<int32_t>(1));
    equalto_float64s->test(LOC, tFloat64, static_cast<float>(-1), tFloat64, static_cast<float>(1), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, static_cast<float>(1), tFloat64, static_cast<float>(-1), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, static_cast<float>(-1), tFloat64, static_cast<float>(-1), tInt32, static_cast<int32_t>(1));
    equalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::min(), tFloat64, static_cast<float>(0), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, static_cast<float>(0), tFloat64, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::min(), tFloat64, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(1));
    equalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::min(), tFloat64, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::max(), tFloat64, static_cast<float>(0), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, static_cast<float>(0), tFloat64, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::max(), tFloat64, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(0));
    equalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::max(), tFloat64, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(1));
    delete equalto_float64s;
}
TEST(omrgenExtension, EqualToAddresses) {
    typedef int32_t (FuncProto)(intptr_t, intptr_t);
    EqualToFunc<FuncProto, intptr_t, intptr_t> *equalto_addresses = new (c->mem()) EqualToFunc<FuncProto,intptr_t,intptr_t>(MEM_LOC(c->mem()), "equalto_addresses", c, false);
    equalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(0), tAddress, static_cast<intptr_t>(0), tInt32, static_cast<int32_t>(1));
    equalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(0), tAddress, static_cast<intptr_t>(3), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(3), tAddress, static_cast<intptr_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(3), tAddress, static_cast<intptr_t>(3), tInt32, static_cast<int32_t>(1));
    equalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(-1), tAddress, static_cast<intptr_t>(1), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(1), tAddress, static_cast<intptr_t>(-1), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(-1), tAddress, static_cast<intptr_t>(-1), tInt32, static_cast<int32_t>(1));
    equalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), tAddress, static_cast<intptr_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(0), tAddress, std::numeric_limits<intptr_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), tAddress, std::numeric_limits<intptr_t>::min(), tInt32, static_cast<int32_t>(1));
    equalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), tAddress, std::numeric_limits<intptr_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tAddress, static_cast<intptr_t>(0), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(0), tAddress, std::numeric_limits<intptr_t>::max(), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tAddress, std::numeric_limits<intptr_t>::min(), tInt32, static_cast<int32_t>(0));
    equalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tAddress, std::numeric_limits<intptr_t>::max(), tInt32, static_cast<int32_t>(1));
    delete equalto_addresses;
}


// Tests for LoadFieldAt
template<typename struct_cType, typename field_cType>
class LoadFieldAtTestFunc : public TestFunc {
public:
    typedef field_cType (FuncPrototype)(struct_cType *);
    LoadFieldAtTestFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _ftid(NoTypeID)
        , _ft(NULL)
        , _structType(NULL)
        , _fieldType(NULL)
        , _fieldValue(0) {
    }
    virtual void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        struct_cType _struct;
        _struct._value = _fieldValue;
        field_cType rv = f(&_struct);
        EXPECT_EQ(rv, _fieldValue) << "Compiled f(struct *) returns _value as " << rv << " (expected " << _fieldValue << ")";
    }
    void test(LOCATION, bool doCompile, const TypeID ftid, field_cType fieldValue) {
        _ftid = ftid;
        _fieldValue = fieldValue;
        if (doCompile)
            compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _ft = comp->ir()->typedict()->Lookup(_ftid);
        _structType = defineStruct(comp, _ft);
        _fieldType = _structType->LookupField("_value");
        assert(_fieldType != NULL);
        ctx->DefineParameter("theStruct", this->bx()->PointerTo(PASSLOC, _structType));
        ctx->DefineReturnType(_fieldType);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto structSym=ctx->LookupLocal("theStruct"); 
        Value *structBase = fx()->Load(LOC, entry, structSym);
        Value *value = bx()->LoadFieldAt(LOC, entry, _fieldType, structBase);
        fx()->Return(LOC, entry, value);
        return true;
    }

protected:
    virtual const Base::StructType * defineStruct(Func::FunctionCompilation *comp, const Type *ft) = 0;

    TypeID _ftid;
    const Type * _ft;
    const Base::StructType *_structType;
    const Base::FieldType *_fieldType;
    field_cType _fieldValue;
};

template<typename struct_cType, typename field_cType>
class LoadFieldAtFunc : public LoadFieldAtTestFunc<struct_cType, field_cType> {
public:
    LoadFieldAtFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : LoadFieldAtTestFunc<struct_cType, field_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void addFieldsBefore(Func::FunctionCompilation *comp, Base::StructTypeBuilder *stb) { }
    virtual void addFieldsAfter(Func::FunctionCompilation *comp, Base::StructTypeBuilder *stb) { }
    virtual const Base::StructType * defineStruct(Func::FunctionCompilation *comp, const Type *ft) {
        Base::StructTypeBuilder stb(this->bx(), comp);
        stb.setName("TheStruct");
        addFieldsBefore(comp, &stb);
        stb.addField("_value", ft, offsetof(struct_cType, _value));
        addFieldsAfter(comp, &stb);
        return stb.create(LOC);
    }
};

TEST(omrgenExtension, LoadFieldFromStructInt8) {
    typedef struct SingleInt8Struct { int8_t _value; } SingleInt8Struct;
    typedef LoadFieldAtFunc<SingleInt8Struct, int8_t> LoadFieldAtSingleInt8Func;
    LoadFieldAtSingleInt8Func *loadsingle_int8 = new (c->mem()) LoadFieldAtSingleInt8Func(MEM_LOC(c->mem()), "loadsingle_int8", c, false);
    loadsingle_int8->test(LOC, true, tInt8, static_cast<int8_t>(0));
    loadsingle_int8->test(LOC, false, tInt8, static_cast<int8_t>(3));
    loadsingle_int8->test(LOC, false, tInt8, static_cast<int8_t>(-1));
    loadsingle_int8->test(LOC, false, tInt8, std::numeric_limits<int8_t>::min());
    loadsingle_int8->test(LOC, false, tInt8, std::numeric_limits<int8_t>::max());
    delete loadsingle_int8;
}
TEST(omrgenExtension, LoadFieldFromStructInt16) {
    typedef struct SingleInt16Struct { int16_t _value; } SingleInt16Struct;
    typedef LoadFieldAtFunc<SingleInt16Struct, int16_t> LoadFieldAtSingleInt16Func;
    LoadFieldAtSingleInt16Func *loadsingle_int16 = new (c->mem()) LoadFieldAtSingleInt16Func(MEM_LOC(c->mem()), "loadsingle_int16", c, false);
    loadsingle_int16->test(LOC, true, tInt16, static_cast<int16_t>(0));
    loadsingle_int16->test(LOC, false, tInt16, static_cast<int16_t>(3));
    loadsingle_int16->test(LOC, false, tInt16, static_cast<int16_t>(-1));
    loadsingle_int16->test(LOC, false, tInt16, std::numeric_limits<int16_t>::min());
    loadsingle_int16->test(LOC, false, tInt16, std::numeric_limits<int16_t>::max());
    delete loadsingle_int16;
}
TEST(omrgenExtension, LoadFieldFromStructInt32) {
    typedef struct SingleInt32Struct { int32_t _value; } SingleInt32Struct;
    typedef LoadFieldAtFunc<SingleInt32Struct, int32_t> LoadFieldAtSingleInt32Func;
    LoadFieldAtSingleInt32Func *loadsingle_int32 = new (c->mem()) LoadFieldAtSingleInt32Func(MEM_LOC(c->mem()), "loadsingle_int32", c, false);
    loadsingle_int32->test(LOC, true, tInt32, static_cast<int32_t>(0));
    loadsingle_int32->test(LOC, false, tInt32, static_cast<int32_t>(3));
    loadsingle_int32->test(LOC, false, tInt32, static_cast<int32_t>(-1));
    loadsingle_int32->test(LOC, false, tInt32, std::numeric_limits<int32_t>::min());
    loadsingle_int32->test(LOC, false, tInt32, std::numeric_limits<int32_t>::max());
    delete loadsingle_int32;
}
TEST(omrgenExtension, LoadFieldFromStructInt64) {
    typedef struct SingleInt64Struct { int64_t _value; } SingleInt64Struct;
    typedef LoadFieldAtFunc<SingleInt64Struct, int64_t> LoadFieldAtSingleInt64Func;
    LoadFieldAtSingleInt64Func *loadsingle_int64 = new (c->mem()) LoadFieldAtSingleInt64Func(MEM_LOC(c->mem()), "loadsingle_int64", c, false);
    loadsingle_int64->test(LOC, true, tInt64, static_cast<int64_t>(0));
    loadsingle_int64->test(LOC, false, tInt64, static_cast<int64_t>(3));
    loadsingle_int64->test(LOC, false, tInt64, static_cast<int64_t>(-1));
    loadsingle_int64->test(LOC, false, tInt64, std::numeric_limits<int64_t>::min());
    loadsingle_int64->test(LOC, false, tInt64, std::numeric_limits<int64_t>::max());
    delete loadsingle_int64;
}
TEST(omrgenExtension, LoadFieldFromStructFloat32) {
    typedef struct SingleFloat32Struct { float _value; } SingleFloat32Struct;
    typedef LoadFieldAtFunc<SingleFloat32Struct, float> LoadFieldAtSingleFloat32Func;
    LoadFieldAtSingleFloat32Func *loadsingle_float32 = new (c->mem()) LoadFieldAtSingleFloat32Func(MEM_LOC(c->mem()), "loadsingle_float32", c, false);
    loadsingle_float32->test(LOC, true, tFloat32, static_cast<float>(0));
    loadsingle_float32->test(LOC, false, tFloat32, static_cast<float>(3));
    loadsingle_float32->test(LOC, false, tFloat32, static_cast<float>(-1));
    loadsingle_float32->test(LOC, false, tFloat32, std::numeric_limits<float>::min());
    loadsingle_float32->test(LOC, false, tFloat32, std::numeric_limits<float>::max());
    delete loadsingle_float32;
}
TEST(omrgenExtension, LoadFieldFromStructFloat64) {
    typedef struct SingleFloat64Struct { double _value; } SingleFloat64Struct;
    typedef LoadFieldAtFunc<SingleFloat64Struct, double> LoadFieldAtSingleFloat64Func;
    LoadFieldAtSingleFloat64Func *loadsingle_float64 = new (c->mem()) LoadFieldAtSingleFloat64Func(MEM_LOC(c->mem()), "loadsingle_float64", c, false);
    loadsingle_float64->test(LOC, true, tFloat64, static_cast<double>(0));
    loadsingle_float64->test(LOC, false, tFloat64, static_cast<double>(3));
    loadsingle_float64->test(LOC, false, tFloat64, static_cast<double>(-1));
    loadsingle_float64->test(LOC, false, tFloat64, std::numeric_limits<double>::min());
    loadsingle_float64->test(LOC, false, tFloat64, std::numeric_limits<double>::max());
    delete loadsingle_float64;
}
TEST(omrgenExtension, LoadFieldFromStructAddress) {
    typedef struct SingleAddressStruct { void * _value; } SingleAddressStruct;
    typedef LoadFieldAtFunc<SingleAddressStruct, void *> LoadFieldAtSingleAddressFunc;
    LoadFieldAtSingleAddressFunc *loadsingle_address = new (c->mem()) LoadFieldAtSingleAddressFunc(MEM_LOC(c->mem()), "loadsingle_address", c, false);
    loadsingle_address->test(LOC, true, tAddress, static_cast<void *>(0));
    loadsingle_address->test(LOC, false, tAddress, reinterpret_cast<void *>(3));
    loadsingle_address->test(LOC, false, tAddress, std::numeric_limits<void *>::min());
    loadsingle_address->test(LOC, false, tAddress, std::numeric_limits<void *>::max());
    delete loadsingle_address;
}

template<typename struct_cType, typename field_cType>
class LoadFieldAtBeforeAfterFunc : public LoadFieldAtFunc<struct_cType, field_cType> {
    typedef field_cType (FuncPrototype)(struct_cType *);
public:
    LoadFieldAtBeforeAfterFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : LoadFieldAtFunc<struct_cType, field_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void addFieldsBefore(Func::FunctionCompilation *comp, Base::StructTypeBuilder *stb) {
        stb->addField("_pad1", this->bx()->Int64(comp->ir()), offsetof(struct_cType, _pad1));
        stb->addField("_pad2", this->bx()->Int32(comp->ir()), offsetof(struct_cType, _pad2));
    }
    virtual void addFieldsAfter(Func::FunctionCompilation *comp, Base::StructTypeBuilder *stb) {
        stb->addField("_pad3", this->bx()->Float64(comp->ir()), offsetof(struct_cType, _pad3));
        stb->addField("_pad4", this->bx()->Float32(comp->ir()), offsetof(struct_cType, _pad4));
    }
    field_cType _notFieldValue;
};

TEST(omrgenExtension, LoadFieldFromStructPaddedInt8) {
    typedef struct PaddedInt8Struct { int64_t _pad1; int32_t _pad2; int8_t _value; double _pad3; float _pad4; } SingleInt8Struct;
    typedef LoadFieldAtBeforeAfterFunc<PaddedInt8Struct, int8_t> LoadFieldAtBeforeAfterInt8Func;
    LoadFieldAtBeforeAfterInt8Func *loadpadded_int8 = new (c->mem()) LoadFieldAtBeforeAfterInt8Func(MEM_LOC(c->mem()), "loadpadded_int8", c, false);
    loadpadded_int8->test(LOC, true, tInt8, static_cast<int8_t>(0));
    loadpadded_int8->test(LOC, false, tInt8, static_cast<int8_t>(3));
    loadpadded_int8->test(LOC, false, tInt8, static_cast<int8_t>(-1));
    loadpadded_int8->test(LOC, false, tInt8, std::numeric_limits<int8_t>::min());
    loadpadded_int8->test(LOC, false, tInt8, std::numeric_limits<int8_t>::max());
    delete loadpadded_int8;
}
TEST(omrgenExtension, LoadFieldFromStructPaddedInt16) {
    typedef struct PaddedInt16Struct { int64_t _pad1; int32_t _pad2; int16_t _value; double _pad3; float _pad4; } SingleInt16Struct;
    typedef LoadFieldAtBeforeAfterFunc<PaddedInt16Struct, int16_t> LoadFieldAtBeforeAfterInt16Func;
    LoadFieldAtBeforeAfterInt16Func *loadpadded_int16 = new (c->mem()) LoadFieldAtBeforeAfterInt16Func(MEM_LOC(c->mem()), "loadpadded_int16", c, false);
    loadpadded_int16->test(LOC, true, tInt16, static_cast<int16_t>(0));
    loadpadded_int16->test(LOC, false, tInt16, static_cast<int16_t>(3));
    loadpadded_int16->test(LOC, false, tInt16, static_cast<int16_t>(-1));
    loadpadded_int16->test(LOC, false, tInt16, std::numeric_limits<int16_t>::min());
    loadpadded_int16->test(LOC, false, tInt16, std::numeric_limits<int16_t>::max());
    delete loadpadded_int16;
}
TEST(omrgenExtension, LoadFieldFromStructPaddedInt32) {
    typedef struct PaddedInt32Struct { int64_t _pad1; int32_t _pad2; int32_t _value; double _pad3; float _pad4; } SingleInt32Struct;
    typedef LoadFieldAtBeforeAfterFunc<PaddedInt32Struct, int32_t> LoadFieldAtBeforeAfterInt32Func;
    LoadFieldAtBeforeAfterInt32Func *loadpadded_int32 = new (c->mem()) LoadFieldAtBeforeAfterInt32Func(MEM_LOC(c->mem()), "loadpadded_int32", c, false);
    loadpadded_int32->test(LOC, true, tInt32, static_cast<int32_t>(0));
    loadpadded_int32->test(LOC, false, tInt32, static_cast<int32_t>(3));
    loadpadded_int32->test(LOC, false, tInt32, static_cast<int32_t>(-1));
    loadpadded_int32->test(LOC, false, tInt32, std::numeric_limits<int32_t>::min());
    loadpadded_int32->test(LOC, false, tInt32, std::numeric_limits<int32_t>::max());
    delete loadpadded_int32;
}
TEST(omrgenExtension, LoadFieldFromStructPaddedInt64) {
    typedef struct PaddedInt64Struct { int64_t _pad1; int32_t _pad2; int64_t _value; double _pad3; float _pad4; } SingleInt64Struct;
    typedef LoadFieldAtBeforeAfterFunc<PaddedInt64Struct, int64_t> LoadFieldAtBeforeAfterInt64Func;
    LoadFieldAtBeforeAfterInt64Func *loadpadded_int64 = new (c->mem()) LoadFieldAtBeforeAfterInt64Func(MEM_LOC(c->mem()), "loadpadded_int64", c, false);
    loadpadded_int64->test(LOC, true, tInt64, static_cast<int64_t>(0));
    loadpadded_int64->test(LOC, false, tInt64, static_cast<int64_t>(3));
    loadpadded_int64->test(LOC, false, tInt64, static_cast<int64_t>(-1));
    loadpadded_int64->test(LOC, false, tInt64, std::numeric_limits<int64_t>::min());
    loadpadded_int64->test(LOC, false, tInt64, std::numeric_limits<int64_t>::max());
    delete loadpadded_int64;
}
TEST(omrgenExtension, LoadFieldFromStructPaddedFloat32) {
    typedef struct PaddedFloat32Struct { int64_t _pad1; int32_t _pad2; float _value; double _pad3; float _pad4; } SingleFloat32Struct;
    typedef LoadFieldAtBeforeAfterFunc<PaddedFloat32Struct, float> LoadFieldAtBeforeAfterFloat32Func;
    LoadFieldAtBeforeAfterFloat32Func *loadpadded_float32 = new (c->mem()) LoadFieldAtBeforeAfterFloat32Func(MEM_LOC(c->mem()), "loadpadded_float32", c, false);
    loadpadded_float32->test(LOC, true, tFloat32, static_cast<float>(0));
    loadpadded_float32->test(LOC, false, tFloat32, static_cast<float>(3));
    loadpadded_float32->test(LOC, false, tFloat32, static_cast<float>(-1));
    loadpadded_float32->test(LOC, false, tFloat32, std::numeric_limits<float>::min());
    loadpadded_float32->test(LOC, false, tFloat32, std::numeric_limits<float>::max());
    delete loadpadded_float32;
}
TEST(omrgenExtension, LoadFieldFromStructPaddedFloat64) {
    typedef struct PaddedFloat64Struct { int64_t _pad1; int32_t _pad2; double _value; double _pad3; float _pad4; } SingleFloat64Struct;
    typedef LoadFieldAtBeforeAfterFunc<PaddedFloat64Struct, double> LoadFieldAtBeforeAfterFloat64Func;
    LoadFieldAtBeforeAfterFloat64Func *loadpadded_float64 = new (c->mem()) LoadFieldAtBeforeAfterFloat64Func(MEM_LOC(c->mem()), "loadpadded_float64", c, false);
    loadpadded_float64->test(LOC, true, tFloat64, static_cast<double>(0));
    loadpadded_float64->test(LOC, false, tFloat64, static_cast<double>(3));
    loadpadded_float64->test(LOC, false, tFloat64, static_cast<double>(-1));
    loadpadded_float64->test(LOC, false, tFloat64, std::numeric_limits<double>::min());
    loadpadded_float64->test(LOC, false, tFloat64, std::numeric_limits<double>::max());
    delete loadpadded_float64;
}
TEST(omrgenExtension, LoadFieldFromStructPaddedAddress) {
    typedef struct PaddedAddressStruct { int64_t _pad1; int32_t _pad2; void * _value; double _pad3; float _pad4; } SingleAddressStruct;
    typedef LoadFieldAtBeforeAfterFunc<PaddedAddressStruct, void *> LoadFieldAtBeforeAfterAddressFunc;
    LoadFieldAtBeforeAfterAddressFunc *loadpadded_address = new (c->mem()) LoadFieldAtBeforeAfterAddressFunc(MEM_LOC(c->mem()), "loadpadded_address", c, false);
    loadpadded_address->test(LOC, true, tAddress, static_cast<void *>(0));
    loadpadded_address->test(LOC, false, tAddress, reinterpret_cast<void *>(3));
    loadpadded_address->test(LOC, false, tAddress, std::numeric_limits<void *>::min());
    loadpadded_address->test(LOC, false, tAddress, std::numeric_limits<void *>::max());
    delete loadpadded_address;
}


template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class MulFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    MulFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->Mul(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, MulInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    MulFunc<FuncProto, int8_t, int8_t, int8_t> *mul_int8s = new (c->mem()) MulFunc<FuncProto,int8_t,int8_t,int8_t>(MEM_LOC(c->mem()), "mul_int8s", c, false);
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(1));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(-1));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(-1));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(1));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(9));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(-9));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(-9));
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(9));
    mul_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::min());
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::min(), tInt8, std::numeric_limits<int8_t>::min());
    mul_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::max());
    mul_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::max(), tInt8, std::numeric_limits<int8_t>::max());
    delete mul_int8s;
}
TEST(omrgenExtension, MulInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    MulFunc<FuncProto, int16_t, int16_t, int16_t> *mul_int16s = new (c->mem()) MulFunc<FuncProto,int16_t,int16_t,int16_t>(MEM_LOC(c->mem()), "mul_int16s", c, false);
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(1));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(-1));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(-1));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(1));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(9));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(-9));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(-9));
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(9));
    mul_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::min());
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::min(), tInt16, std::numeric_limits<int16_t>::min());
    mul_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::max());
    mul_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::max(), tInt16, std::numeric_limits<int16_t>::max());
    delete mul_int16s;
}
TEST(omrgenExtension, MulInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    MulFunc<FuncProto, int32_t, int32_t, int32_t> *mul_int32s = new (c->mem()) MulFunc<FuncProto,int32_t,int32_t,int32_t>(MEM_LOC(c->mem()), "mul_int32s", c, false);
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(1));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(-1));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(-1));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(1));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(9));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(-9));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(-9));
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(9));
    mul_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::min());
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::min(), tInt32, std::numeric_limits<int32_t>::min());
    mul_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::max());
    mul_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::max(), tInt32, std::numeric_limits<int32_t>::max());
    delete mul_int32s;
}
TEST(omrgenExtension, MulInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    MulFunc<FuncProto, int64_t, int64_t, int64_t> *mul_int64s = new (c->mem()) MulFunc<FuncProto,int64_t,int64_t,int64_t>(MEM_LOC(c->mem()), "mul_int64s", c, false);
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(1));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(-1));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(-1));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(1));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(9));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(-9));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(-9));
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(9));
    mul_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::min());
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::min(), tInt64, std::numeric_limits<int64_t>::min());
    mul_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::max());
    mul_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::max(), tInt64, std::numeric_limits<int64_t>::max());
    delete mul_int64s;
}
TEST(omrgenExtension, MulFloat32s) {
    typedef float (FuncProto)(float, float);
    MulFunc<FuncProto, float, float, float> *mul_float32s = new (c->mem()) MulFunc<FuncProto,float,float,float>(MEM_LOC(c->mem()), "mul_float32s", c, false);
    mul_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(0));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(1));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(-1));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(-1));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(1));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(9));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(-9));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(-9));
    mul_float32s->test(LOC, tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(9));
    mul_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, static_cast<float>(1), tFloat32, std::numeric_limits<float>::min());
    mul_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, std::numeric_limits<float>::min(), tFloat32, std::numeric_limits<float>::min());
    mul_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, static_cast<float>(1), tFloat32, std::numeric_limits<float>::max());
    mul_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, std::numeric_limits<float>::max(), tFloat32, std::numeric_limits<float>::max());
    delete mul_float32s;
}
TEST(omrgenExtension, MulFloat64s) {
    typedef double (FuncProto)(double, double);
    MulFunc<FuncProto, double, double, double> *mul_float64s = new (c->mem()) MulFunc<FuncProto,double,double,double>(MEM_LOC(c->mem()), "mul_float64s", c, false);
    mul_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(0));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(1), tFloat64, static_cast<double>(1), tFloat64, static_cast<double>(1));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(-1), tFloat64, static_cast<double>(1), tFloat64, static_cast<double>(-1));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(1), tFloat64, static_cast<double>(-1), tFloat64, static_cast<double>(-1));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(-1), tFloat64, static_cast<double>(-1), tFloat64, static_cast<double>(1));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(9));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(-9));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(-9));
    mul_float64s->test(LOC, tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(9));
    mul_float64s->test(LOC, tFloat64, std::numeric_limits<double>::min(), tFloat64, static_cast<double>(1), tFloat64, std::numeric_limits<double>::min());
    mul_float64s->test(LOC, tFloat64, static_cast<double>(1), tFloat64, std::numeric_limits<double>::min(), tFloat64, std::numeric_limits<double>::min());
    mul_float64s->test(LOC, tFloat64, std::numeric_limits<double>::max(), tFloat64, static_cast<double>(1), tFloat64, std::numeric_limits<double>::max());
    mul_float64s->test(LOC, tFloat64, static_cast<double>(1), tFloat64, std::numeric_limits<double>::max(), tFloat64, std::numeric_limits<double>::max());
    delete mul_float64s;
}

// Test NotEqualTo with two numbers together of the given types
template<typename FuncPrototype, typename left_cType, typename right_cType>
class NotEqualToFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, int32_t> {
public:
    NotEqualToFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, int32_t>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->NotEqualTo(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, NotEqualToInt8s) {
    typedef int32_t (FuncProto)(int8_t, int8_t);
    NotEqualToFunc<FuncProto, int8_t, int8_t> *notequalto_int8s = new (c->mem()) NotEqualToFunc<FuncProto,int8_t,int8_t>(MEM_LOC(c->mem()), "notequalto_int8s", c, false);
    notequalto_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0), tInt32, static_cast<int32_t>(0));
    notequalto_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(3), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(3), tInt32, static_cast<int32_t>(0));
    notequalto_int8s->test(LOC, tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(1), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, static_cast<int8_t>(1), tInt8, static_cast<int8_t>(-1), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, static_cast<int8_t>(-1), tInt8, static_cast<int8_t>(-1), tInt32, static_cast<int32_t>(0));
    notequalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, std::numeric_limits<int8_t>::min(), tInt32, static_cast<int32_t>(0));
    notequalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, std::numeric_limits<int8_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, std::numeric_limits<int8_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, std::numeric_limits<int8_t>::max(), tInt32, static_cast<int32_t>(0));
    delete notequalto_int8s;
}
TEST(omrgenExtension, NotEqualToInt16s) {
    typedef int32_t (FuncProto)(int16_t, int16_t);
    NotEqualToFunc<FuncProto, int16_t, int16_t> *notequalto_int16s = new (c->mem()) NotEqualToFunc<FuncProto,int16_t,int16_t>(MEM_LOC(c->mem()), "notequalto_int16s", c, false);
    notequalto_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0), tInt32, static_cast<int32_t>(0));
    notequalto_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(3), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(3), tInt32, static_cast<int32_t>(0));
    notequalto_int16s->test(LOC, tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(1), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, static_cast<int16_t>(1), tInt16, static_cast<int16_t>(-1), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, static_cast<int16_t>(-1), tInt16, static_cast<int16_t>(-1), tInt32, static_cast<int32_t>(0));
    notequalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, std::numeric_limits<int16_t>::min(), tInt32, static_cast<int32_t>(0));
    notequalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, std::numeric_limits<int16_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, std::numeric_limits<int16_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, std::numeric_limits<int16_t>::max(), tInt32, static_cast<int32_t>(0));
    delete notequalto_int16s;
}
TEST(omrgenExtension, NotEqualToInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    NotEqualToFunc<FuncProto, int32_t, int32_t> *notequalto_int32s = new (c->mem()) NotEqualToFunc<FuncProto,int32_t,int32_t>(MEM_LOC(c->mem()), "notequalto_int32s", c, false);
    notequalto_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    notequalto_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0));
    notequalto_int32s->test(LOC, tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, static_cast<int32_t>(1), tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(-1), tInt32, static_cast<int32_t>(0));
    notequalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0));
    notequalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0));
    delete notequalto_int32s;
}
TEST(omrgenExtension, NotEqualToInt64s) {
    typedef int32_t (FuncProto)(int64_t, int64_t);
    NotEqualToFunc<FuncProto, int64_t, int64_t> *notequalto_int64s = new (c->mem()) NotEqualToFunc<FuncProto,int64_t,int64_t>(MEM_LOC(c->mem()), "notequalto_int64s", c, false);
    notequalto_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0), tInt32, static_cast<int32_t>(0));
    notequalto_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(3), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(3), tInt32, static_cast<int32_t>(0));
    notequalto_int64s->test(LOC, tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(1), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, static_cast<int64_t>(1), tInt64, static_cast<int64_t>(-1), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, static_cast<int64_t>(-1), tInt64, static_cast<int64_t>(-1), tInt32, static_cast<int32_t>(0));
    notequalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, std::numeric_limits<int64_t>::min(), tInt32, static_cast<int32_t>(0));
    notequalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, std::numeric_limits<int64_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, std::numeric_limits<int64_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, std::numeric_limits<int64_t>::max(), tInt32, static_cast<int32_t>(0));
    delete notequalto_int64s;
}
TEST(omrgenExtension, NotEqualToFloat32s) {
    typedef int32_t (FuncProto)(float, float);
    NotEqualToFunc<FuncProto, float, float> *notequalto_float32s = new (c->mem()) NotEqualToFunc<FuncProto,float,float>(MEM_LOC(c->mem()), "notequalto_float32s", c, false);
    notequalto_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0), tInt32, static_cast<int32_t>(0));
    notequalto_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(3), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(0), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(3), tInt32, static_cast<int32_t>(0));
    notequalto_float32s->test(LOC, tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(1), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, static_cast<float>(1), tFloat32, static_cast<float>(-1), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, static_cast<float>(-1), tFloat32, static_cast<float>(-1), tInt32, static_cast<int32_t>(0));
    notequalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, static_cast<float>(0), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(0));
    notequalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, static_cast<float>(0), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(0));
    delete notequalto_float32s;
}
TEST(omrgenExtension, NotEqualToFloat64s) {
    typedef int32_t (FuncProto)(double, double);
    NotEqualToFunc<FuncProto, double, double> *notequalto_float64s = new (c->mem()) NotEqualToFunc<FuncProto,double,double>(MEM_LOC(c->mem()), "notequalto_float64s", c, false);
    notequalto_float64s->test(LOC, tFloat64, static_cast<float>(0), tFloat64, static_cast<float>(0), tInt32, static_cast<int32_t>(0));
    notequalto_float64s->test(LOC, tFloat64, static_cast<float>(0), tFloat64, static_cast<float>(3), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, static_cast<float>(3), tFloat64, static_cast<float>(0), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, static_cast<float>(3), tFloat64, static_cast<float>(3), tInt32, static_cast<int32_t>(0));
    notequalto_float64s->test(LOC, tFloat64, static_cast<float>(-1), tFloat64, static_cast<float>(1), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, static_cast<float>(1), tFloat64, static_cast<float>(-1), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, static_cast<float>(-1), tFloat64, static_cast<float>(-1), tInt32, static_cast<int32_t>(0));
    notequalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::min(), tFloat64, static_cast<float>(0), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, static_cast<float>(0), tFloat64, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::min(), tFloat64, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(0));
    notequalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::min(), tFloat64, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::max(), tFloat64, static_cast<float>(0), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, static_cast<float>(0), tFloat64, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::max(), tFloat64, std::numeric_limits<float>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_float64s->test(LOC, tFloat64, std::numeric_limits<float>::max(), tFloat64, std::numeric_limits<float>::max(), tInt32, static_cast<int32_t>(0));
    delete notequalto_float64s;
}
TEST(omrgenExtension, NotEqualToAddresses) {
    typedef int32_t (FuncProto)(intptr_t, intptr_t);
    NotEqualToFunc<FuncProto, intptr_t, intptr_t> *notequalto_addresses = new (c->mem()) NotEqualToFunc<FuncProto,intptr_t,intptr_t>(MEM_LOC(c->mem()), "notequalto_addresses", c, false);
    notequalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(0), tAddress, static_cast<intptr_t>(0), tInt32, static_cast<int32_t>(0));
    notequalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(0), tAddress, static_cast<intptr_t>(3), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(3), tAddress, static_cast<intptr_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(3), tAddress, static_cast<intptr_t>(3), tInt32, static_cast<int32_t>(0));
    notequalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(-1), tAddress, static_cast<intptr_t>(1), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(1), tAddress, static_cast<intptr_t>(-1), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(-1), tAddress, static_cast<intptr_t>(-1), tInt32, static_cast<int32_t>(0));
    notequalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), tAddress, static_cast<intptr_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(0), tAddress, std::numeric_limits<intptr_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), tAddress, std::numeric_limits<intptr_t>::min(), tInt32, static_cast<int32_t>(0));
    notequalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), tAddress, std::numeric_limits<intptr_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tAddress, static_cast<intptr_t>(0), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, static_cast<intptr_t>(0), tAddress, std::numeric_limits<intptr_t>::max(), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tAddress, std::numeric_limits<intptr_t>::min(), tInt32, static_cast<int32_t>(1));
    notequalto_addresses->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tAddress, std::numeric_limits<intptr_t>::max(), tInt32, static_cast<int32_t>(0));
    delete notequalto_addresses;
}


class GotoOpFunc : public TestFunc {
public:
    GotoOpFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log) {
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
        ctx->DefineParameter("value", this->bx()->Int8(comp->ir()));
        ctx->DefineReturnType(this->bx()->Int8(comp->ir()));
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
    GotoOpFunc *gotoFunc = new (c->mem()) GotoOpFunc(MEM_LOC(c->mem()), "gotoFunc", c, false);
    gotoFunc->compile(LOC);
    gotoFunc->test(LOC, static_cast<int8_t>(0), static_cast<int8_t>(3));
    gotoFunc->test(LOC, static_cast<int8_t>(1), static_cast<int8_t>(3));
    gotoFunc->test(LOC, static_cast<int8_t>(-1), static_cast<int8_t>(3));
    gotoFunc->test(LOC, std::numeric_limits<int8_t>::min(), static_cast<int8_t>(3));
    gotoFunc->test(LOC, std::numeric_limits<int8_t>::max(), static_cast<int8_t>(3));
    delete gotoFunc;
}

// Base test class for IfCmp<condition> opcodes to evalute whether fall-through or taken path occurs
// Compiled code returns int8_t: 0 if fall-through path, 1 if taken path
template<typename FuncPrototype, typename condition_cType>
class IfCmpToZeroBaseFunc : public TestFunc {
public:
    IfCmpToZeroBaseFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _conditionTid(NoTypeID)
        , _conditionType(NULL)
        , _conditionValue(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_conditionValue), _resultValue) << "Compiled f(" << _conditionValue << ") returns " << _resultValue;
    }
    void compile(LOCATION, const TypeID conditionTid) {
        _conditionTid = conditionTid;
        this->TestFunc::compile(PASSLOC);
    }

    void test(LOCATION, const TypeID conditionTid, condition_cType conditionValue, int8_t resultValue) {
        _conditionTid = conditionTid;
        _conditionValue = conditionValue;
        _resultValue = resultValue;
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _conditionType = comp->ir()->typedict()->Lookup(_conditionTid);
        ctx->DefineParameter("condition", _conditionType);
        ctx->DefineReturnType(this->bx()->Int8(comp->ir()));
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

    TypeID _conditionTid;
    const Type *_conditionType;
    condition_cType _conditionValue;
    int8_t _resultValue;
};

template<typename FuncPrototype, typename condition_cType>
class IfCmpZeroFunc : public IfCmpToZeroBaseFunc<FuncPrototype, condition_cType> {
public:
    IfCmpZeroFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpToZeroBaseFunc<FuncPrototype, condition_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpZero(LOCATION, Builder *b, Builder *target, Value *condition) {
        return this->bx()->IfCmpEqualZero(PASSLOC, b, target, condition);
    }
};

TEST(omrgenExtension, IfCmpZeroInt8s) {
    typedef int8_t (FuncProto)(int8_t);
    IfCmpZeroFunc<FuncProto, int8_t> *cmpzero_int8 = new (c->mem()) IfCmpZeroFunc<FuncProto, int8_t>(MEM_LOC(c->mem()), "cmpzero_int8", c, false);
    cmpzero_int8->compile(LOC, tInt8);
    cmpzero_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpzero_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpzero_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmpzero_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpzero_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    delete cmpzero_int8;
}
TEST(omrgenExtension, IfCmpZeroInt16s) {
    typedef int8_t (FuncProto)(int16_t);
    IfCmpZeroFunc<FuncProto, int16_t> *cmpzero_int16 = new (c->mem()) IfCmpZeroFunc<FuncProto, int16_t>(MEM_LOC(c->mem()), "cmpzero_int16", c, false);
    cmpzero_int16->compile(LOC, tInt16);
    cmpzero_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpzero_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpzero_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmpzero_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpzero_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    delete cmpzero_int16;
}
TEST(omrgenExtension, IfCmpZeroInt32s) {
    typedef int8_t (FuncProto)(int32_t);
    IfCmpZeroFunc<FuncProto, int32_t> *cmpzero_int32 = new (c->mem()) IfCmpZeroFunc<FuncProto, int32_t>(MEM_LOC(c->mem()), "cmpzero_int32", c, false);
    cmpzero_int32->compile(LOC, tInt32);
    cmpzero_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpzero_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpzero_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmpzero_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpzero_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    delete cmpzero_int32;
}
TEST(omrgenExtension, IfCmpZeroInt64s) {
    typedef int8_t (FuncProto)(int64_t);
    IfCmpZeroFunc<FuncProto, int64_t> *cmpzero_int64 = new (c->mem()) IfCmpZeroFunc<FuncProto, int64_t>(MEM_LOC(c->mem()), "cmpzero_int64", c, false);
    cmpzero_int64->compile(LOC, tInt64);
    cmpzero_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpzero_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpzero_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmpzero_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpzero_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    delete cmpzero_int64;
}
TEST(omrgenExtension, IfCmpZeroFloat32s) {
    typedef int8_t (FuncProto)(float);
    IfCmpZeroFunc<FuncProto, float> *cmpzero_float32 = new (c->mem()) IfCmpZeroFunc<FuncProto, float>(MEM_LOC(c->mem()), "cmpzero_float32", c, false);
    cmpzero_float32->compile(LOC, tFloat32);
    cmpzero_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<int8_t>(1));
    cmpzero_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<int8_t>(0));
    cmpzero_float32->test(LOC, tFloat32, static_cast<float>(-1), static_cast<int8_t>(0));
    cmpzero_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmpzero_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    delete cmpzero_float32;
}
TEST(omrgenExtension, IfCmpZeroFloat64s) {
    typedef int8_t (FuncProto)(double);
    IfCmpZeroFunc<FuncProto, double> *cmpzero_float64 = new (c->mem()) IfCmpZeroFunc<FuncProto, double>(MEM_LOC(c->mem()), "cmpzero_float64", c, false);
    cmpzero_float64->compile(LOC, tFloat64);
    cmpzero_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<int8_t>(1));
    cmpzero_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<int8_t>(0));
    cmpzero_float64->test(LOC, tFloat64, static_cast<double>(-1), static_cast<int8_t>(0));
    cmpzero_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmpzero_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    delete cmpzero_float64;
}
TEST(omrgenExtension, IfCmpZeroAddresses) {
    typedef int8_t (FuncProto)(uintptr_t);
    IfCmpZeroFunc<FuncProto, uintptr_t> *cmpzero_address = new (c->mem()) IfCmpZeroFunc<FuncProto, uintptr_t>(MEM_LOC(c->mem()), "cmpzero_address", c, false);
    cmpzero_address->compile(LOC, tAddress);
    cmpzero_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmpzero_address->test(LOC, tAddress, static_cast<uintptr_t>(1), static_cast<int8_t>(0));
    cmpzero_address->test(LOC, tAddress, static_cast<uintptr_t>(-1), static_cast<int8_t>(0));
    cmpzero_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmpzero_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
    delete cmpzero_address;
}

template<typename FuncPrototype, typename condition_cType>
class IfCmpNotZeroFunc : public IfCmpToZeroBaseFunc<FuncPrototype, condition_cType> {
public:
    IfCmpNotZeroFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpToZeroBaseFunc<FuncPrototype, condition_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpZero(LOCATION, Builder *b, Builder *target, Value *condition) {
        return this->bx()->IfCmpNotEqualZero(PASSLOC, b, target, condition);
    }
};

TEST(omrgenExtension, IfCmpNotZeroInt8s) {
    typedef int8_t (FuncProto)(int8_t);
    IfCmpNotZeroFunc<FuncProto, int8_t> *cmpnotzero_int8 = new (c->mem()) IfCmpNotZeroFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmpnotzero_int8", c, false);
    cmpnotzero_int8->compile(LOC, tInt8);
    cmpnotzero_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpnotzero_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpnotzero_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpnotzero_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    delete cmpnotzero_int8;
}
TEST(omrgenExtension, IfCmpNotZeroInt16s) {
    typedef int8_t (FuncProto)(int16_t);
    IfCmpNotZeroFunc<FuncProto, int16_t> *cmpnotzero_int16 = new (c->mem()) IfCmpNotZeroFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmpnotzero_int16", c, false);
    cmpnotzero_int16->compile(LOC, tInt16);
    cmpnotzero_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpnotzero_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpnotzero_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpnotzero_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    delete cmpnotzero_int16;
}
TEST(omrgenExtension, IfCmpNotZeroInt32s) {
    typedef int8_t (FuncProto)(int32_t);
    IfCmpNotZeroFunc<FuncProto, int32_t> *cmpnotzero_int32 = new (c->mem()) IfCmpNotZeroFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmpnotzero_int32", c, false);
    cmpnotzero_int32->compile(LOC, tInt32);
    cmpnotzero_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpnotzero_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpnotzero_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpnotzero_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    delete cmpnotzero_int32;
}
TEST(omrgenExtension, IfCmpNotZeroInt64s) {
    typedef int8_t (FuncProto)(int64_t);
    IfCmpNotZeroFunc<FuncProto, int64_t> *cmpnotzero_int64 = new (c->mem()) IfCmpNotZeroFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmpnotzero_int64", c, false);
    cmpnotzero_int64->compile(LOC, tInt64);
    cmpnotzero_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpnotzero_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpnotzero_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpnotzero_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    delete cmpnotzero_int64;
}
TEST(omrgenExtension, IfCmpNotZeroFloat32s) {
    typedef int8_t (FuncProto)(float);
    IfCmpNotZeroFunc<FuncProto, float> *cmpnotzero_float32 = new (c->mem()) IfCmpNotZeroFunc<FuncProto,float>(MEM_LOC(c->mem()), "cmpnotzero_float32", c, false);
    cmpnotzero_float32->compile(LOC, tFloat32);
    cmpnotzero_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<int8_t>(0));
    cmpnotzero_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<int8_t>(1));
    cmpnotzero_float32->test(LOC, tFloat32, static_cast<float>(-1), static_cast<int8_t>(1));
    cmpnotzero_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpnotzero_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    delete cmpnotzero_float32;
}
TEST(omrgenExtension, IfCmpNotZeroFloat64s) {
    typedef int8_t (FuncProto)(double);
    IfCmpNotZeroFunc<FuncProto, double> *cmpnotzero_float64 = new (c->mem()) IfCmpNotZeroFunc<FuncProto,double>(MEM_LOC(c->mem()), "cmpnotzero_float64", c, false);
    cmpnotzero_float64->compile(LOC, tFloat64);
    cmpnotzero_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<int8_t>(0));
    cmpnotzero_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<int8_t>(1));
    cmpnotzero_float64->test(LOC, tFloat64, static_cast<double>(-1), static_cast<int8_t>(1));
    cmpnotzero_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpnotzero_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    delete cmpnotzero_float64;
}
TEST(omrgenExtension, IfCmpNotZeroAddresses) {
    typedef int8_t (FuncProto)(uintptr_t);
    IfCmpNotZeroFunc<FuncProto, uintptr_t> *cmpnotzero_address = new (c->mem()) IfCmpNotZeroFunc<FuncProto,uintptr_t>(MEM_LOC(c->mem()), "cmpnotzero_address", c, false);
    cmpnotzero_address->compile(LOC, tAddress);
    cmpnotzero_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmpnotzero_address->test(LOC, tAddress, static_cast<uintptr_t>(1), static_cast<int8_t>(1));
    cmpnotzero_address->test(LOC, tAddress, static_cast<uintptr_t>(-1), static_cast<int8_t>(1));
    cmpnotzero_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmpnotzero_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
    delete cmpnotzero_address;
}

// Base test class for IfCmp<condition> opcodes to evalute whether fall-through or taken path occurs
// Compiled code returns int8_t: 0 if fall-through path, 1 if taken path
template<typename FuncPrototype, typename cType>
class IfCmpOpBaseFunc : public TestFunc {
public:
    IfCmpOpBaseFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _tid(NoTypeID)
        , _type(NULL)
        , _leftValue(0)
        , _rightValue(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_leftValue,_rightValue), _resultValue) << "Compiled f(" << _leftValue << ", " << _rightValue << ") returns " << _resultValue;
    }
    void compile(LOCATION, const TypeID tid) {
        _tid = tid;
        this->TestFunc::compile(PASSLOC);
    }
    void test(LOCATION, const TypeID tid, cType leftValue, cType rightValue, int8_t resultValue) {
        _tid = tid;
        _leftValue = leftValue;
        _rightValue = rightValue;
        _resultValue = resultValue;
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _type = comp->ir()->typedict()->Lookup(_tid);
        ctx->DefineParameter("left", _type);
        ctx->DefineParameter("right", _type);
        ctx->DefineReturnType(this->bx()->Int8(comp->ir()));
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

    TypeID _tid;
    const Type *_type;
    cType _leftValue;
    cType _rightValue;
    int8_t _resultValue;
};

template<typename FuncPrototype, typename cType>
class IfCmpEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpEqualFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpEqualFunc<FuncProto, int8_t> *cmpequal_int8 = new (c->mem()) IfCmpEqualFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmpequal_int8", c, false);
    cmpequal_int8->compile(LOC, tInt8);
    cmpequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpequal_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmpequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    delete cmpequal_int8;
}
TEST(omrgenExtension, IfCmpEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpEqualFunc<FuncProto, int16_t> *cmpequal_int16 = new (c->mem()) IfCmpEqualFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmpequal_int16", c, false);
    cmpequal_int16->compile(LOC, tInt16);
    cmpequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpequal_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmpequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    delete cmpequal_int16;
}
TEST(omrgenExtension, IfCmpEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpEqualFunc<FuncProto, int32_t> *cmpequal_int32 = new (c->mem()) IfCmpEqualFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmpequal_int32", c, false);
    cmpequal_int32->compile(LOC, tInt32);
    cmpequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpequal_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmpequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    delete cmpequal_int32;
}
TEST(omrgenExtension, IfCmpEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpEqualFunc<FuncProto, int64_t> *cmpequal_int64 = new (c->mem()) IfCmpEqualFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmpequal_int64", c, false);
    cmpequal_int64->compile(LOC, tInt64);
    cmpequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpequal_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmpequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    delete cmpequal_int64;
}
TEST(omrgenExtension, IfCmpEqualFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpEqualFunc<FuncProto, float> *cmpequal_float32 = new (c->mem()) IfCmpEqualFunc<FuncProto,float>(MEM_LOC(c->mem()), "cmpequal_float32", c, false);
    cmpequal_float32->compile(LOC, tFloat32);
    cmpequal_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(1));
    cmpequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(0));
    cmpequal_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(0));
    cmpequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(1));
    cmpequal_float32->test(LOC, tFloat32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(0));
    cmpequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    cmpequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmpequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    delete cmpequal_float32;
}
TEST(omrgenExtension, IfCmpEqualFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpEqualFunc<FuncProto, double> *cmpequal_float64 = new (c->mem()) IfCmpEqualFunc<FuncProto,double>(MEM_LOC(c->mem()), "cmpequal_float64", c, false);
    cmpequal_float64->compile(LOC, tFloat64);
    cmpequal_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(1));
    cmpequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(0));
    cmpequal_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(0));
    cmpequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(1));
    cmpequal_float64->test(LOC, tFloat64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(0));
    cmpequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    cmpequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmpequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    delete cmpequal_float64;
}
TEST(omrgenExtension, IfCmpEqualAddresses) {
    typedef int8_t (FuncProto)(intptr_t, intptr_t);
    IfCmpEqualFunc<FuncProto, intptr_t> *cmpequal_address = new (c->mem()) IfCmpEqualFunc<FuncProto,intptr_t>(MEM_LOC(c->mem()), "cmpequal_address", c, false);
    cmpequal_address->compile(LOC, tAddress);
    cmpequal_address->test(LOC, tAddress, static_cast<intptr_t>(0), static_cast<intptr_t>(0), static_cast<int8_t>(1));
    cmpequal_address->test(LOC, tAddress, static_cast<intptr_t>(4), static_cast<intptr_t>(0), static_cast<int8_t>(0));
    cmpequal_address->test(LOC, tAddress, static_cast<intptr_t>(0), static_cast<intptr_t>(4), static_cast<int8_t>(0));
    cmpequal_address->test(LOC, tAddress, static_cast<intptr_t>(4), static_cast<intptr_t>(4), static_cast<int8_t>(1));
    cmpequal_address->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), std::numeric_limits<intptr_t>::min(), static_cast<int8_t>(1));
    cmpequal_address->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), std::numeric_limits<intptr_t>::max(), static_cast<int8_t>(0));
    cmpequal_address->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), std::numeric_limits<intptr_t>::min(), static_cast<int8_t>(0));
    cmpequal_address->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), std::numeric_limits<intptr_t>::max(), static_cast<int8_t>(1));
    delete cmpequal_address;
}

template<typename FuncPrototype, typename cType>
class IfCmpNotEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpNotEqualFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpNotEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpNotEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpNotEqualFunc<FuncProto, int8_t> *cmpnotequal_int8 = new (c->mem()) IfCmpNotEqualFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmpnotequal_int8", c, false);
    cmpnotequal_int8->compile(LOC, tInt8);
    cmpnotequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpnotequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpnotequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpnotequal_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpnotequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    delete cmpnotequal_int8;
}
TEST(omrgenExtension, IfCmpNotEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpNotEqualFunc<FuncProto, int16_t> *cmpnotequal_int16 = new (c->mem()) IfCmpNotEqualFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmpnotequal_int16", c, false);
    cmpnotequal_int16->compile(LOC, tInt16);
    cmpnotequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpnotequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpnotequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpnotequal_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpnotequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    delete cmpnotequal_int16;
}
TEST(omrgenExtension, IfCmpNotEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpNotEqualFunc<FuncProto, int32_t> *cmpnotequal_int32 = new (c->mem()) IfCmpNotEqualFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmpnotequal_int32", c, false);
    cmpnotequal_int32->compile(LOC, tInt32);
    cmpnotequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpnotequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpnotequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpnotequal_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpnotequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    delete cmpnotequal_int32;
}
TEST(omrgenExtension, IfCmpNotEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpNotEqualFunc<FuncProto, int64_t> *cmpnotequal_int64 = new (c->mem()) IfCmpNotEqualFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmpnotequal_int64", c, false);
    cmpnotequal_int64->compile(LOC, tInt64);
    cmpnotequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpnotequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpnotequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpnotequal_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpnotequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpnotequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    delete cmpnotequal_int64;
}
TEST(omrgenExtension, IfCmpNotEqualFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpNotEqualFunc<FuncProto, float> *cmpnotequal_float32 = new (c->mem()) IfCmpNotEqualFunc<FuncProto,float>(MEM_LOC(c->mem()), "cmpnotequal_float32", c, false);
    cmpnotequal_float32->compile(LOC, tFloat32);
    cmpnotequal_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(0));
    cmpnotequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(1));
    cmpnotequal_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(1));
    cmpnotequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpnotequal_float32->test(LOC, tFloat32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(1));
    cmpnotequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(1));
    cmpnotequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmpnotequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    cmpnotequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpnotequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    delete cmpnotequal_float32;
}
TEST(omrgenExtension, IfCmpNotEqualFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpNotEqualFunc<FuncProto, double> *cmpnotequal_float64 = new (c->mem()) IfCmpNotEqualFunc<FuncProto,double>(MEM_LOC(c->mem()), "cmpnotequal_float64", c, false);
    cmpnotequal_float64->compile(LOC, tFloat64);
    cmpnotequal_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(0));
    cmpnotequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(1));
    cmpnotequal_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(1));
    cmpnotequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpnotequal_float64->test(LOC, tFloat64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(1));
    cmpnotequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(1));
    cmpnotequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmpnotequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    cmpnotequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpnotequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    delete cmpnotequal_float64;
}
TEST(omrgenExtension, IfCmpNotEqualAddresses) {
    typedef int8_t (FuncProto)(intptr_t, intptr_t);
    IfCmpNotEqualFunc<FuncProto, intptr_t> *cmpnotequal_address = new (c->mem()) IfCmpNotEqualFunc<FuncProto,intptr_t>(MEM_LOC(c->mem()), "cmpnotequal_address", c, false);
    cmpnotequal_address->compile(LOC, tAddress);
    cmpnotequal_address->test(LOC, tAddress, static_cast<intptr_t>(0), static_cast<intptr_t>(0), static_cast<int8_t>(0));
    cmpnotequal_address->test(LOC, tAddress, static_cast<intptr_t>(4), static_cast<intptr_t>(0), static_cast<int8_t>(1));
    cmpnotequal_address->test(LOC, tAddress, static_cast<intptr_t>(0), static_cast<intptr_t>(4), static_cast<int8_t>(1));
    cmpnotequal_address->test(LOC, tAddress, static_cast<intptr_t>(4), static_cast<intptr_t>(4), static_cast<int8_t>(0));
    cmpnotequal_address->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), std::numeric_limits<intptr_t>::min(), static_cast<int8_t>(0));
    cmpnotequal_address->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), std::numeric_limits<intptr_t>::max(), static_cast<int8_t>(1));
    cmpnotequal_address->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), std::numeric_limits<intptr_t>::min(), static_cast<int8_t>(1));
    cmpnotequal_address->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), std::numeric_limits<intptr_t>::max(), static_cast<int8_t>(0));
    delete cmpnotequal_address;
}

template<typename FuncPrototype, typename cType>
class IfCmpGreaterThanFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpGreaterThanFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpGreaterThan(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpGreaterThanInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpGreaterThanFunc<FuncProto, int8_t> *cmpgreaterthan_int8 = new (c->mem()) IfCmpGreaterThanFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmpgreaterthan_int8", c, false);
    cmpgreaterthan_int8->compile(LOC, tInt8);
    cmpgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    delete cmpgreaterthan_int8;
}
TEST(omrgenExtension, IfCmpGreaterThanInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpGreaterThanFunc<FuncProto, int16_t> *cmpgreaterthan_int16 = new (c->mem()) IfCmpGreaterThanFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmpgreaterthan_int16", c, false);
    cmpgreaterthan_int16->compile(LOC, tInt16);
    cmpgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    delete cmpgreaterthan_int16;
}
TEST(omrgenExtension, IfCmpGreaterThanInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpGreaterThanFunc<FuncProto, int32_t> *cmpgreaterthan_int32 = new (c->mem()) IfCmpGreaterThanFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmpgreaterthan_int32", c, false);
    cmpgreaterthan_int32->compile(LOC, tInt32);
    cmpgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    delete cmpgreaterthan_int32;
}
TEST(omrgenExtension, IfCmpGreaterThanInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpGreaterThanFunc<FuncProto, int64_t> *cmpgreaterthan_int64 = new (c->mem()) IfCmpGreaterThanFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmpgreaterthan_int64", c, false);
    cmpgreaterthan_int64->compile(LOC, tInt64);
    cmpgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    delete cmpgreaterthan_int64;
}
TEST(omrgenExtension, IfCmpGreaterThanFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpGreaterThanFunc<FuncProto, float> *cmpgreaterthan_float32 = new (c->mem()) IfCmpGreaterThanFunc<FuncProto,float>(MEM_LOC(c->mem()), "cmpgreaterthan_float32", c, false);
    cmpgreaterthan_float32->compile(LOC, tFloat32);
    cmpgreaterthan_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(0));
    cmpgreaterthan_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(1));
    cmpgreaterthan_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterthan_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterthan_float32->test(LOC, tFloat32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterthan_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    delete cmpgreaterthan_float32;
}
TEST(omrgenExtension, IfCmpGreaterThanFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpGreaterThanFunc<FuncProto, double> *cmpgreaterthan_float64 = new (c->mem()) IfCmpGreaterThanFunc<FuncProto,double>(MEM_LOC(c->mem()), "cmpgreaterthan_float64", c, false);
    cmpgreaterthan_float64->compile(LOC, tFloat64);
    cmpgreaterthan_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(0));
    cmpgreaterthan_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(1));
    cmpgreaterthan_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterthan_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterthan_float64->test(LOC, tFloat64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterthan_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(1));
    cmpgreaterthan_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    delete cmpgreaterthan_float64;
}
TEST(omrgenExtension, IfCmpGreaterThanAddresses) {
    typedef int8_t (FuncProto)(uintptr_t, uintptr_t);
    IfCmpGreaterThanFunc<FuncProto, uintptr_t> *cmpgreaterthan_address = new (c->mem()) IfCmpGreaterThanFunc<FuncProto,uintptr_t>(MEM_LOC(c->mem()), "cmpgreaterthan_address", c, false);
    cmpgreaterthan_address->compile(LOC, tAddress);
    cmpgreaterthan_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmpgreaterthan_address->test(LOC, tAddress, static_cast<uintptr_t>(4), static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmpgreaterthan_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<uintptr_t>(4), static_cast<int8_t>(0));
    cmpgreaterthan_address->test(LOC, tAddress, static_cast<uintptr_t>(4), static_cast<uintptr_t>(4), static_cast<int8_t>(0));
    cmpgreaterthan_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmpgreaterthan_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
    cmpgreaterthan_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmpgreaterthan_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
    delete cmpgreaterthan_address;
}

template<typename FuncPrototype, typename cType>
class IfCmpGreaterOrEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpGreaterOrEqualFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpGreaterOrEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpGreaterOrEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpGreaterOrEqualFunc<FuncProto, int8_t> *cmpgreaterorequal_int8 = new (c->mem()) IfCmpGreaterOrEqualFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmpgreaterorequal_int8", c, false);
    cmpgreaterorequal_int8->compile(LOC, tInt8);
    cmpgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    delete cmpgreaterorequal_int8;
}
TEST(omrgenExtension, IfCmpGreaterOrEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpGreaterOrEqualFunc<FuncProto, int16_t> *cmpgreaterorequal_int16 = new (c->mem()) IfCmpGreaterOrEqualFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmpgreaterorequal_int16", c, false);
    cmpgreaterorequal_int16->compile(LOC, tInt16);
    cmpgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    delete cmpgreaterorequal_int16;
}
TEST(omrgenExtension, IfCmpGreaterOrEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpGreaterOrEqualFunc<FuncProto, int32_t> *cmpgreaterorequal_int32 = new (c->mem()) IfCmpGreaterOrEqualFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmpgreaterorequal_int32", c, false);
    cmpgreaterorequal_int32->compile(LOC, tInt32);
    cmpgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    delete cmpgreaterorequal_int32;
}
TEST(omrgenExtension, IfCmpGreaterOrEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpGreaterOrEqualFunc<FuncProto, int64_t> *cmpgreaterorequal_int64 = new (c->mem()) IfCmpGreaterOrEqualFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmpgreaterorequal_int64", c, false);
    cmpgreaterorequal_int64->compile(LOC, tInt64);
    cmpgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    delete cmpgreaterorequal_int64;
}
TEST(omrgenExtension, IfCmpGreaterOrEqualFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpGreaterOrEqualFunc<FuncProto, float> *cmpgreaterorequal_float32 = new (c->mem()) IfCmpGreaterOrEqualFunc<FuncProto,float>(MEM_LOC(c->mem()), "cmpgreaterorequal_float32", c, false);
    cmpgreaterorequal_float32->compile(LOC, tFloat32);
    cmpgreaterorequal_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_float32->test(LOC, tFloat32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    delete cmpgreaterorequal_float32;
}
TEST(omrgenExtension, IfCmpGreaterOrEqualFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpGreaterOrEqualFunc<FuncProto, double> *cmpgreaterorequal_float64 = new (c->mem()) IfCmpGreaterOrEqualFunc<FuncProto,double>(MEM_LOC(c->mem()), "cmpgreaterorequal_float64", c, false);
    cmpgreaterorequal_float64->compile(LOC, tFloat64);
    cmpgreaterorequal_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(1));
    cmpgreaterorequal_float64->test(LOC, tFloat64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(0));
    cmpgreaterorequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(1));
    cmpgreaterorequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    delete cmpgreaterorequal_float64;
}
TEST(omrgenExtension, IfCmpGreaterOrEqualAddresses) {
    typedef int8_t (FuncProto)(uintptr_t, uintptr_t);
    IfCmpGreaterOrEqualFunc<FuncProto, uintptr_t> *cmpgreaterorequal_address = new (c->mem()) IfCmpGreaterOrEqualFunc<FuncProto,uintptr_t>(MEM_LOC(c->mem()), "cmpgreaterorequal_address", c, false);
    cmpgreaterorequal_address->compile(LOC, tAddress);
    cmpgreaterorequal_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_address->test(LOC, tAddress, static_cast<uintptr_t>(4), static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmpgreaterorequal_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<uintptr_t>(4), static_cast<int8_t>(0));
    cmpgreaterorequal_address->test(LOC, tAddress, static_cast<uintptr_t>(4), static_cast<uintptr_t>(4), static_cast<int8_t>(1));
    cmpgreaterorequal_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
    cmpgreaterorequal_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmpgreaterorequal_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
    delete cmpgreaterorequal_address;
}

template<typename FuncPrototype, typename cType>
class IfCmpLessThanFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpLessThanFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpLessThan(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpLessThanInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpLessThanFunc<FuncProto, int8_t> *cmplessthan_int8 = new (c->mem()) IfCmpLessThanFunc<FuncProto, int8_t>(MEM_LOC(c->mem()), "cmplessthan_int8", c, false);
    cmplessthan_int8->compile(LOC, tInt8);
    cmplessthan_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmplessthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmplessthan_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmplessthan_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmplessthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmplessthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    delete cmplessthan_int8;
}
TEST(omrgenExtension, IfCmpLessThanInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpLessThanFunc<FuncProto, int16_t> *cmplessthan_int16 = new (c->mem()) IfCmpLessThanFunc<FuncProto, int16_t>(MEM_LOC(c->mem()), "cmplessthan_int16", c, false);
    cmplessthan_int16->compile(LOC, tInt16);
    cmplessthan_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmplessthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmplessthan_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmplessthan_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmplessthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmplessthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    delete cmplessthan_int16;
}
TEST(omrgenExtension, IfCmpLessThanInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpLessThanFunc<FuncProto, int32_t> *cmplessthan_int32 = new (c->mem()) IfCmpLessThanFunc<FuncProto, int32_t>(MEM_LOC(c->mem()), "cmplessthan_int32", c, false);
    cmplessthan_int32->compile(LOC, tInt32);
    cmplessthan_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmplessthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmplessthan_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmplessthan_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmplessthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmplessthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    delete cmplessthan_int32;
}
TEST(omrgenExtension, IfCmpLessThanInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpLessThanFunc<FuncProto, int64_t> *cmplessthan_int64 = new (c->mem()) IfCmpLessThanFunc<FuncProto, int64_t>(MEM_LOC(c->mem()), "cmplessthan_int64", c, false);
    cmplessthan_int64->compile(LOC, tInt64);
    cmplessthan_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmplessthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmplessthan_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmplessthan_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmplessthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmplessthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmplessthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    delete cmplessthan_int64;
}
TEST(omrgenExtension, IfCmpLessThanFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpLessThanFunc<FuncProto, float> *cmplessthan_float32 = new (c->mem()) IfCmpLessThanFunc<FuncProto, float>(MEM_LOC(c->mem()), "cmplessthan_float32", c, false);
    cmplessthan_float32->compile(LOC, tFloat32);
    cmplessthan_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(0));
    cmplessthan_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(0));
    cmplessthan_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessthan_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(0));
    cmplessthan_float32->test(LOC, tFloat32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessthan_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(0));
    cmplessthan_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmplessthan_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    cmplessthan_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmplessthan_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(0));
    delete cmplessthan_float32;
}
TEST(omrgenExtension, IfCmpLessThanFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpLessThanFunc<FuncProto, double> *cmplessthan_float64 = new (c->mem()) IfCmpLessThanFunc<FuncProto, double>(MEM_LOC(c->mem()), "cmplessthan_float64", c, false);
    cmplessthan_float64->compile(LOC, tFloat64);
    cmplessthan_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(0));
    cmplessthan_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(0));
    cmplessthan_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessthan_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(0));
    cmplessthan_float64->test(LOC, tFloat64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessthan_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(0));
    cmplessthan_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmplessthan_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    cmplessthan_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmplessthan_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(0));
    delete cmplessthan_float64;
}
TEST(omrgenExtension, IfCmpLessThanAddresses) {
    typedef int8_t (FuncProto)(uintptr_t, uintptr_t);
    IfCmpLessThanFunc<FuncProto, uintptr_t> *cmplessthan_address = new (c->mem()) IfCmpLessThanFunc<FuncProto, uintptr_t>(MEM_LOC(c->mem()), "cmplessthan_address", c, false);
    cmplessthan_address->compile(LOC, tAddress);
    cmplessthan_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmplessthan_address->test(LOC, tAddress, static_cast<uintptr_t>(4), static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmplessthan_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<uintptr_t>(4), static_cast<int8_t>(1));
    cmplessthan_address->test(LOC, tAddress, static_cast<uintptr_t>(4), static_cast<uintptr_t>(4), static_cast<int8_t>(0));
    cmplessthan_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmplessthan_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
    cmplessthan_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmplessthan_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(0));
    delete cmplessthan_address;
}

template<typename FuncPrototype, typename cType>
class IfCmpLessOrEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpLessOrEqualFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpLessOrEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpLessOrEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpLessOrEqualFunc<FuncProto, int8_t> *cmplessorequal_int8 = new (c->mem()) IfCmpLessOrEqualFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmplessorequal_int8", c, false);
    cmplessorequal_int8->compile(LOC, tInt8);
    cmplessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmplessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmplessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmplessorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    delete cmplessorequal_int8;
}
TEST(omrgenExtension, IfCmpLessOrEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpLessOrEqualFunc<FuncProto, int16_t> *cmplessorequal_int16 = new (c->mem()) IfCmpLessOrEqualFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmplessorequal_int16", c, false);
    cmplessorequal_int16->compile(LOC, tInt16);
    cmplessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmplessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmplessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmplessorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    delete cmplessorequal_int16;
}
TEST(omrgenExtension, IfCmpLessOrEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpLessOrEqualFunc<FuncProto, int32_t> *cmplessorequal_int32 = new (c->mem()) IfCmpLessOrEqualFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmplessorequal_int32", c, false);
    cmplessorequal_int32->compile(LOC, tInt32);
    cmplessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmplessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmplessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmplessorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    delete cmplessorequal_int32;
}
TEST(omrgenExtension, IfCmpLessOrEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpLessOrEqualFunc<FuncProto, int64_t> *cmplessorequal_int64 = new (c->mem()) IfCmpLessOrEqualFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmplessorequal_int64", c, false);
    cmplessorequal_int64->compile(LOC, tInt64);
    cmplessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmplessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmplessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmplessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmplessorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    delete cmplessorequal_int64;
}
TEST(omrgenExtension, IfCmpLessOrEqualFloat32s) {
    typedef int8_t (FuncProto)(float, float);
    IfCmpLessOrEqualFunc<FuncProto, float> *cmplessorequal_float32 = new (c->mem()) IfCmpLessOrEqualFunc<FuncProto,float>(MEM_LOC(c->mem()), "cmplessorequal_float32", c, false);
    cmplessorequal_float32->compile(LOC, tFloat32);
    cmplessorequal_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(0), static_cast<int8_t>(1));
    cmplessorequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(0), static_cast<int8_t>(0));
    cmplessorequal_float32->test(LOC, tFloat32, static_cast<float>(0), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessorequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessorequal_float32->test(LOC, tFloat32, static_cast<float>(-1), static_cast<float>(1), static_cast<int8_t>(1));
    cmplessorequal_float32->test(LOC, tFloat32, static_cast<float>(1), static_cast<float>(-1), static_cast<int8_t>(0));
    cmplessorequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), static_cast<int8_t>(1));
    cmplessorequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    cmplessorequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::min(), static_cast<int8_t>(0));
    cmplessorequal_float32->test(LOC, tFloat32, std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), static_cast<int8_t>(1));
    delete cmplessorequal_float32;
}
TEST(omrgenExtension, IfCmpLessOrEqualFloat64s) {
    typedef int8_t (FuncProto)(double, double);
    IfCmpLessOrEqualFunc<FuncProto, double> *cmplessorequal_float64 = new (c->mem()) IfCmpLessOrEqualFunc<FuncProto,double>(MEM_LOC(c->mem()), "cmplessorequal_float64", c, false);
    cmplessorequal_float64->compile(LOC, tFloat64);
    cmplessorequal_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(0), static_cast<int8_t>(1));
    cmplessorequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(0), static_cast<int8_t>(0));
    cmplessorequal_float64->test(LOC, tFloat64, static_cast<double>(0), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessorequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessorequal_float64->test(LOC, tFloat64, static_cast<double>(-1), static_cast<double>(1), static_cast<int8_t>(1));
    cmplessorequal_float64->test(LOC, tFloat64, static_cast<double>(1), static_cast<double>(-1), static_cast<int8_t>(0));
    cmplessorequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), static_cast<int8_t>(1));
    cmplessorequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    cmplessorequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), static_cast<int8_t>(0));
    cmplessorequal_float64->test(LOC, tFloat64, std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), static_cast<int8_t>(1));
    delete cmplessorequal_float64;
}
TEST(omrgenExtension, IfCmpLessOrEqualAddresses) {
    typedef int8_t (FuncProto)(uintptr_t, uintptr_t);
    IfCmpLessOrEqualFunc<FuncProto, uintptr_t> *cmplessorequal_address = new (c->mem()) IfCmpLessOrEqualFunc<FuncProto,uintptr_t>(MEM_LOC(c->mem()), "cmplessorequal_uintptr_t", c, false);
    cmplessorequal_address->compile(LOC, tAddress);
    cmplessorequal_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<uintptr_t>(0), static_cast<int8_t>(1));
    cmplessorequal_address->test(LOC, tAddress, static_cast<uintptr_t>(4), static_cast<uintptr_t>(0), static_cast<int8_t>(0));
    cmplessorequal_address->test(LOC, tAddress, static_cast<uintptr_t>(0), static_cast<uintptr_t>(4), static_cast<int8_t>(1));
    cmplessorequal_address->test(LOC, tAddress, static_cast<uintptr_t>(4), static_cast<uintptr_t>(4), static_cast<int8_t>(1));
    cmplessorequal_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(1));
    cmplessorequal_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::min(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
    cmplessorequal_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::min(), static_cast<int8_t>(0));
    cmplessorequal_address->test(LOC, tAddress, std::numeric_limits<uintptr_t>::max(), std::numeric_limits<uintptr_t>::max(), static_cast<int8_t>(1));
    delete cmplessorequal_address;
}

template<typename FuncPrototype, typename cType>
class IfCmpUnsignedGreaterThanFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpUnsignedGreaterThanFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpUnsignedGreaterThan(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpUnsignedGreaterThanInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpUnsignedGreaterThanFunc<FuncProto, int8_t> *cmpunsignedgreaterthan_int8 = new (c->mem()) IfCmpUnsignedGreaterThanFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmpunsignedgreaterthan_int8", c, false);
    cmpunsignedgreaterthan_int8->compile(LOC, tInt8);
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    delete cmpunsignedgreaterthan_int8;
}
TEST(omrgenExtension, IfCmpUnsignedGreaterThanInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpUnsignedGreaterThanFunc<FuncProto, int16_t> *cmpunsignedgreaterthan_int16 = new (c->mem()) IfCmpUnsignedGreaterThanFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmpunsignedgreaterthan_int16", c, false);
    cmpunsignedgreaterthan_int16->compile(LOC, tInt16);
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    delete cmpunsignedgreaterthan_int16;
}
TEST(omrgenExtension, IfCmpUnsignedGreaterThanInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpUnsignedGreaterThanFunc<FuncProto, int32_t> *cmpunsignedgreaterthan_int32 = new (c->mem()) IfCmpUnsignedGreaterThanFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmpunsignedgreaterthan_int32", c, false);
    cmpunsignedgreaterthan_int32->compile(LOC, tInt32);
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    delete cmpunsignedgreaterthan_int32;
}
TEST(omrgenExtension, IfCmpUnsignedGreaterThanInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpUnsignedGreaterThanFunc<FuncProto, int64_t> *cmpunsignedgreaterthan_int64 = new (c->mem()) IfCmpUnsignedGreaterThanFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmpunsignedgreaterthan_int64", c, false);
    cmpunsignedgreaterthan_int64->compile(LOC, tInt64);
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    delete cmpunsignedgreaterthan_int64;
}
// no unsigned comparisons for Float32,Float64,Address

template<typename FuncPrototype, typename cType>
class IfCmpUnsignedGreaterOrEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpUnsignedGreaterOrEqualFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpUnsignedGreaterOrEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpUnsignedGreaterOrEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpUnsignedGreaterOrEqualFunc<FuncProto, int8_t> *cmpunsignedgreaterorequal_int8 = new (c->mem()) IfCmpUnsignedGreaterOrEqualFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmpunsignedgreaterorequal_int8", c, false);
    cmpunsignedgreaterorequal_int8->compile(LOC, tInt8);
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    delete cmpunsignedgreaterorequal_int8;
}
TEST(omrgenExtension, IfCmpUnsignedGreaterOrEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpUnsignedGreaterOrEqualFunc<FuncProto, int16_t> *cmpunsignedgreaterorequal_int16 = new (c->mem()) IfCmpUnsignedGreaterOrEqualFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmpunsignedgreaterorequal_int16", c, false);
    cmpunsignedgreaterorequal_int16->compile(LOC, tInt16);
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    delete cmpunsignedgreaterorequal_int16;
}
TEST(omrgenExtension, IfCmpUnsignedGreaterOrEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpUnsignedGreaterOrEqualFunc<FuncProto, int32_t> *cmpunsignedgreaterorequal_int32 = new (c->mem()) IfCmpUnsignedGreaterOrEqualFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmpunsignedgreaterorequal_int32", c, false);
    cmpunsignedgreaterorequal_int32->compile(LOC, tInt32);
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    delete cmpunsignedgreaterorequal_int32;
}
TEST(omrgenExtension, IfCmpUnsignedGreaterOrEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpUnsignedGreaterOrEqualFunc<FuncProto, int64_t> *cmpunsignedgreaterorequal_int64 = new (c->mem()) IfCmpUnsignedGreaterOrEqualFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmpunsignedgreaterorequal_int64", c, false);
    cmpunsignedgreaterorequal_int64->compile(LOC, tInt64);
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpunsignedgreaterorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    delete cmpunsignedgreaterorequal_int64;
}
// no unsigned comparisons for Float32,Float64,Address

template<typename FuncPrototype, typename cType>
class IfCmpUnsignedLessThanFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpUnsignedLessThanFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpUnsignedLessThan(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpUnsignedLessThanInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpUnsignedLessThanFunc<FuncProto, int8_t> *cmpunsignedlessthan_int8 = new (c->mem()) IfCmpUnsignedLessThanFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmpunsignedlessthan_int8", c, false);
    cmpunsignedlessthan_int8->compile(LOC, tInt8);
    cmpunsignedlessthan_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessthan_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    delete cmpunsignedlessthan_int8;
}
TEST(omrgenExtension, IfCmpUnsignedLessThanInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpUnsignedLessThanFunc<FuncProto, int16_t> *cmpunsignedlessthan_int16 = new (c->mem()) IfCmpUnsignedLessThanFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmpunsignedlessthan_int16", c, false);
    cmpunsignedlessthan_int16->compile(LOC, tInt16);
    cmpunsignedlessthan_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessthan_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    delete cmpunsignedlessthan_int16;
}
TEST(omrgenExtension, IfCmpUnsignedLessThanInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpUnsignedLessThanFunc<FuncProto, int32_t> *cmpunsignedlessthan_int32 = new (c->mem()) IfCmpUnsignedLessThanFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmpunsignedlessthan_int32", c, false);
    cmpunsignedlessthan_int32->compile(LOC, tInt32);
    cmpunsignedlessthan_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessthan_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    delete cmpunsignedlessthan_int32;
}
TEST(omrgenExtension, IfCmpUnsignedLessThanInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpUnsignedLessThanFunc<FuncProto, int64_t> *cmpunsignedlessthan_int64 = new (c->mem()) IfCmpUnsignedLessThanFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmpunsignedlessthan_int64", c, false);
    cmpunsignedlessthan_int64->compile(LOC, tInt64);
    cmpunsignedlessthan_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessthan_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    delete cmpunsignedlessthan_int64;
}
// no unsigned comparisons for Float32,Float64,Address

template<typename FuncPrototype, typename cType>
class IfCmpUnsignedLessOrEqualFunc : public IfCmpOpBaseFunc<FuncPrototype, cType> {
public:
    IfCmpUnsignedLessOrEqualFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : IfCmpOpBaseFunc<FuncPrototype, cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void doIfCmpOp(LOCATION, Builder *b, Builder *target, Value *left, Value *right) {
        this->bx()->IfCmpUnsignedLessOrEqual(PASSLOC, b, target, left, right);
    }
};

TEST(omrgenExtension, IfCmpUnsignedLessOrEqualInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    IfCmpUnsignedLessOrEqualFunc<FuncProto, int8_t> *cmpunsignedlessorequal_int8 = new (c->mem()) IfCmpUnsignedLessOrEqualFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "cmpunsignedlessorequal_int8", c, false);
    cmpunsignedlessorequal_int8->compile(LOC, tInt8);
    cmpunsignedlessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(0), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int8->test(LOC, tInt8, static_cast<int8_t>(1), static_cast<int8_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int8->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), std::numeric_limits<int8_t>::max(), static_cast<int8_t>(1));
    delete cmpunsignedlessorequal_int8;
}
TEST(omrgenExtension, IfCmpUnsignedLessOrEqualInt16s) {
    typedef int8_t (FuncProto)(int16_t, int16_t);
    IfCmpUnsignedLessOrEqualFunc<FuncProto, int16_t> *cmpunsignedlessorequal_int16 = new (c->mem()) IfCmpUnsignedLessOrEqualFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "cmpunsignedlessorequal_int16", c, false);
    cmpunsignedlessorequal_int16->compile(LOC, tInt16);
    cmpunsignedlessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int16->test(LOC, tInt16, static_cast<int16_t>(1), static_cast<int16_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int16->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max(), static_cast<int8_t>(1));
    delete cmpunsignedlessorequal_int16;
}
TEST(omrgenExtension, IfCmpUnsignedLessOrEqualInt32s) {
    typedef int8_t (FuncProto)(int32_t, int32_t);
    IfCmpUnsignedLessOrEqualFunc<FuncProto, int32_t> *cmpunsignedlessorequal_int32 = new (c->mem()) IfCmpUnsignedLessOrEqualFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "cmpunsignedlessorequal_int32", c, false);
    cmpunsignedlessorequal_int32->compile(LOC, tInt32);
    cmpunsignedlessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int32->test(LOC, tInt32, static_cast<int32_t>(1), static_cast<int32_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int32->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), static_cast<int8_t>(1));
    delete cmpunsignedlessorequal_int32;
}
TEST(omrgenExtension, IfCmpUnsignedLessOrEqualInt64s) {
    typedef int8_t (FuncProto)(int64_t, int64_t);
    IfCmpUnsignedLessOrEqualFunc<FuncProto, int64_t> *cmpunsignedlessorequal_int64 = new (c->mem()) IfCmpUnsignedLessOrEqualFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "cmpunsignedlessorequal_int64", c, false);
    cmpunsignedlessorequal_int64->compile(LOC, tInt64);
    cmpunsignedlessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(0), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(0), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(1), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int64->test(LOC, tInt64, static_cast<int64_t>(1), static_cast<int64_t>(-1), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(0));
    cmpunsignedlessorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min(), static_cast<int8_t>(1));
    cmpunsignedlessorequal_int64->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), static_cast<int8_t>(1));
    delete cmpunsignedlessorequal_int64;
}
// no unsigned comparisons for Float32,Float64,Address

// Base class for IndexAt
template<typename FuncPrototype, typename index_cType>
class IndexAtFunc : public TestFunc {
public:
    IndexAtFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _elementTid(NoTypeID)
        , _elementType(NULL)
        , _baseValue(0)
        , _baseType(0)
        , _indexTid(NoTypeID)
        , _indexType(NULL)
        , _indexValue(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        EXPECT_EQ(f(_baseValue,_indexValue), _resultValue) << "Compiled f(" << _baseValue << ", " << (int64_t)_indexValue << ") returns " << _resultValue;
    }
    void test(LOCATION, const TypeID elementTid, uintptr_t baseValue, const TypeID indexTid, index_cType indexValue, uintptr_t resultValue) {
        _elementTid = elementTid;
        _baseValue = baseValue;
        _indexTid = indexTid;
        _indexValue = indexValue;
        _resultValue = resultValue;
        compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _elementType = comp->ir()->typedict()->Lookup(_elementTid);
        _indexType = comp->ir()->typedict()->Lookup(_indexTid);
        _baseType = bx()->PointerTo(PASSLOC, _elementType);
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
    TypeID _elementTid;
    const Type *_elementType;
    const Type *_baseType;
    uintptr_t _baseValue;
    TypeID _indexTid;
    const Type *_indexType;
    index_cType _indexValue;
    uintptr_t _resultValue;
};

TEST(omrgenExtension, IndexAtInt8byInt8) {
    typedef uintptr_t (FuncProto)(uintptr_t, int8_t);
    IndexAtFunc<FuncProto, int8_t> *indexat_int8_int8 = new (c->mem()) IndexAtFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "indexat_int8_int8", c, false);
    char array[4];
    indexat_int8_int8->test(LOC, tInt8, static_cast<uintptr_t>(0), tInt8, static_cast<int8_t>(0), static_cast<uintptr_t>(0));
    indexat_int8_int8->test(LOC, tInt8, static_cast<uintptr_t>(4), tInt8, static_cast<int8_t>(4), static_cast<uintptr_t>(8));
    indexat_int8_int8->test(LOC, tInt8, static_cast<uintptr_t>(8), tInt8, static_cast<int8_t>(-4), static_cast<uintptr_t>(4));
    indexat_int8_int8->test(LOC, tInt8, reinterpret_cast<uintptr_t>(array), tInt8, static_cast<int8_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int8_int8->test(LOC, tInt8, static_cast<uintptr_t>(0), tInt8, std::numeric_limits<int8_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int8_t>::max()));
    delete indexat_int8_int8;
}
TEST(omrgenExtension, IndexAtInt8byInt16) {
    typedef uintptr_t (FuncProto)(uintptr_t, int16_t);
    IndexAtFunc<FuncProto, int16_t> *indexat_int8_int16 = new (c->mem()) IndexAtFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "indexat_int8_int16", c, false);
    char array[4];
    indexat_int8_int16->test(LOC, tInt8, static_cast<uintptr_t>(0), tInt16, static_cast<int16_t>(0), static_cast<uintptr_t>(0));
    indexat_int8_int16->test(LOC, tInt8, static_cast<uintptr_t>(4), tInt16, static_cast<int16_t>(4), static_cast<uintptr_t>(8));
    indexat_int8_int16->test(LOC, tInt8, static_cast<uintptr_t>(8), tInt16, static_cast<int16_t>(-4), static_cast<uintptr_t>(4));
    indexat_int8_int16->test(LOC, tInt8, reinterpret_cast<uintptr_t>(array), tInt16, static_cast<int16_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int8_int16->test(LOC, tInt8, static_cast<uintptr_t>(0), tInt16, std::numeric_limits<int16_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int16_t>::max()));
    delete indexat_int8_int16;
}
TEST(omrgenExtension, IndexAtInt8byInt32) {
    typedef uintptr_t (FuncProto)(uintptr_t, int32_t);
    IndexAtFunc<FuncProto, int32_t> *indexat_int8_int32 = new (c->mem()) IndexAtFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "indexat_int8_int32", c, false);
    char array[4];
    indexat_int8_int32->test(LOC, tInt8, static_cast<uintptr_t>(0), tInt32, static_cast<int32_t>(0), static_cast<uintptr_t>(0));
    indexat_int8_int32->test(LOC, tInt8, static_cast<uintptr_t>(4), tInt32, static_cast<int32_t>(4), static_cast<uintptr_t>(8));
    indexat_int8_int32->test(LOC, tInt8, static_cast<uintptr_t>(8), tInt32, static_cast<int32_t>(-4), static_cast<uintptr_t>(4));
    indexat_int8_int32->test(LOC, tInt8, reinterpret_cast<uintptr_t>(array), tInt32, static_cast<int32_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int8_int32->test(LOC, tInt8, static_cast<uintptr_t>(0), tInt32, std::numeric_limits<int32_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int32_t>::max()));
    delete indexat_int8_int32;
}
TEST(omrgenExtension, IndexAtInt8byInt64) {
    typedef uintptr_t (FuncProto)(uintptr_t, int64_t);
    IndexAtFunc<FuncProto, int64_t> *indexat_int8_int64 = new (c->mem()) IndexAtFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "indexat_int8_int64", c, false);
    char array[4];
    indexat_int8_int64->test(LOC, tInt8, static_cast<uintptr_t>(0), tInt64, static_cast<int64_t>(0), static_cast<uintptr_t>(0));
    indexat_int8_int64->test(LOC, tInt8, static_cast<uintptr_t>(4), tInt64, static_cast<int64_t>(4), static_cast<uintptr_t>(8));
    indexat_int8_int64->test(LOC, tInt8, static_cast<uintptr_t>(8), tInt64, static_cast<int64_t>(-4), static_cast<uintptr_t>(4));
    indexat_int8_int64->test(LOC, tInt8, reinterpret_cast<uintptr_t>(array), tInt64, static_cast<int64_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int8_int64->test(LOC, tInt8, static_cast<uintptr_t>(0), tInt64, std::numeric_limits<int64_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int64_t>::max()));
    delete indexat_int8_int64;
}
TEST(omrgenExtension, IndexAtInt16byInt8) {
    typedef uintptr_t (FuncProto)(uintptr_t, int8_t);
    IndexAtFunc<FuncProto, int8_t> *indexat_int16_int8 = new (c->mem()) IndexAtFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "indexat_int16_int8", c, false);
    uint16_t array[4];
    indexat_int16_int8->test(LOC, tInt16, static_cast<uintptr_t>(0), tInt8, static_cast<int8_t>(0), static_cast<uintptr_t>(0));
    indexat_int16_int8->test(LOC, tInt16, static_cast<uintptr_t>(4), tInt8, static_cast<int8_t>(4), static_cast<uintptr_t>(12));
    indexat_int16_int8->test(LOC, tInt16, static_cast<uintptr_t>(8), tInt8, static_cast<int8_t>(-4), static_cast<uintptr_t>(0));
    indexat_int16_int8->test(LOC, tInt16, reinterpret_cast<uintptr_t>(array), tInt8, static_cast<int8_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int16_int8->test(LOC, tInt16, static_cast<uintptr_t>(0), tInt8, std::numeric_limits<int8_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int8_t>::max()) << 1);
    delete indexat_int16_int8;
}
TEST(omrgenExtension, IndexAtInt16byInt16) {
    typedef uintptr_t (FuncProto)(uintptr_t, int16_t);
    IndexAtFunc<FuncProto, int16_t> *indexat_int16_int16 = new (c->mem()) IndexAtFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "indexat_int16_int16", c, false);
    uint16_t array[4];
    indexat_int16_int16->test(LOC, tInt16, static_cast<uintptr_t>(0), tInt16, static_cast<int16_t>(0), static_cast<uintptr_t>(0));
    indexat_int16_int16->test(LOC, tInt16, static_cast<uintptr_t>(4), tInt16, static_cast<int16_t>(4), static_cast<uintptr_t>(12));
    indexat_int16_int16->test(LOC, tInt16, static_cast<uintptr_t>(8), tInt16, static_cast<int16_t>(-4), static_cast<uintptr_t>(0));
    indexat_int16_int16->test(LOC, tInt16, reinterpret_cast<uintptr_t>(array), tInt16, static_cast<int16_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int16_int16->test(LOC, tInt16, static_cast<uintptr_t>(0), tInt16, std::numeric_limits<int16_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int16_t>::max()) << 1);
    delete indexat_int16_int16;
}
TEST(omrgenExtension, IndexAtInt16byInt32) {
    typedef uintptr_t (FuncProto)(uintptr_t, int32_t);
    IndexAtFunc<FuncProto, int32_t> *indexat_int16_int32 = new (c->mem()) IndexAtFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "indexat_int16_int32", c, false);
    uint16_t array[4];
    indexat_int16_int32->test(LOC, tInt16, static_cast<uintptr_t>(0), tInt32, static_cast<int32_t>(0), static_cast<uintptr_t>(0));
    indexat_int16_int32->test(LOC, tInt16, static_cast<uintptr_t>(4), tInt32, static_cast<int32_t>(4), static_cast<uintptr_t>(12));
    indexat_int16_int32->test(LOC, tInt16, static_cast<uintptr_t>(8), tInt32, static_cast<int32_t>(-4), static_cast<uintptr_t>(0));
    indexat_int16_int32->test(LOC, tInt16, reinterpret_cast<uintptr_t>(array), tInt32, static_cast<int32_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int16_int32->test(LOC, tInt16, static_cast<uintptr_t>(0), tInt32, std::numeric_limits<int32_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int32_t>::max()) << 1);
    delete indexat_int16_int32;
}
TEST(omrgenExtension, IndexAtInt16byInt64) {
    typedef uintptr_t (FuncProto)(uintptr_t, int64_t);
    IndexAtFunc<FuncProto, int64_t> *indexat_int16_int64 = new (c->mem()) IndexAtFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "indexat_int16_int64", c, false);
    uint16_t array[4];
    indexat_int16_int64->test(LOC, tInt16, static_cast<uintptr_t>(0), tInt64, static_cast<int64_t>(0), static_cast<uintptr_t>(0));
    indexat_int16_int64->test(LOC, tInt16, static_cast<uintptr_t>(4), tInt64, static_cast<int64_t>(4), static_cast<uintptr_t>(12));
    indexat_int16_int64->test(LOC, tInt16, static_cast<uintptr_t>(8), tInt64, static_cast<int64_t>(-4), static_cast<uintptr_t>(0));
    indexat_int16_int64->test(LOC, tInt16, reinterpret_cast<uintptr_t>(array), tInt64, static_cast<int64_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int16_int64->test(LOC, tInt16, static_cast<uintptr_t>(0), tInt64, std::numeric_limits<int64_t>::max() >> 1, static_cast<uintptr_t>((std::numeric_limits<int64_t>::max() >> 1) << 1));
    delete indexat_int16_int64;
}
TEST(omrgenExtension, IndexAtInt32byInt8) {
    typedef uintptr_t (FuncProto)(uintptr_t, int8_t);
    IndexAtFunc<FuncProto, int8_t> *indexat_int32_int8 = new (c->mem()) IndexAtFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "indexat_int32_int8", c, false);
    uint32_t array[4];
    indexat_int32_int8->test(LOC, tInt32, static_cast<uintptr_t>(0), tInt8, static_cast<int8_t>(0), static_cast<uintptr_t>(0));
    indexat_int32_int8->test(LOC, tInt32, static_cast<uintptr_t>(4), tInt8, static_cast<int8_t>(4), static_cast<uintptr_t>(20));
    indexat_int32_int8->test(LOC, tInt32, static_cast<uintptr_t>(20), tInt8, static_cast<int8_t>(-4), static_cast<uintptr_t>(4));
    indexat_int32_int8->test(LOC, tInt32, reinterpret_cast<uintptr_t>(array), tInt8, static_cast<int8_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int32_int8->test(LOC, tInt32, static_cast<uintptr_t>(0), tInt8, std::numeric_limits<int8_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int8_t>::max()) << 2);
    delete indexat_int32_int8;
}
TEST(omrgenExtension, IndexAtInt32byInt16) {
    typedef uintptr_t (FuncProto)(uintptr_t, int16_t);
    IndexAtFunc<FuncProto, int16_t> *indexat_int32_int16 = new (c->mem()) IndexAtFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "indexat_int32_int16", c, false);
    uint32_t array[4];
    indexat_int32_int16->test(LOC, tInt32, static_cast<uintptr_t>(0), tInt16, static_cast<int16_t>(0), static_cast<uintptr_t>(0));
    indexat_int32_int16->test(LOC, tInt32, static_cast<uintptr_t>(4), tInt16, static_cast<int16_t>(4), static_cast<uintptr_t>(20));
    indexat_int32_int16->test(LOC, tInt32, static_cast<uintptr_t>(20), tInt16, static_cast<int16_t>(-4), static_cast<uintptr_t>(4));
    indexat_int32_int16->test(LOC, tInt32, reinterpret_cast<uintptr_t>(array), tInt16, static_cast<int16_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int32_int16->test(LOC, tInt32, static_cast<uintptr_t>(0), tInt16, std::numeric_limits<int16_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int16_t>::max()) << 2);
    delete indexat_int32_int16;
}
TEST(omrgenExtension, IndexAtInt32byInt32) {
    typedef uintptr_t (FuncProto)(uintptr_t, int32_t);
    IndexAtFunc<FuncProto, int32_t> *indexat_int32_int32 = new (c->mem()) IndexAtFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "indexat_int32_int32", c, false);
    uint32_t array[4];
    indexat_int32_int32->test(LOC, tInt32, static_cast<uintptr_t>(0), tInt32, static_cast<int32_t>(0), static_cast<uintptr_t>(0));
    indexat_int32_int32->test(LOC, tInt32, static_cast<uintptr_t>(4), tInt32, static_cast<int32_t>(4), static_cast<uintptr_t>(20));
    indexat_int32_int32->test(LOC, tInt32, static_cast<uintptr_t>(20), tInt32, static_cast<int32_t>(-4), static_cast<uintptr_t>(4));
    indexat_int32_int32->test(LOC, tInt32, reinterpret_cast<uintptr_t>(array), tInt32, static_cast<int32_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int32_int32->test(LOC, tInt32, static_cast<uintptr_t>(0), tInt32, std::numeric_limits<int32_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int32_t>::max()) << 2);
    delete indexat_int32_int32;
}
TEST(omrgenExtension, IndexAtInt32byInt64) {
    typedef uintptr_t (FuncProto)(uintptr_t, int64_t);
    IndexAtFunc<FuncProto, int64_t> *indexat_int32_int64 = new (c->mem()) IndexAtFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "indexat_int32_int64", c, false);
    uint32_t array[4];
    indexat_int32_int64->test(LOC, tInt32, static_cast<uintptr_t>(0), tInt64, static_cast<int64_t>(0), static_cast<uintptr_t>(0));
    indexat_int32_int64->test(LOC, tInt32, static_cast<uintptr_t>(4), tInt64, static_cast<int64_t>(4), static_cast<uintptr_t>(20));
    indexat_int32_int64->test(LOC, tInt32, static_cast<uintptr_t>(20), tInt64, static_cast<int64_t>(-4), static_cast<uintptr_t>(4));
    indexat_int32_int64->test(LOC, tInt32, reinterpret_cast<uintptr_t>(array), tInt64, static_cast<int64_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int32_int64->test(LOC, tInt32, static_cast<uintptr_t>(0), tInt64, std::numeric_limits<int64_t>::max() >> 2, static_cast<uintptr_t>((std::numeric_limits<int64_t>::max() >> 2) << 2));
    delete indexat_int32_int64;
}
TEST(omrgenExtension, IndexAtInt64byInt8) {
    typedef uintptr_t (FuncProto)(uintptr_t, int8_t);
    IndexAtFunc<FuncProto, int8_t> *indexat_int64_int8 = new (c->mem()) IndexAtFunc<FuncProto,int8_t>(MEM_LOC(c->mem()), "indexat_int64_int8", c, false);
    uint64_t array[4];
    indexat_int64_int8->test(LOC, tInt64, static_cast<uintptr_t>(0), tInt8, static_cast<int8_t>(0), static_cast<uintptr_t>(0));
    indexat_int64_int8->test(LOC, tInt64, static_cast<uintptr_t>(4), tInt8, static_cast<int8_t>(4), static_cast<uintptr_t>(36));
    indexat_int64_int8->test(LOC, tInt64, static_cast<uintptr_t>(36), tInt8, static_cast<int8_t>(-4), static_cast<uintptr_t>(4));
    indexat_int64_int8->test(LOC, tInt64, reinterpret_cast<uintptr_t>(array), tInt8, static_cast<int8_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int64_int8->test(LOC, tInt64, static_cast<uintptr_t>(0), tInt8, std::numeric_limits<int8_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int8_t>::max()) << 3);
    delete indexat_int64_int8;
}
TEST(omrgenExtension, IndexAtInt64byInt16) {
    typedef uintptr_t (FuncProto)(uintptr_t, int16_t);
    IndexAtFunc<FuncProto, int16_t> *indexat_int64_int16 = new (c->mem()) IndexAtFunc<FuncProto,int16_t>(MEM_LOC(c->mem()), "indexat_int64_int16", c, false);
    uint64_t array[4];
    indexat_int64_int16->test(LOC, tInt64, static_cast<uintptr_t>(0), tInt16, static_cast<int16_t>(0), static_cast<uintptr_t>(0));
    indexat_int64_int16->test(LOC, tInt64, static_cast<uintptr_t>(4), tInt16, static_cast<int16_t>(4), static_cast<uintptr_t>(36));
    indexat_int64_int16->test(LOC, tInt64, static_cast<uintptr_t>(36), tInt16, static_cast<int16_t>(-4), static_cast<uintptr_t>(4));
    indexat_int64_int16->test(LOC, tInt64, reinterpret_cast<uintptr_t>(array), tInt16, static_cast<int16_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int64_int16->test(LOC, tInt64, static_cast<uintptr_t>(0), tInt16, std::numeric_limits<int16_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int16_t>::max()) << 3);
    delete indexat_int64_int16;
}
TEST(omrgenExtension, IndexAtInt64byInt32) {
    typedef uintptr_t (FuncProto)(uintptr_t, int32_t);
    IndexAtFunc<FuncProto, int32_t> *indexat_int64_int32 = new (c->mem()) IndexAtFunc<FuncProto,int32_t>(MEM_LOC(c->mem()), "indexat_int64_int32", c, false);
    uint64_t array[4];
    indexat_int64_int32->test(LOC, tInt64, static_cast<uintptr_t>(0), tInt32, static_cast<int32_t>(0), static_cast<uintptr_t>(0));
    indexat_int64_int32->test(LOC, tInt64, static_cast<uintptr_t>(4), tInt32, static_cast<int32_t>(4), static_cast<uintptr_t>(36));
    indexat_int64_int32->test(LOC, tInt64, static_cast<uintptr_t>(36), tInt32, static_cast<int32_t>(-4), static_cast<uintptr_t>(4));
    indexat_int64_int32->test(LOC, tInt64, reinterpret_cast<uintptr_t>(array), tInt32, static_cast<int32_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int64_int32->test(LOC, tInt64, static_cast<uintptr_t>(0), tInt32, std::numeric_limits<int32_t>::max(), static_cast<uintptr_t>(std::numeric_limits<int32_t>::max()) << 3);
    delete indexat_int64_int32;
}
TEST(omrgenExtension, IndexAtInt64byInt64) {
    typedef uintptr_t (FuncProto)(uintptr_t, int64_t);
    IndexAtFunc<FuncProto, int64_t> *indexat_int64_int64 = new (c->mem()) IndexAtFunc<FuncProto,int64_t>(MEM_LOC(c->mem()), "indexat_int64_int64", c, false);
    uint64_t array[4];
    indexat_int64_int64->test(LOC, tInt64, static_cast<uintptr_t>(0), tInt64, static_cast<int64_t>(0), static_cast<uintptr_t>(0));
    indexat_int64_int64->test(LOC, tInt64, static_cast<uintptr_t>(4), tInt64, static_cast<int64_t>(4), static_cast<uintptr_t>(36));
    indexat_int64_int64->test(LOC, tInt64, static_cast<uintptr_t>(36), tInt64, static_cast<int64_t>(-4), static_cast<uintptr_t>(4));
    indexat_int64_int64->test(LOC, tInt64, reinterpret_cast<uintptr_t>(array), tInt64, static_cast<int64_t>(4), reinterpret_cast<uintptr_t>(array+4));
    indexat_int64_int64->test(LOC, tInt64, static_cast<uintptr_t>(0), tInt64, std::numeric_limits<int64_t>::max() >> 3, static_cast<uintptr_t>((std::numeric_limits<int64_t>::max() >> 3) << 3));
    delete indexat_int64_int64;
}


// Test class for nested IfCmp opcodes to evalute more complicated conditional structures
// Since all the basic IfCmp opcodes work (earlier tests) just use IfCmpGreaterThan with Int64
// Compiled code returns int8_t and a different number for each of the possible paths.
class NestedControlFlowFunc : public TestFunc {
public:
    NestedControlFlowFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _numLeaves(0)
        , _numInternal(0)
        , _internalNodes(NULL)
        , _resultValue(0) {
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
        ctx->DefineParameter("path", this->bx()->Int64(comp->ir()));
        ctx->DefineReturnType(this->bx()->Int64(comp->ir()));
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
    NestedControlFlowFunc *nest2 = new (c->mem()) NestedControlFlowFunc(MEM_LOC(c->mem()), "nest2", c, false);
    int64_t nodes[1] = { 3 };
    nest2->compileAndRun(LOC, 2, 1, nodes);
    delete nest2;
}
TEST(omrgenExtension, NestedControlFlow3L) {
    NestedControlFlowFunc *nest3l = new (c->mem()) NestedControlFlowFunc(MEM_LOC(c->mem()), "nest3l", c, false);
    int64_t nodes[2] = { 3, 7 };
    nest3l->compileAndRun(LOC, 3, 2, nodes);
    delete nest3l;
}
TEST(omrgenExtension, NestedControlFlow3R) {
    NestedControlFlowFunc *nest3r = new (c->mem()) NestedControlFlowFunc(MEM_LOC(c->mem()), "nest3r", c, false);
    int64_t nodes[2] = { 6, 7 };
    nest3r->compileAndRun(LOC, 3, 2, nodes);
    delete nest3r;
}
TEST(omrgenExtension, NestedControlFlow6) {
    NestedControlFlowFunc *nest6 = new (c->mem()) NestedControlFlowFunc(MEM_LOC(c->mem()), "nest6", c, false);
    int64_t nodes[5] = { 6, 14, 48, 62, 63 };
    nest6->compileAndRun(LOC, 6, 5, nodes);
    delete nest6;
}


// Tests for StoreFieldAt
template<typename struct_cType, typename field_cType>
class StoreFieldAtTestFunc : public TestFunc {
public:
    typedef void (FuncPrototype)(struct_cType *, field_cType);
    StoreFieldAtTestFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : TestFunc(MEM_PASSLOC(a), compiler, name, __FILE__, LINETOSTR(__LINE__), log)
        , _ftid(NoTypeID)
        , _ft(NULL)
        , _structType(NULL)
        , _fieldType(NULL)
        , _fieldValue(0)
        , _notFieldValue(0) {
    }
    void run(LOCATION) {
        FuncPrototype *f = body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        struct_cType _struct;
        _struct._value = _notFieldValue;
        f(&_struct, _fieldValue);
        field_cType v = _struct._value;
        EXPECT_EQ(v, _fieldValue) << "Compiled f(struct *) sets _value to " << v << " (expected " << _fieldValue << ")";
    }
    void test(LOCATION, bool doCompile, const TypeID ftid, field_cType fieldValue, field_cType notFieldValue) {
        _ftid = ftid;
        _fieldValue = fieldValue;
        _notFieldValue = notFieldValue;
        if (doCompile)
            compile(PASSLOC);
        run(PASSLOC);
    }

protected:
    virtual bool buildContext(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        _ft = comp->ir()->typedict()->Lookup(_ftid);
        _structType = defineStruct(comp, _ft);
        _fieldType = _structType->LookupField("_value");
        assert(_fieldType != NULL);
        ctx->DefineParameter("theStruct", this->bx()->PointerTo(PASSLOC, _structType));
        ctx->DefineParameter("value", _fieldType->type());
        ctx->DefineReturnType(comp->ir()->NoType);
        return true;
    }
    virtual bool buildIL(LOCATION, Func::FunctionCompilation *comp, Func::FunctionScope *scope, Func::FunctionContext *ctx) {
        Builder *entry = scope->entryPoint<BuilderEntry>(0)->builder();
        auto structSym=ctx->LookupLocal("theStruct"); 
        Value *structBase = fx()->Load(LOC, entry, structSym);
        auto valueSym = ctx->LookupLocal("value");
        Value *value = fx()->Load(LOC, entry, valueSym);
        bx()->StoreFieldAt(LOC, entry, _fieldType, structBase, value);
        fx()->Return(LOC, entry);
        return true;
    }

protected:
    virtual const Base::StructType * defineStruct(Func::FunctionCompilation *comp, const Type *ft) = 0;

    TypeID _ftid;
    const Type * _ft;
    const Base::StructType *_structType;
    const Base::FieldType *_fieldType;
    field_cType _fieldValue;
    field_cType _notFieldValue;
};

template<typename struct_cType, typename field_cType>
class StoreFieldAtFunc : public StoreFieldAtTestFunc<struct_cType, field_cType> {
public:
    StoreFieldAtFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : StoreFieldAtTestFunc<struct_cType, field_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual void addFieldsBefore(Func::FunctionCompilation *comp, Base::StructTypeBuilder *stb) { }
    virtual void addFieldsAfter(Func::FunctionCompilation *comp, Base::StructTypeBuilder *stb) { }
    virtual const Base::StructType * defineStruct(Func::FunctionCompilation *comp, const Type *ft) {
        Base::StructTypeBuilder stb(this->bx(), comp);
        stb.setName("TheStruct");
        addFieldsBefore(comp, &stb);
        stb.addField("_value", ft, offsetof(struct_cType, _value));
        addFieldsAfter(comp, &stb);
        return stb.create(LOC);
    }
};

TEST(omrgenExtension, StoreFieldFromStructInt8) {
    typedef struct SingleInt8Struct { int8_t _value; } SingleInt8Struct;
    typedef StoreFieldAtFunc<SingleInt8Struct, int8_t> StoreFieldAtSingleInt8Func;
    StoreFieldAtSingleInt8Func *storesingle_int8 = new (c->mem()) StoreFieldAtSingleInt8Func(MEM_LOC(c->mem()), "storesingle_int8", c, false);
    storesingle_int8->test(LOC, true, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(15));
    storesingle_int8->test(LOC, false, tInt8, static_cast<int8_t>(3), static_cast<int8_t>(15));
    storesingle_int8->test(LOC, false, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(15));
    storesingle_int8->test(LOC, false, tInt8, std::numeric_limits<int8_t>::min(), static_cast<int8_t>(15));
    storesingle_int8->test(LOC, false, tInt8, std::numeric_limits<int8_t>::max(), static_cast<int8_t>(15));
    delete storesingle_int8;
}
TEST(omrgenExtension, StoreFieldFromStructInt16) {
    typedef struct SingleInt16Struct { int16_t _value; } SingleInt16Struct;
    typedef StoreFieldAtFunc<SingleInt16Struct, int16_t> StoreFieldAtSingleInt16Func;
    StoreFieldAtSingleInt16Func *storesingle_int16 = new (c->mem()) StoreFieldAtSingleInt16Func(MEM_LOC(c->mem()), "storesingle_int16", c, false);
    storesingle_int16->test(LOC, true, tInt16, static_cast<int16_t>(0), static_cast<int16_t>(15));
    storesingle_int16->test(LOC, false, tInt16, static_cast<int16_t>(3), static_cast<int16_t>(15));
    storesingle_int16->test(LOC, false, tInt16, static_cast<int16_t>(-1), static_cast<int16_t>(15));
    storesingle_int16->test(LOC, false, tInt16, std::numeric_limits<int16_t>::min(), static_cast<int16_t>(15));
    storesingle_int16->test(LOC, false, tInt16, std::numeric_limits<int16_t>::max(), static_cast<int16_t>(15));
    delete storesingle_int16;
}
TEST(omrgenExtension, StoreFieldFromStructInt32) {
    typedef struct SingleInt32Struct { int32_t _value; } SingleInt32Struct;
    typedef StoreFieldAtFunc<SingleInt32Struct, int32_t> StoreFieldAtSingleInt32Func;
    StoreFieldAtSingleInt32Func *storesingle_int32 = new (c->mem()) StoreFieldAtSingleInt32Func(MEM_LOC(c->mem()), "storesingle_int32", c, false);
    storesingle_int32->test(LOC, true, tInt32, static_cast<int32_t>(0), static_cast<int32_t>(15));
    storesingle_int32->test(LOC, false, tInt32, static_cast<int32_t>(3), static_cast<int32_t>(15));
    storesingle_int32->test(LOC, false, tInt32, static_cast<int32_t>(-1), static_cast<int32_t>(15));
    storesingle_int32->test(LOC, false, tInt32, std::numeric_limits<int32_t>::min(), static_cast<int32_t>(15));
    storesingle_int32->test(LOC, false, tInt32, std::numeric_limits<int32_t>::max(), static_cast<int32_t>(15));
    delete storesingle_int32;
}
TEST(omrgenExtension, StoreFieldFromStructInt64) {
    typedef struct SingleInt64Struct { int64_t _value; } SingleInt64Struct;
    typedef StoreFieldAtFunc<SingleInt64Struct, int64_t> StoreFieldAtSingleInt64Func;
    StoreFieldAtSingleInt64Func *storesingle_int64 = new (c->mem()) StoreFieldAtSingleInt64Func(MEM_LOC(c->mem()), "storesingle_int64", c, false);
    storesingle_int64->test(LOC, true, tInt64, static_cast<int64_t>(0), static_cast<int64_t>(15));
    storesingle_int64->test(LOC, false, tInt64, static_cast<int64_t>(3), static_cast<int64_t>(15));
    storesingle_int64->test(LOC, false, tInt64, static_cast<int64_t>(-1), static_cast<int64_t>(15));
    storesingle_int64->test(LOC, false, tInt64, std::numeric_limits<int64_t>::min(), static_cast<int64_t>(15));
    storesingle_int64->test(LOC, false, tInt64, std::numeric_limits<int64_t>::max(), static_cast<int64_t>(15));
    delete storesingle_int64;
}
TEST(omrgenExtension, StoreFieldFromStructFloat32) {
    typedef struct SingleFloat32Struct { float _value; } SingleFloat32Struct;
    typedef StoreFieldAtFunc<SingleFloat32Struct, float> StoreFieldAtSingleFloat32Func;
    StoreFieldAtSingleFloat32Func *storesingle_float32 = new (c->mem()) StoreFieldAtSingleFloat32Func(MEM_LOC(c->mem()), "storesingle_float32", c, false);
    storesingle_float32->test(LOC, true, tFloat32, static_cast<float>(0), static_cast<float>(15));
    storesingle_float32->test(LOC, false, tFloat32, static_cast<float>(3), static_cast<float>(15));
    storesingle_float32->test(LOC, false, tFloat32, static_cast<float>(-1), static_cast<float>(15));
    storesingle_float32->test(LOC, false, tFloat32, std::numeric_limits<float>::min(), static_cast<float>(15));
    storesingle_float32->test(LOC, false, tFloat32, std::numeric_limits<float>::max(), static_cast<float>(15));
    delete storesingle_float32;
}
TEST(omrgenExtension, StoreFieldFromStructFloat64) {
    typedef struct SingleFloat64Struct { double _value; } SingleFloat64Struct;
    typedef StoreFieldAtFunc<SingleFloat64Struct, double> StoreFieldAtSingleFloat64Func;
    StoreFieldAtSingleFloat64Func *storesingle_float64 = new (c->mem()) StoreFieldAtSingleFloat64Func(MEM_LOC(c->mem()), "storesingle_float64", c, false);
    storesingle_float64->test(LOC, true, tFloat64, static_cast<double>(0), static_cast<double>(15));
    storesingle_float64->test(LOC, false, tFloat64, static_cast<double>(3), static_cast<double>(15));
    storesingle_float64->test(LOC, false, tFloat64, static_cast<double>(-1), static_cast<double>(15));
    storesingle_float64->test(LOC, false, tFloat64, std::numeric_limits<double>::min(), static_cast<double>(15));
    storesingle_float64->test(LOC, false, tFloat64, std::numeric_limits<double>::max(), static_cast<double>(15));
    delete storesingle_float64;
}
TEST(omrgenExtension, StoreFieldFromStructAddress) {
    typedef struct SingleAddressStruct { void * _value; } SingleAddressStruct;
    typedef StoreFieldAtFunc<SingleAddressStruct, void *> StoreFieldAtSingleAddressFunc;
    StoreFieldAtSingleAddressFunc *storesingle_address = new (c->mem()) StoreFieldAtSingleAddressFunc(MEM_LOC(c->mem()), "storesingle_address", c, false);
    storesingle_address->test(LOC, true, tAddress, static_cast<void *>(0), reinterpret_cast<void *>(15));
    storesingle_address->test(LOC, false, tAddress, reinterpret_cast<void *>(3), reinterpret_cast<void *>(15));
    storesingle_address->test(LOC, false, tAddress, std::numeric_limits<void *>::min(), reinterpret_cast<void *>(15));
    storesingle_address->test(LOC, false, tAddress, std::numeric_limits<void *>::max(), reinterpret_cast<void *>(15));
    delete storesingle_address;
}

template<typename struct_cType, typename field_cType>
class StoreFieldAtBeforeAfterFunc : public StoreFieldAtFunc<struct_cType, field_cType> {
    typedef void (FuncPrototype)(struct_cType *, field_cType);
public:
    StoreFieldAtBeforeAfterFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : StoreFieldAtFunc<struct_cType, field_cType>(MEM_PASSLOC(a), name, compiler, log) { }
    virtual void run(LOCATION) {
        FuncPrototype *f = this->body()->template nativeEntryPoint<FuncPrototype>();
        EXPECT_NE(f, nullptr);
        struct_cType _struct;
        _struct._pad1 = static_cast<int64_t>(-1);
        _struct._pad2 = static_cast<int32_t>(-1);
        _struct._value = this->_notFieldValue;
        _struct._pad3 = static_cast<double>(-1);
        _struct._pad4 = static_cast<float>(-1);
        f(&_struct, this->_fieldValue);
        field_cType v = _struct._value;
        EXPECT_EQ(v, this->_fieldValue) << "Compiled f(struct *) sets _value as " << v << " (expected " << this->_fieldValue << ")";
        EXPECT_EQ(_struct._pad1, static_cast<int64_t>(-1)) << "Compiled f(struct *) did not change _pad1" << _struct._pad1 << " (expected -1)";
        EXPECT_EQ(_struct._pad2, static_cast<int32_t>(-1)) << "Compiled f(struct *) did not change _pad2" << _struct._pad2 << " (expected -1)";
        EXPECT_EQ(_struct._pad3, static_cast<double>(-1)) << "Compiled f(struct *) did not change _pad3" << _struct._pad3 << " (expected -1)";
        EXPECT_EQ(_struct._pad3, static_cast<float>(-1)) << "Compiled f(struct *) did not change _pad4" << _struct._pad4 << " (expected -1)";
    }
protected:
    virtual void addFieldsBefore(Func::FunctionCompilation *comp, Base::StructTypeBuilder *stb) {
        stb->addField("_pad1", this->bx()->Int64(comp->ir()), offsetof(struct_cType, _pad1));
        stb->addField("_pad2", this->bx()->Int32(comp->ir()), offsetof(struct_cType, _pad2));
     }
    virtual void addFieldsAfter(Func::FunctionCompilation *comp, Base::StructTypeBuilder *stb) {
        stb->addField("_pad3", this->bx()->Float64(comp->ir()), offsetof(struct_cType, _pad3));
        stb->addField("_pad4", this->bx()->Float32(comp->ir()), offsetof(struct_cType, _pad3));
     }
};

TEST(omrgenExtension, StoreFieldFromStructPaddedInt8) {
    typedef struct PaddedInt8Struct { int64_t _pad1; int32_t _pad2; int8_t _value; double _pad3; float _pad4; } SingleInt8Struct;
    typedef StoreFieldAtBeforeAfterFunc<PaddedInt8Struct, int8_t> StoreFieldAtBeforeAfterInt8Func;
    StoreFieldAtBeforeAfterInt8Func *storepadded_int8 = new (c->mem()) StoreFieldAtBeforeAfterInt8Func(MEM_LOC(c->mem()), "storepadded_int8", c, false);
    storepadded_int8->test(LOC, true, tInt8, static_cast<int8_t>(0), static_cast<int8_t>(15));
    storepadded_int8->test(LOC, false, tInt8, static_cast<int8_t>(3), static_cast<int8_t>(15));
    storepadded_int8->test(LOC, false, tInt8, static_cast<int8_t>(-1), static_cast<int8_t>(15));
    storepadded_int8->test(LOC, false, tInt8, std::numeric_limits<int8_t>::min(), static_cast<int8_t>(15));
    storepadded_int8->test(LOC, false, tInt8, std::numeric_limits<int8_t>::max(), static_cast<int8_t>(15));
    delete storepadded_int8;
}
TEST(omrgenExtension, StoreFieldFromStructPaddedInt16) {
    typedef struct PaddedInt16Struct { int64_t _pad1; int32_t _pad2; int16_t _value; double _pad3; float _pad4; } SingleInt16Struct;
    typedef StoreFieldAtBeforeAfterFunc<PaddedInt16Struct, int16_t> StoreFieldAtBeforeAfterInt16Func;
    StoreFieldAtBeforeAfterInt16Func *storepadded_int16 = new (c->mem()) StoreFieldAtBeforeAfterInt16Func(MEM_LOC(c->mem()), "storepadded_int16", c, false);
    storepadded_int16->test(LOC, true, tInt16, static_cast<int16_t>(0), static_cast<int8_t>(15));
    storepadded_int16->test(LOC, false, tInt16, static_cast<int16_t>(3), static_cast<int8_t>(15));
    storepadded_int16->test(LOC, false, tInt16, static_cast<int16_t>(-1), static_cast<int8_t>(15));
    storepadded_int16->test(LOC, false, tInt16, std::numeric_limits<int16_t>::min(), static_cast<int8_t>(15));
    storepadded_int16->test(LOC, false, tInt16, std::numeric_limits<int16_t>::max(), static_cast<int8_t>(15));
    delete storepadded_int16;
}
TEST(omrgenExtension, StoreFieldFromStructPaddedInt32) {
    typedef struct PaddedInt32Struct { int64_t _pad1; int32_t _pad2; int32_t _value; double _pad3; float _pad4; } SingleInt32Struct;
    typedef StoreFieldAtBeforeAfterFunc<PaddedInt32Struct, int32_t> StoreFieldAtBeforeAfterInt32Func;
    StoreFieldAtBeforeAfterInt32Func *storepadded_int32 = new (c->mem()) StoreFieldAtBeforeAfterInt32Func(MEM_LOC(c->mem()), "storepadded_int32", c, false);
    storepadded_int32->test(LOC, true, tInt32, static_cast<int32_t>(0), static_cast<int8_t>(15));
    storepadded_int32->test(LOC, false, tInt32, static_cast<int32_t>(3), static_cast<int8_t>(15));
    storepadded_int32->test(LOC, false, tInt32, static_cast<int32_t>(-1), static_cast<int8_t>(15));
    storepadded_int32->test(LOC, false, tInt32, std::numeric_limits<int32_t>::min(), static_cast<int8_t>(15));
    storepadded_int32->test(LOC, false, tInt32, std::numeric_limits<int32_t>::max(), static_cast<int8_t>(15));
    delete storepadded_int32;
}
TEST(omrgenExtension, StoreFieldFromStructPaddedInt64) {
    typedef struct PaddedInt64Struct { int64_t _pad1; int32_t _pad2; int64_t _value; double _pad3; float _pad4; } SingleInt64Struct;
    typedef StoreFieldAtBeforeAfterFunc<PaddedInt64Struct, int64_t> StoreFieldAtBeforeAfterInt64Func;
    StoreFieldAtBeforeAfterInt64Func *storepadded_int64 = new (c->mem()) StoreFieldAtBeforeAfterInt64Func(MEM_LOC(c->mem()), "storepadded_int64", c, false);
    storepadded_int64->test(LOC, true, tInt64, static_cast<int64_t>(0), static_cast<int8_t>(15));
    storepadded_int64->test(LOC, false, tInt64, static_cast<int64_t>(3), static_cast<int8_t>(15));
    storepadded_int64->test(LOC, false, tInt64, static_cast<int64_t>(-1), static_cast<int8_t>(15));
    storepadded_int64->test(LOC, false, tInt64, std::numeric_limits<int64_t>::min(), static_cast<int8_t>(15));
    storepadded_int64->test(LOC, false, tInt64, std::numeric_limits<int64_t>::max(), static_cast<int64_t>(15));
    delete storepadded_int64;
}
TEST(omrgenExtension, StoreFieldFromStructPaddedFloat32) {
    typedef struct PaddedFloat32Struct { int64_t _pad1; int32_t _pad2; float _value; double _pad3; float _pad4; } SingleFloat32Struct;
    typedef StoreFieldAtBeforeAfterFunc<PaddedFloat32Struct, float> StoreFieldAtBeforeAfterFloat32Func;
    StoreFieldAtBeforeAfterFloat32Func *storepadded_float32 = new (c->mem()) StoreFieldAtBeforeAfterFloat32Func(MEM_LOC(c->mem()), "storepadded_float32", c, false);
    storepadded_float32->test(LOC, true, tFloat32, static_cast<float>(0), static_cast<float>(15));
    storepadded_float32->test(LOC, false, tFloat32, static_cast<float>(3), static_cast<float>(15));
    storepadded_float32->test(LOC, false, tFloat32, static_cast<float>(-1), static_cast<float>(15));
    storepadded_float32->test(LOC, false, tFloat32, std::numeric_limits<float>::min(), static_cast<float>(15));
    storepadded_float32->test(LOC, false, tFloat32, std::numeric_limits<float>::max(), static_cast<float>(15));
    delete storepadded_float32;
}
TEST(omrgenExtension, StoreFieldFromStructPaddedFloat64) {
    typedef struct PaddedFloat64Struct { int64_t _pad1; int32_t _pad2; double _value; double _pad3; float _pad4; } SingleFloat64Struct;
    typedef StoreFieldAtBeforeAfterFunc<PaddedFloat64Struct, double> StoreFieldAtBeforeAfterFloat64Func;
    StoreFieldAtBeforeAfterFloat64Func *storepadded_float64 = new (c->mem()) StoreFieldAtBeforeAfterFloat64Func(MEM_LOC(c->mem()), "storepadded_float64", c, false);
    storepadded_float64->test(LOC, true, tFloat64, static_cast<double>(0), static_cast<double>(15));
    storepadded_float64->test(LOC, false, tFloat64, static_cast<double>(3), static_cast<double>(15));
    storepadded_float64->test(LOC, false, tFloat64, static_cast<double>(-1), static_cast<double>(15));
    storepadded_float64->test(LOC, false, tFloat64, std::numeric_limits<double>::min(), static_cast<double>(15));
    storepadded_float64->test(LOC, false, tFloat64, std::numeric_limits<double>::max(), static_cast<double>(15));
    delete storepadded_float64;
}
TEST(omrgenExtension, StoreFieldFromStructPaddedAddress) {
    typedef struct PaddedAddressStruct { int64_t _pad1; int32_t _pad2; void * _value; double _pad3; float _pad4; } SingleAddressStruct;
    typedef StoreFieldAtBeforeAfterFunc<PaddedAddressStruct, void *> StoreFieldAtBeforeAfterAddressFunc;
    StoreFieldAtBeforeAfterAddressFunc *storepadded_address = new (c->mem()) StoreFieldAtBeforeAfterAddressFunc(MEM_LOC(c->mem()), "storepadded_address", c, false);
    storepadded_address->test(LOC, true, tAddress, static_cast<void *>(0), reinterpret_cast<void *>(15));
    storepadded_address->test(LOC, false, tAddress, reinterpret_cast<void *>(3), reinterpret_cast<void *>(15));
    storepadded_address->test(LOC, false, tAddress, std::numeric_limits<void *>::min(), reinterpret_cast<void *>(15));
    storepadded_address->test(LOC, false, tAddress, std::numeric_limits<void *>::max(), reinterpret_cast<void *>(15));
    delete storepadded_address;
}


// Test subtracting two numbers of the given types
template<typename FuncPrototype, typename left_cType, typename right_cType, typename result_cType>
class SubFunc : public BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType> {
public:
    SubFunc(MEM_LOCATION(a), String name, Compiler *compiler, bool log)
        : BinaryOpFunc<FuncPrototype, left_cType, right_cType, result_cType>(MEM_PASSLOC(a), name, compiler, log) { }
protected:
    virtual Value *doBinaryOp(LOCATION, Builder *b, Value *left, Value *right) {
        return this->bx()->Sub(PASSLOC, b, left, right);
    }
};

TEST(omrgenExtension, SubInt8s) {
    typedef int8_t (FuncProto)(int8_t, int8_t);
    SubFunc<FuncProto, int8_t, int8_t, int8_t> *sub_int8s = new (c->mem()) SubFunc<FuncProto,int8_t,int8_t,int8_t>(MEM_LOC(c->mem()), "sub_int8s", c, false);
    sub_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(0));
    sub_int8s->test(LOC, tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(-3));
    sub_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(0), tInt8, static_cast<int8_t>(3));
    sub_int8s->test(LOC, tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(-6));
    sub_int8s->test(LOC, tInt8, static_cast<int8_t>(3), tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(6));
    sub_int8s->test(LOC, tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(-3), tInt8, static_cast<int8_t>(0));
    sub_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::min());
    sub_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(0), tInt8, std::numeric_limits<int8_t>::max());
    sub_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::min(), tInt8, static_cast<int8_t>(-1), tInt8, std::numeric_limits<int8_t>::min()+1);
    sub_int8s->test(LOC, tInt8, std::numeric_limits<int8_t>::max(), tInt8, static_cast<int8_t>(1), tInt8, std::numeric_limits<int8_t>::max()-1);
    delete sub_int8s;
}
TEST(omrgenExtension, SubInt16s) {
    typedef int16_t (FuncProto)(int16_t, int16_t);
    SubFunc<FuncProto, int16_t, int16_t, int16_t> *sub_int16s = new (c->mem()) SubFunc<FuncProto,int16_t,int16_t,int16_t>(MEM_LOC(c->mem()), "sub_int16s", c, false);
    sub_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(0));
    sub_int16s->test(LOC, tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(-3));
    sub_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(0), tInt16, static_cast<int16_t>(3));
    sub_int16s->test(LOC, tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(-6));
    sub_int16s->test(LOC, tInt16, static_cast<int16_t>(3), tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(6));
    sub_int16s->test(LOC, tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(-3), tInt16, static_cast<int16_t>(0));
    sub_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::min());
    sub_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(0), tInt16, std::numeric_limits<int16_t>::max());
    sub_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::min(), tInt16, static_cast<int16_t>(-1), tInt16, std::numeric_limits<int16_t>::min()+1);
    sub_int16s->test(LOC, tInt16, std::numeric_limits<int16_t>::max(), tInt16, static_cast<int16_t>(1), tInt16, std::numeric_limits<int16_t>::max()-1);
    delete sub_int16s;
}
TEST(omrgenExtension, SubInt32s) {
    typedef int32_t (FuncProto)(int32_t, int32_t);
    SubFunc<FuncProto, int32_t, int32_t, int32_t> *sub_int32s = new (c->mem()) SubFunc<FuncProto,int32_t,int32_t,int32_t>(MEM_LOC(c->mem()), "sub_int32s", c, false);
    sub_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(0));
    sub_int32s->test(LOC, tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(-3));
    sub_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(0), tInt32, static_cast<int32_t>(3));
    sub_int32s->test(LOC, tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(-6));
    sub_int32s->test(LOC, tInt32, static_cast<int32_t>(3), tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(6));
    sub_int32s->test(LOC, tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(-3), tInt32, static_cast<int32_t>(0));
    sub_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::min());
    sub_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(0), tInt32, std::numeric_limits<int32_t>::max());
    sub_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::min(), tInt32, static_cast<int32_t>(-1), tInt32, std::numeric_limits<int32_t>::min()+1);
    sub_int32s->test(LOC, tInt32, std::numeric_limits<int32_t>::max(), tInt32, static_cast<int32_t>(1), tInt32, std::numeric_limits<int32_t>::max()-1);
    delete sub_int32s;
}
TEST(omrgenExtension, SubInt64s) {
    typedef int64_t (FuncProto)(int64_t, int64_t);
    SubFunc<FuncProto, int64_t, int64_t, int64_t> *sub_int64s = new (c->mem()) SubFunc<FuncProto,int64_t,int64_t,int64_t>(MEM_LOC(c->mem()), "sub_int64s", c, false);
    sub_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(0));
    sub_int64s->test(LOC, tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(-3));
    sub_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(0), tInt64, static_cast<int64_t>(3));
    sub_int64s->test(LOC, tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(-6));
    sub_int64s->test(LOC, tInt64, static_cast<int64_t>(3), tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(6));
    sub_int64s->test(LOC, tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(-3), tInt64, static_cast<int64_t>(0));
    sub_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::min());
    sub_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(0), tInt64, std::numeric_limits<int64_t>::max());
    sub_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::min(), tInt64, static_cast<int64_t>(-1), tInt64, std::numeric_limits<int64_t>::min()+1);
    sub_int64s->test(LOC, tInt64, std::numeric_limits<int64_t>::max(), tInt64, static_cast<int64_t>(1), tInt64, std::numeric_limits<int64_t>::max()-1);
    delete sub_int64s;
}
TEST(omrgenExtension, SubFloat32s) {
    typedef float (FuncProto)(float, float);
    SubFunc<FuncProto, float, float, float> *sub_float32s = new (c->mem()) SubFunc<FuncProto,float,float,float>(MEM_LOC(c->mem()), "sub_float32s", c, false);
    sub_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(0));
    sub_float32s->test(LOC, tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(-3));
    sub_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(0), tFloat32, static_cast<float>(3));
    sub_float32s->test(LOC, tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(-6));
    sub_float32s->test(LOC, tFloat32, static_cast<float>(3), tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(6));
    sub_float32s->test(LOC, tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(-3), tFloat32, static_cast<float>(0));
    sub_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::min());
    sub_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, static_cast<float>(0), tFloat32, std::numeric_limits<float>::max());
    sub_float32s->test(LOC, tFloat32, std::numeric_limits<float>::min(), tFloat32, static_cast<float>(-1), tFloat32, std::numeric_limits<float>::min()+1);
    sub_float32s->test(LOC, tFloat32, std::numeric_limits<float>::max(), tFloat32, static_cast<float>(1), tFloat32, std::numeric_limits<float>::max()-1);
    delete sub_float32s;
}
TEST(omrgenExtension, SubFloat64s) {
    typedef double (FuncProto)(double, double);
    SubFunc<FuncProto, double, double, double> *sub_float64s = new (c->mem()) SubFunc<FuncProto,double,double,double>(MEM_LOC(c->mem()), "sub_float64s", c, false);
    sub_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(0));
    sub_float64s->test(LOC, tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(-3));
    sub_float64s->test(LOC, tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(0), tFloat64, static_cast<double>(3));
    sub_float64s->test(LOC, tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(-6));
    sub_float64s->test(LOC, tFloat64, static_cast<double>(3), tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(6));
    sub_float64s->test(LOC, tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(-3), tFloat64, static_cast<double>(0));
    sub_float64s->test(LOC, tFloat64, std::numeric_limits<double>::min(), tFloat64, static_cast<double>(0), tFloat64, std::numeric_limits<double>::min());
    sub_float64s->test(LOC, tFloat64, std::numeric_limits<double>::max(), tFloat64, static_cast<double>(0), tFloat64, std::numeric_limits<double>::max());
    sub_float64s->test(LOC, tFloat64, std::numeric_limits<double>::min(), tFloat64, static_cast<double>(-1), tFloat64, std::numeric_limits<double>::min()+1);
    sub_float64s->test(LOC, tFloat64, std::numeric_limits<double>::max(), tFloat64, static_cast<double>(1), tFloat64, std::numeric_limits<double>::max()-1);
    delete sub_float64s;
}
TEST(omrgenExtension, SubAddressAndInt) {
    if (c->platformWordSize() == 32) {
        typedef intptr_t (FuncProto)(intptr_t, int32_t);
        SubFunc<FuncProto, intptr_t, int32_t, intptr_t> *sub_addressint32s = new (c->mem()) SubFunc<FuncProto,intptr_t,int32_t,intptr_t>(MEM_LOC(c->mem()), "sub_addressint32s", c, false);
        sub_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(NULL), tInt32, static_cast<int32_t>(0), tAddress, static_cast<intptr_t>(NULL));
        sub_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(NULL), tInt32, static_cast<int32_t>(4), tAddress, static_cast<intptr_t>(-4));
        sub_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(0xdeadbeef), tInt32, static_cast<int32_t>(0), tAddress, static_cast<intptr_t>(0xdeadbeef));
        sub_addressint32s->test(LOC, tAddress, static_cast<intptr_t>(0xdeadbeef), tInt32, static_cast<int32_t>(16), tAddress, static_cast<intptr_t>(0xdeadbeef)-16);
        sub_addressint32s->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tInt32, static_cast<int32_t>(0), tAddress, std::numeric_limits<intptr_t>::max());
        sub_addressint32s->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), tInt32, static_cast<int32_t>(0), tAddress, std::numeric_limits<intptr_t>::min());
        delete sub_addressint32s;
    } else if (c->platformWordSize() == 64) {
        typedef intptr_t (FuncProto)(intptr_t, int64_t);
        SubFunc<FuncProto, intptr_t, int64_t, intptr_t> *sub_addressint64s = new (c->mem()) SubFunc<FuncProto,intptr_t,int64_t,intptr_t>(MEM_LOC(c->mem()), "sub_addressint64s", c, false);
        sub_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(NULL), tInt64, static_cast<int64_t>(0), tAddress, static_cast<intptr_t>(NULL));
        sub_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(NULL), tInt64, static_cast<int64_t>(4), tAddress, static_cast<intptr_t>(-4));
        sub_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(0xdeadbeefdeadbeef), tInt64, static_cast<int64_t>(0), tAddress, static_cast<intptr_t>(0xdeadbeefdeadbeef));
        sub_addressint64s->test(LOC, tAddress, static_cast<intptr_t>(0xdeadbeefdeadbeef), tInt64, static_cast<int64_t>(16), tAddress, static_cast<intptr_t>(0xdeadbeefdeadbeef)-16);
        sub_addressint64s->test(LOC, tAddress, std::numeric_limits<intptr_t>::max(), tInt64, static_cast<int64_t>(0), tAddress, std::numeric_limits<intptr_t>::max());
        sub_addressint64s->test(LOC, tAddress, std::numeric_limits<intptr_t>::min(), tInt64, static_cast<int64_t>(0), tAddress, std::numeric_limits<intptr_t>::min());
        delete sub_addressint64s;
    }
}

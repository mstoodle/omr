<!--
Copyright (c) 2022, 2022 IBM Corp. and others

This program and the accompanying materials are made available under
the terms of the Eclipse Public License 2.0 which accompanies this
distribution and is available at https://www.eclipse.org/legal/epl-2.0/
or the Apache License, Version 2.0 which accompanies this distribution and
is available at https://www.apache.org/licenses/LICENSE-2.0.

This Source Code may also be made available under the following
Secondary Licenses when the conditions for such availability set
forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
General Public License, version 2 with the GNU Classpath 
Exception [1] and GNU General Public License, version 2 with the
OpenJDK Assembly Exception [2].

[1] https://www.gnu.org/software/classpath/license.html
[2] http://openjdk.java.net/legal/assembly-exception.html

SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
-->

Welcome to JB2 (a.k.a JitBuilder 2.0) !!

JB2 is an extensible C++ based compiler infrastructure designed to facilitate the construction
of JIT (Just In Time) compilers dynamically, even supporting the construction of many different
JIT compilers workflows within the same process.

Looking to build JB2? See (BuildingJB2.md)[BuildingJB2.md].

# Core Elements

At a high level, JB2 introduces a few core elements to enable JIT compilers to be built at
runtime. Many of these will be familiar if you have built a compiler before, or if you have
used the JitBuilder library :

* `Compiler` -	An object representing an entire JIT compiler. See (Compiler.md)[Compiler.md].

* `Config` -	An object representing configuration data for a `Compiler` or a `Compilation`.
			See (Compilation.md)[Compilation.md].

* `Compilation` -	An object representing a compilation unit such as a function. See (Compilation.md)[Compilation.md].

* `Extension` -	An object representing pluggable implementation for a compiler instance, including
			IL constructs like `Builder`s, `Operation`s, `Type`s, `Symbol`s, etc. See (Extension.md)[Extension.md].

* `Builder` -	An object representing a sequence of code in its own scope. See (Builder.md)[Builder.md]

* `Operation` -	An object representing a basic instruction in a compiler's IL. `Operation`s are loaded
			into a `Compiler` via an `Extension`. See (Operation.md)[Operation.md].

* `Value` -	An object representing an argument or result of an `Operation`, must have a `Type`.
			Each `Value` is defined by some number of `Operation`s, most often a single `Operation`.
			Multiple-definition `Value` can be produced by using the `MergeDef` service in `Builder`.
			See (Value.md)[Value.md].

* `Type` -	An object representing a category of possible data values that can be held by a `Value`.
			`Type`s can be loaded into a `Compiler` via an `Extension`. `Type`s are maintained in
			`TypeDictionary`s and can be referenced by multiple `Operation`s and other `Type`s.
			See (Type.md)[Type.md].

* `TypeDictionary` - An object that records a set of `Type`s. Each `Compiler` maintains a set of `Type`s that
			can be used in any of that `Compiler`'s `Compilation`s. Each `Compilation` also maintains
			its own `TypeDictionary` object that holds `Type`s used by that `Compilation`. See
			(TypeDictionary.md)[TypeDictionary.md].

* `Literal` -	An object representing a particular data value of a particular `Type`, sometimes known as
			a "constant".. A `Literal` must have a `Type`. `Literal`s are kept in a `LiteralDictionary`
			maintained by each `Compilation` object and can be referenced by multiple `Operation`s.
			See (Literal.md)[Literal.md].

* `LiteralDictionary` - An object representing a set of `Literal`s associated with a `Compilation`. See
			(LitarlDictionary.md)[LiteralDictionary.md].

* `Symbol` -	An object representing a named symbol, such as a local variable or a function parameter.
			`Symbol` categories can be loaded into a `Compiler` via an `Extension. `Symbol`s are
			kept in a `SymbolDictionary` maintained by each `Compilation` object and can be referenced
			by multiple `Operation`s. See (Symbol.md)[Symbol.md).

* `SymbolDictionary` - An object representing a set of `Symbol`s associated with a `Compilation`. See
			(SymbolDictionary.md)[SymbolDictionary.md].


* `Location` -	An object representing a source program location. By default, a `Location` holds a filename,
			a line number, and a bytecode index. Every `Builder` maintains a current `Location` and
			every `Operation` added to a `Builder` will be associated with the current `Location` for
			source tracking. See (Location.md)[Location.md].

* `Strategy` -	An object representing a sequence of `Pass`es that will be performed as part of a compilation.
			`Strategy`s can be loaded into a `Compiler` via an `Extension` and can be requested when
			a `Compilation` is compiled. See (Strategy.md)[Strategy.md].

* `Pass` -	An object representing one step in a `Strategy`. `Pass`es can be loaded into a `Compiler`
			via an `Extension` and can be assembled into `Strategy`s. See (Pass.md)[Pass.md].

* `CreateLocation` -	An object representing a source code location in a compiler itself, specifying
			the file name, line number, and function name where some JB2 services are called. For example
			every `Operation`, `Literal`, `Type`, and `Symbol` records the create location where these
			instances are instantiated. Used primarily for error reporting when invalid IL is detected.
			See (CreateLocation.md)[CreateLocation.md].

* `SemanticVersion` -	An object representing a (semantic version)[] used to track version changes for
			JB2 and for `Extension`s. When loading an `Extension` into a `Compiler`, a specific
			`SemanticVersion` can be requested or a compatible version can be required. See
			(SemanticVersion.md)[SemanticVersion.md].


# Basic Facilities

JB2 defines a few basic facilities that can help to build more interesting compilers. This list is always growing:

* `TextWriter` -	writes a textual format for JB2 to the screen and is used for logging.
			See (TextWriter.md)[TextWriter.md].

* `KindService` - used as a form of run time type info for identifying and casting to individual
			classes across generic contexts. Used e.g. in `Type`, `Symbol`, `Extension`. See
			(KindService.md)[KindService.md].

* `Visitor` -	simple Visitor pattern, subclass to walk over IL. Used e.g. in `TextWriter`,
			`JB1CodeGenerator`. See (Visitor.md)(Visitor.md].

* `Transformer` -	implementation of `Visitor` that allows an `Operation` to be transformed into the
			code inside a `Builder` object. 

* `OperationCloner` -	helper class used to clone an Operation with different operands or results
				or types or symbols, etc. Follows a builder-like pattern. See
				(OperationCloner.md)[OperationCloner.md].

* `OperationReplacer` -	helper class used to replace one Operation possibly multiple times. uses
				`*Mapper` objects (e.g. `ValueMapper`, `TypeMapper`, etc.) to provide operands,
				results, etc. for each new replacement for the original `Operation`. Used by
				`TypeReplacer` pass. See (OperationReplacer.md)[OperationReplacer.md].

* `TypeReplacer` -		subclass of `Transformer` which can replace references to one `Type` with
				another `Type` or help with exploding any `Type` that has a *layout* into its
				constituent fields (think of a layout like a struct). See
				(TypeReplacer.md)[TypeReplacer.md].

* `JB1CodeGenerator` -	code generator written to use JitBuilder and hence the OMR compiler to generate
				native code. Sets up a JitBuilder compilation that visits each `Builder` and
				`Operation` for translation using `JB1MethodBuilder`. See
				(JB1CodeGenerator.md)[JB1CodeGenerator.md].

* `JB1MethodBuilder` -	support class for converting JB2 `Builder`s and `Operation`s to JitBuilder API
				calls to generate code during a visit driven by `JB1CodeGenerator`. See
				(JB1MethodBuilder.md)[JB1MethodBuilder.md].


## Extensions

Note that the core JB2 infrastructure does not define any specific `Type`s, `Operation`s, `Symbol`s, etc. Only
the core infrastructure is provided. This design is on purpose, so that the mechanisms for extensibility are
exercised even for basic simple `Type`s and `Operation`s. JB2 currently has two `Extension`s:

* `Base	 -	The `Base` extension introduces basic Types and Operations that roughly correspond to those
		provided in the original JitBuilder API. It introduces `Function`s and `FunctionCompilation`s
		to compile them. See (BaseExtension.md)[BaseExtension.md].

* `VM` -	The `VM` extension introduces a subset of the original JitBuilder APi that is specifically
		designed for implementing compilers for VirtualMachines. `BytecodeBuilder` and `VirtualMachineState`
		along with several subclasses of `VirtualMachineState` are included. See
		(VMExtension.md)[VMExtension.md].


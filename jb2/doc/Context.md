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

#  Context

`Context` is a concept associated with the target program: it describes the
lexical execution context for a specific unit of the eprogram. For example, the
 `Context` for a C function might include `Symbol`s that describe the parameters and
their `Type`s, the expected return `Type`, and possible definitions for other external
`Symbol`s that can be accessed from within that function. A `Context` has a number of
entry points as well as some number of exits. These both default to one but can be
increased when the `Context` is constructed.

Every `Context` has a `parent` but it is allowed to be `NULL` to represent top-level
(also called global) execution contexts. There can be multiple top-level execution
`Context`s but they cannot (by default) reference each other. Each `Compiler` defines
one `GlobalContext` top-level execution context if none is specified. If a `parent` is
specified, the parent `Context` must completely overlap the child `Context`, and child
`Context`s are assumed not to overlap with one another.

A `Context` can hold its own `LiteralDictionary`, `TypeDictionary` and `SymbolDictionary`.
Implementations may choose to only maintain these dictionaries for specific `Context`
objects, for example those that are associated with the scope of a `CompileUnit`, but
every `Context` can choose independently of other `Context`s. Each `Context` knows about
its parent and also its children `Context`s.

`Symbol` lookup by default delegates to its parent if no `SymbolDictionary` has
been created for a particular `Context`. However, each lookup can be directed to
search only the object's dictionary and to give up, or to continue lookup through to
parent `Context` dictionaries until a match is found (or it is not found in the
top-level `Context`).

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

`Context`s represent the state of `Symbol`s accessed by a target `CompileUnit`.
`Context`s are associated with `Builder`s to describe the state under which the
`Builder` executes. While `Operation`s refer to `Symbol`s and `Value`s and canu
read or write to them, what is actually done to read or write a `Symbol` or a
`Value` is delegated to the `Context`. This abstraction provides at least three
useful capabilities for the compiler.

First, some kinds of useful capabilities can be abstracted away (or not) from
the `Operation`s specified by the `CompileUnit`.  For example, by default, reading
or writing to a `LocalSymbol` or `ParameterSymbol` will just read or write to a
location in a function's stack frame. But those same `Symbol`s could also be read
or written by a `DebugContext` which maps those `Symbol`s to load or store
specific values out of a simulated (Debugger) stack frame. Another example
would be the addition of value tracing or profiling to a compiled function.
By isolating these different code requirements to the `Context`, generating code
for a tracing or native execution scenarios does not need to impact the quality
of code analysis / optimization because the fundamental Operations are no
different between the two scenarios (until the code is actually generated, of
course).

A second important capability enabled by the `Context` abstraction is to
represent refinement. If an analysis proves that a particular `Symbol` or
`Value` will produce a constant value, for example, that can be recorded
in the `Context`. For correctness, that means that the compiler can split
`Context` objects in cases where not all `Builder`s that share the `Context`
can support the same refinement. `Context` is like a cached "global" state
for entry to a `Builder` object, similar to an `IN` dataflow set often
computed for basic blocks during a dataflow analysis in a traditional compiler.

Thirdly, `Context` objects represent the different kinds of global state accessible
to the `CompileUnit` and can represent different levels of granularity at different
times. For example, early on when compiling for a language runtime, `Context` objects
might describe more "virtual" kinds of state like an operand stack or a virtual
machine register. Through the compilation, the `Context` may evolve into a lower
level model resembling a native stack frame. In the final stages of compilation,
the `Context` objects may contain native register context as `Symbol`s or `Value`s
are assigned to registers. A register `Context` may also be used when there are
specific linkage requirements for the environment in which the code will execute or
to express desirable regsiter allocation hints to the compiler.  In other scenarios,
for example a binary translator, the `Context` may start as state variables from the
source architecture to be translated later into state variables of the target
architecture.

`Context` defines virtual functions to read and write `Symbol`s and `Value`s. By
design, these functions delegate to any parent `Context` if there is no specific
information in the `Context` object for the requested `Symbol` / `Value`. The
`CompileUnit` should have a top level `Context` object with NULL parent; every
other `Context` object should have a parent that eventually leads to the top level
object. This "parent" relationship doesn't necessarily imply a lexical or
"structural" relationship betweent the `Context` objects.

This parent delagation model is also meant to be used, for example, to provide
step-wise refinement: the refined information for one particular `Symbol` or
`Value` can be stored in a specific `Context` object whose parent can hold the
less refined state of that same `Symbol` or `Value` as well as the state for
other `Symbol`s and `Value`s. So this parent delegation model helps to express
step-wise refined state in a simple and somewhat memory efficient manner. See
`StepRefinedContext`.

`Context` is also designed to be subclassed to facilitate different scenerios
like the tracing and debug scenarios mentioned above.

Collecting multiple `Context` objects together is also supported. A general
but inefficient `CombinedContext` object is provided that simply stores a
`List` of `Context` objects and delegates `Symbol` and `Value` lookups through
the list until a match is found, but it is expected that specific `Extension`s
will define more efficient aggregate `Context` objects that perform more
directed lookups to specific `Context` objects for known kinds of `Symbol`s
and `Value`s. For example, in a more specific environment, it may be known
that a `Symbol` lookup should search in an operand stack-specific `Context`
rather than looking through all the possible `Context` objects.
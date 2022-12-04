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

#  CompileUnit

`CompileUnit``instances represent a body of code that can be compiled by a
`Compiler`. A `CompileUnit` has a `Context` that captures what values are
available to the `CompileUnit`, how many entry and exit points it has, etc.
For example, a compiler designed to compile functions could define a
`Function``object that subclasses `CompileUnit` that corresponds to each
function in the target program that is to be compiled. The `Context` for
such a `Function` object might have information about the parameters
and their Types, the function return Type, etc. Similarly, a compiler that
compiles basic blocks might define a `BasicBlock` subclass of `CompileUnit`
that would correspond to each basic block in the target program that is
to be compiled, and its `Context` object might hold different kinds of
information (e.g. available local variables in an outer stack frame, or
perhaps registers and details of the Values stored in them).

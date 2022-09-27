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

#  Compilation

`Compilation` instances represent a compile unit . `Compilation` is a generic notion which
needs to be filled in by Extensions to support producing a particular kind of
compile unit. For example, a `FunctionCompilation` might be defined by an
`Extension` to be able to compile a function whose calling convention defines
parameters with types and produces return values with types. Other kinds of
compilation units can be created; the basic JB2 library does not define any
concrete `Compilation` unit. A `Compilation` can apply a set of `Pass`es
that may be `Optimization`s or `Analyses` defined by `Extension`s.

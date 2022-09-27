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

# `Builder`
`Builder` instances are analogous to a C or C++ code scope: i.e. a set of code statements written
inside curly braces. Every `Builder` object contains a sequence of `Operation`s which represent
the sequence of program steps that should execute if control is directed to the `Builder` object
(by another `Operation` like a `Goto` for example). `Builder` objects are called *bound* if the
flow out of the `Builder` object is determined by the `Operation` that caused control to reach it.
For example, a hypothetical `ForLoop` `Operation` may reference a bound `Builder` object
representing the body of the loop. `Builder` objects that are not bound (*unbound*) must explicitly
direct control flow to another `Builder`. Unbound `Builder` objects are not allowed to "fall through".
This requirement for unbound `Builder` objects represents a change from the original JitBuilder library
where `Builder` objects could fall through.

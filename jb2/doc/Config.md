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

# `Config`

`Config` instance associated with a `Compiler` or a `Compilation` that can be
queried by a `Compilation` to tailor its behaviour based on the user or client
environment. Examples would be to activate various kinds of logging or details
about the target environment for a `Compilation`. The `Config` object is meant
to be a repository of simple information; it should not perform any complicated
reasoning itself. Eventually, there will be a default command-line option
processing function that will translate command-line options into settings
recorded by the `Config` object and read by other classes in the `Compiler`
to adjust its behaviour.

`Config` is pretty basic at the moment. It has the following facilities:
* `setTraceBuildIL` - enable logging during the buildIL step of IL generation
* `setTraceCodeGenerator` - enable logging during JB1 code generation
* `setTraceTypeReplacer` - enable logging during type replacement
* `setLastTransformationIndex - diagnostic support to disable transformations after the given index

A `Config` object can be configured in a fluent interface where you
chain calls on the object to set the properties needed.

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

# `Extension`
`Extension` instances are like plug-ins that allow a `Compiler` to be customized to
support compilations using particular `Builder`s, `Operation`s, `Symbol`s,
`Type`s, `Compilation`s, etc. `Extension`s are loaded into a `Compiler`
instance and can then be used to create `Builder` objects and to add
`Operation`s into `Builder` objects, referring to specific `Symbol`s and
using `Type`s that are defined by the `Extension`. Mechanisms are
introduced at the `Extension` level to enable one `Extension` to add
support for its IL into the facilities created in another `Extension`.
An example might be an `DataFlow` extension introducing an analysis
engine that knows how to do a few kinds of dataflow analysis. Other
`Extension`s should be able to both add new kinds of dataflow analysis
and to add support for specific `Operation`s and `Type`s into a particular
analysis.

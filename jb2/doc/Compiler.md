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

# `Compiler`

`Compiler` instances represent different compilers created within the process. A
`Compiler` has a name (string), a `Config` that is used by all `Compilation`s performed
by each `Compiler`, and can load `Extension`s that define the IL that can be used by
the `Compiler`. Multiple `Compiler` objects can co-exist and each can load different
`Extension`s. Imagine a language JIT compiler could be implemented by one `Compiler`
object while a specialized compiler could load a second `Compiler` designed to accelerate
regex or pattern processing directly in native code.

`Compiler` objects can also be connected in a parent-child relationship, which can be
used to perform child compilations that leverage constructs in the parent compilations.
For example, an optimizer or a debugger can create child `Compiler`s to compute the
effects of an `Operation` or a `Builder`, which will need to access all the IL
constructs that could be loaded by the parent `Compiler` as well as create
new Types that should not pollute or appear in the parent `Compiler`.

Top level `Compiler` objects have a name and an optional `Config` object that can
configure the `Compiler`. For example:

```
{
    ...
    Compiler c("My Compiler");
    ...
}
```
creates a `Compiler` that will be known as "My Compiler" and whose lifetime will
extend until the end of its lexical scope. Alternative, some configuration can be
performed by passimg a `Config` object (see [Config.md](Config.md):

```
{
    ...
    Config cfg;
    cfg.setTraceCodeGenerator()->setTraceBuildIL();
    Compiler c("My Compiler", &cfg);
    ...
}
```
In this example, any compilations performed by "My Compiler" will output tracing
information for IL generation (`setTraceBuildIL`) and native code generation
(`setTraceCodeGenerator`).  For more details on configuration options, see
[Config.md](Config.md).

Parent `Compiler`s can be created only providing an optional name. Child `Compiler`
objects can be created by referencing the parent `Compiler` when constructed. When
a `Compiler` object is deconstructed, all the memory and objects created by that
`Compiler` are reclaimed including `CompileUnit`s and `CompiledBody`s (which includes
the code compiled by the `Compiler`). `Extension`s loaded by the `Compiler` may
also be unloaded if they are the only `Compiler` that loaded the `Extension`.

The following creates a child compiler for "My Compiler" that does not have any
additional configuration (assuming `c` is accessible):

```
    Compiler childCompiler("Child compiler", NULL, &c)
```

You must keep each `Compiler` object alive as long as you want to be able to invoke
any code is has compiled.

To load an `Extension` into a `Compiler` use the `loadExtension<T>()` facility:

```
{
    Compiler c("My Compiler");
    Base::BaseExtension *bx = c.loadExtension<Base::BaseExtension>();
    ...
}
```
Specific versions of an `Extension` can be required by passing a `SemanticVersion`
object initialized with the required minimum major,minor,patch version numbers:

```
{
    Compiler c("My Compiler");
    SemanticVersion required(2,1,0); // requires version 2.1.0 or compatible
    Base::BaseExtension *bx = c.loadExtension<Base::BaseExtension>(required);
    ...
}
```

If, for amy reason, the requested `Extension` cannot be loaded, `loadExtension<T>()`
will return NULL. In this case, you can check the error condition on the
`Comppiler` object to get more information on why it failed:
```
{
    Compiler c("My Compiler");
    Base::BaseExtension *bx = c.loadExtension<Base::BaseExtension>();
    if (bx == NULL) {
        CompilerException *e = c.errorCondition();
        CompilerReturnCode rc = e->result();
        if (rc == c.CompilerError_Extension_CouldNotLoad) {
            // failed to load required library name
        } else if (rc == c.CompilerError_Extension_HasNoCreateFunction) {
            // extension library doesn't have a create() function
        } else if (rc == c.CompilerError_Extension_CouldNotCreate) {
            // extension library's create function returned NULL (could not allocate Extension object)
        } else if (rc == c.CompilerError_Extension_VersionMismatch) {
            // extension loaded does not meet required version
        } else {
            // unexpected
        }
    }
    ...
}
```

Whenever an error condition is detected (`c.hasErrorCondition() == true`), you can access
the error condition (`CompilerExtension` object). The `CompilerException` can give you a 
string representing the error code (`c.errorCondition()->resultString()`) or a string
that may contain a more detailed error message (`c.errorCondition()->message()`). You may
be able to learn the code location that caused the error condition by asking for the error
condition's location (`c.errorCondition()->location()`). This location is also included
in the error message.

# Core file debugging

`Compiler` objects are fundamental objects from which other objects can be found. But
without the `Compiler` object, it can be more difficult to interpret the data in a core
file. For this reason, we add an "eye catcher" (specific magic number) to the beginning
of each `Compiler` object in memory. This eye catcher is `Compiler::EYE_CATCHER_COMPILER`
and it is set to `0xAABBCCDDDDCCBBAA`. Searching the memory in a core file for this
number is likely to identify possible live or recently live `Compiler` objects.

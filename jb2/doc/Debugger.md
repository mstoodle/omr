# Debugger

The Debugger is defined inside the Debug extension (`Debug::DebugExtension`). It can be used
to debug functions at the IL level even in situations where not all the IL is available to
generate executable instructions. All the facilities described below are defined in the
`OMR::JitBuilder::Debug` namespace. The DebugExtension depends on the BaseExtension being
loaded.

## Debugger

Debugger should be an automoton that performs debugging actions on DebuggerFrames
    Should support multiple active independent DebuggerFrames
    how are results communicated back?
    do we need a DebuggerInterface class?
    Operation * should be equivalent of PC, maintained by DebuggerFrame
    
DebugInputLoop should accept text commands and drive actions against a Debugger
DebuggerThunk enters DebugInputLoop via a native call to the thunk which initiates a call action in the Debugger

The `Debugger` class is used to initiate a debug session. It is possible to create multiple
Debugger objects, though only one of them can be productively interactive at a time. Each
Debugger object is tied to a particular Compiler object and can be used to debug Functions
that are owned by the same Compiler object. Because the Debugger performs compilations itself,
however, the Debugger creates a child Compiler. Since the child Compiler's lifetime is tied
to the lifetime of the Debugger object, all compilations performed by the Debugger will only
exist while the debugger session is live. Once the session Debugger object is freed, all
compiled code associated with that session will be freed.

The Debugger class contains the implementation of the main loop that drives the interactive
debugger session. In order to debug a function, one requests a `debugEntry<T>(Function *)`
from the Debugger object for a particular function. What is returned by `debugEntry` is
a native callable function entry point that expects the same signature as the given
Function. Instead of calling the native code for the function, however, the entry point
returned simply passes the function's arguments to the Debugger object and initiates the
interactive debugging session. This native entry is called a `DebuggerThunk`. To generate
this thunk, a `DebugCompilation` is initiated for the `Function` which uses settings in
the `Config` object to determine when to stop the compilation so that the current version
of the IL can be debugged. By default, only the `initContext()` and `buildIL()` functions
will be called on the Function which means the Debugger defaults to debugging the initial
IL.

The `DebugCompilation` uses a special `DebugDictionary` subclass of `TypeDictionary` to
dynamically create many Types that are useful to simulate the target Function's IL.

When the seesion is initiated, it constructs a `DebuggerFrame` to represent the simulated
state for ths provided Function. In particular, this frame object holds two arrays that
hold the simulated values for every Value and every Symbol defined by the target Function.
These arrays are indexed by the id() of the Values and Symbols and each element is called
a `DebugValue`, but this type is computed dynamically based on the full set of Types
accessed by the target Function. There is a static type called `DebugValue` but it is
only a memory template that helps the Debugger source code to access the memory
pointed at by the actual dynamic DebugValue type (for example, to print the value of
a Value or a Symbol). These arrays are allocated when the DebuggerFrame object is
initialized. The values of the arrays are read and written by the simulation of the
Function's IL.

To simulate the Operations in the target Function, the Debugger will compile and call
a set of small functions called `OperationDebugger`s. There is one `OperationDebugger`
per original Operation objet in the target Function's IL, but these op debuggers (as
they are called) are compiled dynamically only when needed by the simulation of the
DebuggerFrame. If an Operation is never executed, no op debugger for that Operation
will be compiled. Each op debugger is specific to a specific Operation: it embeds
the specific Value IDs, Symbol IDs, Builder IDs, and Types that are referenced by
the Operation.

Op debuggers are compiled with FunctionCompilations (not DebugCompilations). They
each contain the target Operation along with additional code (preceding and following)
that is constructed dynamically to read and write the simulated state of the
DebuggerFrame rather than native data accessed by each Operation. In this way,
the Debugger reuses the code generation implementation for each Operation to
simulate IL. For example, code is inserted ahead of the Operation to load
DebugValues from the simulated state in the DebuggerFrame and the Operation is
rewritten to consume those loaded DebugValues. Simiarly, if the Operation produces
a result Value, that Value is then written back to the appropriate DebugValue
in the DebuggerFrame.

Operations that initiate control flow (via bound or unbound Builders) require
special handling because op debuggers are not recursive and do not call other
op debuggers. One of the parameters to an op debugger is called `fromBuilderID`,
representing the Builder from which control was directed to the Operation. An
op debugger also returns a Builder ID that represents the next Builder that
should be executed. When `fromBuilderID` is equal to the Operation's parent
Builder, that means the Operation is being executed for the first time as part
of the sequence of Operations in the parent (i.e. control just flowed to
this Operation via the previous Operation or it's the first Operation in
the parent Builder). If the Operation is going to direct control to another
Builder (say an IfCmpEqualTo Operation where the operands are equal), then
the op debugger needs to return the target Builder ID. The Debugger will
take this return value as indicative of control flow. If there is no 
outward control flow, then the op debugger returns the Operation's parent
Builder ID.

In the case of bound Builders, eventually the Debugger will finish simulating
the Operations in the bound Builder. After the last Operation is simulated,
control needs to return to the parent Operation. In this case, the
`fromBuilderID` passed by the Debugger will be the bound Builder, and the op
debugger will jump to a point that follows the reference to the original
target Builder ID. The original Builder target is replaced by a new
Builder that does the following: it first saves every Symbol value that
may have been changed by the Operation, then it returns from the op
debugger function with the return value of the original Builder target.
Following the return operation is a Builder that is really a label.
When the Debugger calls the op debugger after executing the target
bound builder, a switch statement at the beginning of the op debugger
will direct control to this "label" Builder. In this way, whatever
the Operation needs to happen after the bound Builder is executed
will be performed when the op debugger is called by the Debugger
following the simulation of the bound Builder's Operations.


# JitBuilder 2.0

This project is probably a temporary repository for more quickly iterating on the development
of the version 2.0 of the JitBuilder library from the
[Eclipse OMR project](https://github.com/eclipse/omr).

This project depends on Eclipse OMR for code generation. To use JB2, you first need to clone
and build Eclipse OMR. Then you need to build the JB2 core library (`libjbcore.so`) and its
`Base` extension library (`libbase.so`). Then you can try out the code sample in the
`samples` directory or run the tests in the `test` directory.

## Get and build Eclipse OMR

First, you need to clone the upstream Eclipse OMR repository:
```
$ git clone git@github.com:eclipse/omr.git
```

Once cloned, you need to get, configure, and build OMR's JitBuilder library:
```
$ cd jb2
$ make getomr
$ make omrcfg # you can also do omrcfgdbg for a debug build
$ make omrjb
```

The `getomr` target clones the Eclipse OMR project into the `omr` directory. Then
`omrcfg` runs cmake to configure the OMR build to compile the JitBuilder library.
Use `omrcfgdbg` if you want the OMR compiler to be compiled with debug symbols,
but it will generate a MUCH larger library that will slow down JB2 linking.
Finally, the `omrjb` target actually builds the JitBuilder library, which may
take a while.

## Build JB2 core library

Once the JitBuilder library has been built, you can build JB2's core library:
```
$ make libjbcore.so
```

## Build the Base extension library

The `libjbcore.so` shared object contains both the core JB2 library as well as
the JitBuilder library from Eclipse OMR. However, it does not include many
JB2 feature that you can use directly. The primary extension with JB2 is called
`Base` and it can be built by going into the `Base` directory and building it:
```
cd Base && make
```

This command will build `Base/libbase.so` which can be used by a JB2 program
to load the Base extension features into a `Compiler`.

## Trying out the matrix multiply code sample

 There is an example (called `matmult`) for how to use JB2 in the top level
`samples` directory:
```
cd samples
make matmult
./matmult
```

This sample demonstrates how to dynamically compile a function that can multiple
dense matrices of different types (float and double are the two examples included).
The source code for this sample is in `MatMult.hpp` and `MatMult.cpp`. Try
following along with the code in the `main()` function as it breaks down the
various steps needed to use JB2.

## How to build and run the tests

There is a top-level `test` directory which contains some tests of some core
JB2 features (`testsemver` and `testcompiler`) as well as tests of the
features of the `Base` extension (`testbase`). The Makefile will make all of
them by default or you can use a specific target to only generate those tests.
For example, this command builds and then runs `testbase` :
```
$ make testbase
$ ./testbase
```

## What's next?

Welcome to the world of JB2! I'll be creating more documentation and code
samples, porting more JitBuilder features into the Base extension, and
creating new extensions (e.g. Complex, DenseArray) as this library continues
to evolve.

Let me know what you think or if you have any suggestions or feedback! You can
(preferably) [open an issue at this repository](https://github.com/mstoodle/jb2/issues), or
(if you really prefer to) you can reach me directly at mstoodle AT gmail.com .
You can also find me at the #jitbuilder channel at the
[Eclipse OMR slack](https://join.slack.com/t/eclipse-omr/shared_invite/enQtMzg2ODIwODc4MTAyLTk4ZjJjNTZlZmMyMGRmYTczOTkzMGJiNTQ4NTA3YTA1NGU4MmJjNWI4NTBjOGNkNmNjMWQ3MmFmYjA4OGZjZjM)!

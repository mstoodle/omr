# Building JB2

To build JB2 as part of OMR, you need to activate `OMR_JB2` (and `OMR_COMPILER`) when you run `cmake`. That will
 cause all of the JB2 functionality to be built and for its tests to be added to the main test list. After the
usual steps to clone the OMR project and then creating a build directory.

For example:

```
$ git clone git@github.com:mstoodle/omr.git          # once jb2 merged, can be eclipse/omr.git
Cloning into 'omr'...
remote: Enumerating objects: 96928, done.
remote: Counting objects: 100% (254/254), done.
remote: Compressing objects: 100% (146/146), done.
remote: Total 96928 (delta 142), reused 177 (delta 106), pack-reused 96674
Receiving objects: 100% (96928/96928), 62.56 MiB | 3.19 MiB/s, done.
Resolving deltas: 100% (77049/77049), done.
$ cd omr
$ git checkout jb2                                   # until merged, need to use jb2 branch
$ mkdir build.release && cd build.release
```

At this point, you need to run `cmake`:
```
$ cmake .. -DOMR_JB2=1 -DOMR_COMPILER=1 -DCMAKE_BUILD_TYPE=Release
```

If you want, you can alternatively specify `-DCMAKE_BUILD_TYPE=Debug` to build OMR and JB2 with
debug info (I usually do this in a build.debug directory).

Once you've finished configuring with `cmake` you just need to build JB2:
```
$ make
```

You can verify that your build works by running `make test` (jb2 tests automatically
add themselves to the test target).
```
$ make test
```

The JB2 functionality tests run very quickly and seem to usually appear in the first few
tests. They should all pass:
```
Running tests...
Test project /home/mstoodle/omr/build
Test project /home/mstoodle/omr/build.debug
      Start  1: JB2TestSemVer
 1/20 Test  #1: JB2TestSemVer ....................   Passed    0.00 sec
      Start  2: JB2TestCompiler
 2/20 Test  #2: JB2TestCompiler ..................   Passed    0.01 sec
      Start  3: JB2TestBase
 3/20 Test  #3: JB2TestBase ......................   Passed    0.23 sec
      Start  4: JB2TestVMRegister
 4/20 Test  #4: JB2TestVMRegister ................   Passed    0.03 sec
      Start  5: JB2TestVMOperandStack
 5/20 Test  #5: JB2TestVMOperandStack ............   Passed    0.04 sec
      Start  6: algotest
...
```

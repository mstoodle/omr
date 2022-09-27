# Building JB2

To build JB2 as part of OMR, you need to activate `OMR_JB2` (and `OMR_COMPILER`) when you run `cmake`. That will
 cause all of the JB2 functionality to be built and for its tests to be added to the main test list. After the
usual steps to clone the OMR project and then creating a build directory.

For example:

```
$ git clone git@github.com:eclipse/omr.git
Cloning into 'omr'...
remote: Enumerating objects: 96928, done.
remote: Counting objects: 100% (254/254), done.
remote: Compressing objects: 100% (146/146), done.
remote: Total 96928 (delta 142), reused 177 (delta 106), pack-reused 96674
Receiving objects: 100% (96928/96928), 62.56 MiB | 3.19 MiB/s, done.
Resolving deltas: 100% (77049/77049), done.
$ cd omr
$ mkdir build.release && cd build.release
```

At this point, you need to run `cmake`:
```
$ cmake .. -DOMR_JB2=1 -DOMR_COMPILER=1 -DCMAKE_BUILD_TYPE=Release
```

If you want, you can specify `-DCMAKE_BUILD_TYPE=Debug` to build OMR and JB2 with debug info.

Once you've finished configuring with `cmake` you just need to build JB2:
```
$ make
```

You can verify that your build works by running `make test`
```
$ make test
```

The first few tests are testing JB2 functionality; verify that they pass:
```
Running tests...
Test project /home/mstoodle/omr/build
      Start  1: gcexample
 1/22 Test  #1: gcexample ........................   Passed    0.02 sec
      Start  2: TestJB2SemVer
 2/22 Test  #2: TestJB2SemVer ....................   Passed    0.01 sec
      Start  3: TestJB2Compiler
 3/22 Test  #3: TestJB2Compiler ..................   Passed    0.01 sec
      Start  4: testbase
 4/22 Test  #4: testbase .........................   Passed    0.07 sec
      Start  5: testregister
 5/22 Test  #5: testregister .....................   Passed    0.01 sec
      Start  6: testoperandstack
 6/22 Test  #6: testoperandstack .................   Passed    0.01 sec
      Start  7: algotest
...
```

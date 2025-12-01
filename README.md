
Here's how this is supposed to work

```
mkdir build
cd build
cmake ../ -DCMAKE_VERBOSE_MAKEFILE=1
cmake --build . -- -j`nproc`
```

For HW3 you should add following arguments to build your project:
```
-D KIRAZ_TEST_WASMGEN=ON -D KIRAZ_TEST_WASMGEN_MOZJS=ON
```

For Mac users installed flex w "brew":

```
mkdir build
cd build
cmake ../ -D FL_LIBRARY=/opt/homebrew/opt/flex/lib/libfl.a -D FL_INCLUDE_DIR=/opt/homebrew/opt/flex/include -D BISON_EXECUTABLE=/opt/homebrew/opt/bison/bin/bison
cmake --build . -- -j`nproc`
```

For mac users, since mozjs-115 is archived, you can use mozjs-128. it is available on macports. 
After installed via ports, you should add flags to your path. (ask if you cannot do.) 
So the full cmake command should look like this:
```
cmake ../ \
  -D FL_LIBRARY=/opt/homebrew/opt/flex/lib/libfl.a \
  -D FL_INCLUDE_DIR=/opt/homebrew/opt/flex/include \
  -D BISON_EXECUTABLE=/opt/homebrew/opt/bison/bin/bison \
  -D KIRAZ_TEST_WASMGEN=ON \
  -D KIRAZ_TEST_WASMGEN_MOZJS=ON \
  -D MOZJS_INCLUDE_DIR=/opt/local/include \
  -D MOZJS_LIBRARY=/opt/local/lib/libmozjs-128.dylib \
  -DCMAKE_EXE_LINKER_FLAGS="-L/opt/local/lib"
```

Once the code compiles, you can use the ``ctest`` command to run the
test suite. Peruse ``ctest --help`` output for more info

Cheatsheet:

```shell
ctest -j`nproc`
ctest --rerun-failed
ctest --output-on-failure
ctest -V
ctest -R plus.*  # (regex)
```

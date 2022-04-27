# M2 Cell

## Required Package

- boost library available from mutilple sources
  - www.boost.com
  - centos  `yum install boost-devel`
  - Ubuntu  `apt-get install libboost-all-dev`
- nlohmann json for CPP
  - centos `yum install json-devel`
  - Ubuntu `apt install nlohmann-json3-dev`
- yaml-cpp
  - centos `yum install yaml-cpp`
  - Ubuntu `apt install libyaml-cpp-dev`
- [Catch2](https://https://github.com/catchorg/Catch2)
- [gcovr](https://github.com/gcovr)
- clang-format (optional)
- glib-devel

## Code Format

The C++ code is automatically formatted by `clang-format` see the `.clang-format` file.
To enable this with a git pre-commit hook:

- Install the `clang-format` C++ package.
- Run `git config core.hooksPath .githooks` once in this repository.

## Compile the software

The executable is `bin/m2cell`

```bash
make
```

The software compiles significantly faster with the following line, but this
prevents code coverage from working.

```bash
NOGCOVR=1 make
```

To remove all files from previous builds

```bash
make clean
```


## Unit Tests and code coverage

### Do the following to make and run the unit tests.:

```bash
make run_tests
```

The output of most unit tests can be reduced by setting the environment
variable `LOGLVL` to 6, for minimal output.

```bash
export LOGLVL=6
```

For verbose output either of the following

```bash
export LOGLVL=1
```

Default behavior is verbose if the `LOGLVL` variable is undefined.

```bash
unset LOGLVL
```

### To run code coverage

```bash
make clean
unset NOGCOVR
make run_tests
gcovr -r src/ .
```

The `unset NOGOVR` and `make clean` can be skipped if NOGCOVR was never set.

If the Cobertura xml report is needed, change the `gcovr` command to

```bash
gcovr -r src/ . --xml-pretty > coverageReport.xml
```

To produce Jenkins results, `make junit` needs to be called.  For details, please see the `Jenkinsfile` `Unit Test` section.

### To make documentation

```bash
make doc
```

UML documents are available in the `/doc` subdirectory The text of these can be cut
and pasted into `http://www.plantuml.com/plantuml/uml/`.
Current UML files are
```bash
/doc/comClassUML.txt
```

### Environment variables for the make file

For each of the following, setting or unsetting the environment
variable will probably require a `make clean` to have its effects
apply to all files.

- `DEBUG` setting this will have the code compile with the `-g` option so the code will include debugging information for gdb.

- `SIMULATOR` setting this cause make to build for the simulator.

- `NOGCOVR` setting this removes compiler options that the `gcovr` coverage tool requires to generate reports. It does reduce compile time.

- `VERBOSE` setting this should result in verbose output from the compiler.

### Cross Compiling

# Cross compiler installation

- On the build computer, download the Linux version for x68 from https://www.ni.com/en-us/support/downloads/software-products/download/unpackaged-linux.gnu-c---c---compile-tools-x64.338443.html
- run the downloaded file to install - oecore-x86_64-core2-64-toolchain-6.0.sh

# Building with the cross compiler

- When a new shell is started:

  ```bash
  source /usr/local/oecore-x86_64/environment-setup-core2-64-nilrt-linux
  export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/lib/x86_64-linux-gnu/pkgconfig/
  ```

- build with

  ```bash
  make tests
  ```

# Running cross compiler unit tests on the cRIO

cRIO:

```bash
mkdir <reasonable_directory>/ts_m2cellcpp
```

Build machine: (`scp -r ts_m2cellcpp` copies a large set of .git files, so avoid that.)
source statement is only needed for a new shell

```bash
source /usr/local/oecore-x86_64/environment-setup-core2-64-nilrt-linux
cd ts_m2cellcpp
make clean
NOGOVR=1 make -j8 run_tests
scp -r * admin@<crIO>:<reasonable_directory>/ts_m2cellcpp/.
```

cRIO: (note: `make crio_x_test` will not build anything, it just runs what it finds in the tests directory.)

bash```
cd <reasonable_directory>/ts_m2cellcpp/tests
make crio_x_test
```

### Building on the cRIO

This is not recommended as the cRIO has limited resources.

Installing required packages
- On cRIO
-- opkg update
-- opkg install packagegroup-core-buildessential
-- opkg install boost-dev      (opkg list | grep boost)

- On local machine, install nlohmann, and clone Catch2
-- copied nlohmann header files to <cRIO>:/usr/include/nlohmann/.
-- cd Catch2     (check that the branch is v2.x)
-- scp -r single_include/catch2/ <crIO>:/usr/include/.
- Clone yaml-cpp to the local machine and copy it to an appropriate directory on the cRIO.
-- Build yaml-cpp library on the cRIO
--- ssh to the cRIO
--- cd yaml-cpp      (Top level of copy of yaml-cpp clone.)
--- mkdir build && cd build
--- cmake .. && make -j
--- make install
- On cRIO build m2cell
-- cd ts_m2cellcpp
-- make clean
-- edit Makefile.inc and add "-lboost_system"  (no quotes) to the "CPP :=" lines
-- make  (this will take several minutes)

# M2 Cell

## Required Package

- boost library available from mutilple sources
  - www.boost.com
  - centos  `yum install boost-devel`
  - Ubuntu  `apt-get install libboost-all-dev`
- nlohmann json for CPP
  - centos `yum install json-devel`
  - Ubuntu `apt install nlohmann-json3-dev`
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

### To run code coverage 

``` bash
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

### Environment variables for the make file

For each of the following, setting or unsetting the environment 
variable will probably require a `make clean` to have its effects
apply to all files.

- `DEBUG` setting this will have the code compile with the `-g` option so the code will include debugging information for gdb.

- `SIMULATOR` setting this cause make to build for the simulator.

- `NOGCOVR` setting this removes compiler options that the `gcovr` coverage tool requires to generate reports. It does reduce compile time.

- `VERBOSE` setting this should result in verbose output from the compiler.

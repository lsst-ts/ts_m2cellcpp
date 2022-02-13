# Common Code of the Main Telescope Rotator and Hexapod Controllers

## Needed Package

- boost library available from mutilple sources
  - www.boost.com
  - centos - yum install boost-devel
  - Ubuntu - apt-get install libboost-all-dev
- [Catch2](https://https://github.com/catchorg/Catch2)
- [gcovr](https://github.com/gcovr)  
- clang-format (optional)
- glib-devel
- glib2-devel.x86_64

## Code Format

The C/C++ code is automatically formatted by `clang-format` see the `.clang-format` file.
To enable this with a git pre-commit hook:

- Install the `clang-format` C++ package.
- Run `git config core.hooksPath .githooks` once in this repository.

## Compile the software

```
make
```

The software compiles significantly faster with the following line, but this 
prevents code coverage from working.

```
NOGCOVR=1 make
```

To remove all files from previous builds

```
make clean
```


## Unit Tests and code coverage

1. Do the following to make and run the unit tests.:

```
make run_tests
```

2. To run code coverage 

``` 
make clean
unset NOGCOVR
make run_tests
gcovr -r src/ .
```
The `unset NOGOVR` and `make clean` can be skipped if NOGCOVR was never set.

If the Cobertura xml report is needed, change the `gcovr` command to
```
gcovr -r src/ --xml-pretty > coverageReport.xml
```

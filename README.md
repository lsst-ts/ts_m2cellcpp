# Common Code of the Main Telescope Rotator and Hexapod Controllers

## Needed Package

- [Catch2](https://https://github.com/catchorg/Catch2)
- clang-format (optional)
- glib-devel
- glib2-devel.x86_64

## Code Format

The C/C++ code is automatically formatted by `clang-format` see the `.clang-format` file.
Format can be applied inside `vs code` with `'shift' + 'alt' + 'f'`

To enable this with a git pre-commit hook:

- Install the `clang-format` C++ package.
- Run `git config core.hooksPath .githooks` once in this repository.

## Setup the Environment



## Unit Tests

1. Do the following to do the unit tests in docker container:

```make run_tests
```



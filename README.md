# Unarian C++ Interpreter
[Unarian](https://esolangs.org/wiki/Unarian) is an esoteric programming
language where every program is a unary function. Programming in the language
is quite tricky because you effectively only have a single accumulator to hold
the program's state.

This repo is for a fast interpreter for Unarian written in C++. This
interpreter compiles programs to a fast bytecode representation, while also
condensing multiple of the same instruction into one, and optimizing common
patterns, such as multiplication and division. As such, this interpreter
represents a large improvement over the reference interpreter.

## Building
Assuming you have Meson, Ninja, Boost and a compiler with C++20 support, the interpreter can built like so:

```bash
$ meson build --prefix=$(pwd)/dist
$ ninja -C build install
# The interpreter should now be installed at ./dist/bin/unarian
$ echo 12 | ./dist/bin/unarian examples/fibonacci.un -i
# 144
```

## Usage
To run a program, simply provide it to the interpreter. By default, it will
evaluate the function `main` with the input of `0`.

```bash
$ unarian examples/power_of_two.un
# 1
```

To run with interactive input, provide either `-i` or `--input` to the interpreter.

```bash
$ echo 1 2 3 4 | unarian examples/power_of_two.un
# 2
# 4
# 8
# 16
```

To evaluate an expression, other than `main`, provide it using the `-e` or
`--expr` options.

```bash
$ echo 2 | unarian -e '^2 + +' examples/power_of_two.un
# 6
```

To see the debug output when running a program, pass `-g` or `--debug` to the
interpreter. Note that only `!` is supported, to print the current value of the
accumulator. The `?` option, to print a stack trace, is not supported.

```bash
$ echo 2 | unarian --debug -e '! + ! + !' examples/some_file.un
# 2
# 3
# 4
```

To see the bytecode generated for a file, pass the `-b` option. This probably
not useful to you unless you're hacking on the interpreter.

```bash
$ unarian examples/power_of_2.un
# 0: TAIL_CALL 6
# 5: RET
# 6: TAIL_CALL 12
# 11: RET
# 12: DEC
# 13: FAIL_JMP 27
# 18: CALL 12
# 23: MULT 2
# 26: RET
# 27: INC
# 28: RET
```

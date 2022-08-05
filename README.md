# Unarian C++ Interpreter
This repo is for an interpreter for the [Unarian](https://esolangs.org/wiki/Unarian) esoteric programming language. The emphasis of this interpreter is on speed, and it is several times faster than the reference interpreter, making it the clear choice for all your serious Unarian computation needs.

## Building

Assuming you have [Meson](https://mesonbuild.com/), [Ninja](https://ninja-build.org/), [Boost](https://www.boost.org/) and a compiler with C++20 support, the interpreter can built like so:

```bash
$ meson build --prefix=$(pwd)/dist
$ ninja -C build install
# The interpreter should now be installed at ./dist/bin/unarian
$ seq 0 8 | ./dist/bin/unarian examples/fibonacci.un -i
# 0
# 1
# 1
# 2
# 3
# 5
# 8
# 13
# 21
```

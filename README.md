# pthread (winpthreads/libpthread) replacement for Windows 9x/NT

This is mingw binary replace pthread library, so you can compile program with newest msys2/mingw-w64.

## Usage

Compile (make) and add link flags to your program:

```<path-to-pthread9x>/crtfix.o -static -L<path-to-pthread9x> -lpthread```

`-lpthread` should be at first place, so GCC use symbols from this lib in first place.

## Configuration

Copy `default.mk` to `config.mk`.

## Source

Original sources is from older mingw release (winpthreads), `TryEnterCriticalSection` code is from KernelEx code. 

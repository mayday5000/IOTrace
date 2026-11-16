# IOTrace

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

IOTrace is an [Intel Pin](https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html) tool that shows which stdio file I/O routines a program executes, both in the main executable and in every shared library it loads. For each routine it reports the image it lives in, its entry address, the address of the `RET` it returned through, how many instructions ran inside it, and how it returns.

It's a small, readable starting point for reverse engineering and dynamic analysis: run any binary under Pin, no source code or recompilation needed.

## Traced routines

| Name fragment | Typical matches |
|---|---|
| `fopen` | `fopen` PLT stub, `fopen64`, `_IO_file_fopen` |
| `fgets` | `fgets` |
| `fseek` | `fseek` |
| `fclose` | `fclose` PLT stub, `_IO_fclose` |
| `fread` | `fread` |
| `fwrite` | `fwrite` PLT stub, `_IO_fwrite` |

Matching is by substring of the demangled symbol name, so glibc's internal aliases (and any other symbol that contains one of these names) are reported too. The list lives in `kTracedNames` in `iotrace.cpp`.

## How it works

1. `PIN_InitSymbols()` loads symbol tables so Pin can name routines.
2. `RTN_AddInstrumentFunction` registers `InstrumentRoutine`, which Pin calls once for every routine of every image as it's loaded. Routines whose name doesn't match are skipped and run at full speed.
3. For a matching routine, IOTrace saves its name, image and entry address, then walks its instructions:
   - every instruction gets a `CountInstruction` call, which adds 1 to the routine's counter;
   - every `RET` gets a `RecordReturn` call, which stores the address of that `RET`;
   - the `RET` mnemonic (or, if the routine has none, the mnemonic of its last instruction, usually a tail `JMP`) is saved for the report.
4. When the program exits, `Fini` writes one row per routine that actually executed to the output file.

The counters are `std::atomic`, so the numbers stay exact when several application threads call the same routine at once.

## Requirements

- **OS / CPU:** Linux on x86-64 with an Intel processor (a Pin requirement).
- **Pin:** Intel Pin 3.x or 4.x for Linux. Tested with Pin 3.31 (98869) and Pin 4.4 (99977).
- **Compiler:** GCC and GNU make. Pin 4.x supports GCC 6.1 to 14.
- **C++ standard:** the code uses only C++11 features, so it builds with Pin 3.x (which compiles tools with the compiler's default standard) and Pin 4.x (which uses `-std=c++17 -Wall -Werror`).

Tested on Debian 13 (kernel 6.12, glibc 2.41, GCC 14.2).

## Build

Download a Linux Pin kit from Intel's [Pin downloads page](https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-binary-instrumentation-tool-downloads.html) and unpack it:

```bash
tar xzf pin-external-4.4-99977-g54fbb8814-gcc-linux.tar.gz
export PIN_ROOT=$PWD/pin-external-4.4-99977-g54fbb8814-gcc-linux
```

Then build the tool from this repository:

```bash
make PIN_ROOT=$PIN_ROOT
```

The tool is written to `obj-intel64/iotrace.so`. Use `make PIN_ROOT=$PIN_ROOT clean` to remove it.

## Usage

```bash
$PIN_ROOT/pin -t obj-intel64/iotrace.so [-o <file>] -- <program> [args...]
```

| Option | Default | Description |
|---|---|---|
| `-o <file>` | `filetrace.txt` | Where to write the report. If the file can't be created, IOTrace prints an error and exits with code 1. |

The program runs normally, and the report is written when it exits.

### Example

`test/fileio_demo.c` writes three lines to a file, reads them back with `fgets`, rewinds with `fseek`, then reads again with `fread`:

```bash
gcc -O0 -g -o test/fileio_demo test/fileio_demo.c
$PIN_ROOT/pin -t obj-intel64/iotrace.so -- ./test/fileio_demo
```

```text
read: record 0
read: record 1
read: record 2
fread 15 bytes
```

`filetrace.txt` (Pin 4.4, Debian 13):

```text
              Procedure               Image       Call Address        Ret Address    Ins Count   Ret Mnemonic
-------------------------------------------------------------------------------------------------------------

                  fread         fileio_demo     0x55df01bda040                  -            3            JMP
                 fclose         fileio_demo     0x55df01bda050                  -            4            JMP
                  fgets         fileio_demo     0x55df01bda090                  -            6            JMP
                  fseek         fileio_demo     0x55df01bda0a0                  -            3            JMP
                  fopen         fileio_demo     0x55df01bda0b0                  -            4            JMP
                 fwrite         fileio_demo     0x55df01bda0d0                  -            5            JMP
             _IO_fclose           libc.so.6     0x7fe329489bb0     0x7fe329489c4b          138       RET_NEAR
                  fgets           libc.so.6     0x7fe32948a390     0x7fe32948a47e          250       RET_NEAR
                fopen64           libc.so.6     0x7fe32948a690                  -            4            JMP
                  fread           libc.so.6     0x7fe32948ab00     0x7fe32948abde           55       RET_NEAR
             _IO_fwrite           libc.so.6     0x7fe32948afe0     0x7fe32948b0cc          200       RET_NEAR
                  fseek           libc.so.6     0x7fe329492d80     0x7fe329492e20           37       RET_NEAR
         _IO_file_fopen           libc.so.6     0x7fe329495df0     0x7fe329495eb5          124       RET_NEAR
```

The rows from `fileio_demo` are the program's PLT stubs, which jump into libc. The rows from `libc.so.6` are the real implementations. Addresses change from run to run because of ASLR.

`test/mt_demo.c` is a multithreaded check: 4 threads each call `fwrite` 200,000 times. The `fwrite` PLT stub row should show 800,000 instructions (plus a few for the first, lazily bound call):

```bash
gcc -O0 -pthread -o test/mt_demo test/mt_demo.c
$PIN_ROOT/pin -t obj-intel64/iotrace.so -o mt.txt -- ./test/mt_demo
```

## Output format

One header, then one row per traced routine that executed at least one instruction, in the order the routines were instrumented.

| Column | Meaning |
|---|---|
| Procedure | Demangled routine name. |
| Image | File name of the executable or shared library that contains the routine. |
| Call Address | Entry address of the routine, which is where a `call` to it lands. |
| Ret Address | Address of the last `RET` instruction the routine executed, or `-` if it never executed one (PLT stubs and tail calls leave with a `JMP`). |
| Ins Count | Total instructions executed inside the routine, summed over all calls and threads. |
| Ret Mnemonic | Mnemonic of the routine's `RET` (`RET_NEAR`), or of its last instruction if it has no `RET` (usually `JMP`). |

## Limitations

- Only the six stdio routines above are traced. Lower-level I/O (`open`, `read`, `write`, `pread`, `mmap`, ...) and C++ streams aren't, unless their symbol names contain one of the fragments.
- Results are per routine, not per call: there are no arguments, return values, file names or timestamps, and `Ins Count` is a total.
- Routines are found through symbols. Stripped binaries without a dynamic symbol for a routine won't show it.
- The report is written by Pin's Fini callback, so it's only produced if the program exits normally (not on `SIGKILL`, a crash Pin can't handle, or `execve`).
- Child processes aren't followed unless you add Pin's `-follow_execv` option.
- Every instruction of a traced routine is instrumented, so heavily used I/O routines slow the program down while it runs under Pin.
- Only built and tested on Linux x86-64. A 32-bit build (`make PIN_ROOT=$PIN_ROOT TARGET=ia32`) needs 32-bit GCC support and is untested.

## Project structure

```text
IOTrace/
├── iotrace.cpp         # the Pin tool
├── makefile            # out-of-tree Pin build: make PIN_ROOT=/path/to/pin
├── makefile.rules      # tells the Pin build system which tool to build
├── test/
│   ├── fileio_demo.c   # single-threaded fopen/fwrite/fgets/fseek/fread/fclose demo
│   └── mt_demo.c       # 4-thread fwrite stress test for the counters
├── LICENSE
└── README.md
```

## License

This project is licensed under the [MIT License](LICENSE).

Intel Pin itself isn't included in this repository. It's distributed by Intel under its own license, so download it separately.

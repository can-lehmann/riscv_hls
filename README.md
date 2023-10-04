# High Level Synthesis RISC-V Core

A multi cycle RISC-V core for demonstrating the capabilities of my high level synthesis (HLS) compiler.

The RISC-V core is written in regular C code, but can be automatically compiled for FPGAs by my (currently still unreleased) HLS compiler.

It demonstrates the following features:
- State machine synthesis using `clock()` statements
- Functions calls and control flow
- Input/Output using `__input_*`/`__output_*`
- Structs, arrays and stack allocations
- Higher order functions (passing functions as arguments)

## License

This project is licensed under the MIT license.
See [LICENSE.txt](LICENSE.txt) for more details.

The files in `tools/opcodes` are from the [riscv-opcodes](https://github.com/riscv/riscv-opcodes) project.
They are licensed under a BSD-3-Clause license, see [tools/opcodes/LICENSE.txt](tools/opcodes/LICENSE.txt) for more details.


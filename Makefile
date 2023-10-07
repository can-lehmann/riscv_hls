all: bin/main build/main.o.opt

sim: bin/main bin/example.bin
	./bin/main bin/example.bin

bin/main: src/main.c src/decode.c
	clang src/main.c -o bin/main -DSIM

src/decode.c: tools/decode.tmpl.c tools/gen_decoder.py $(wildcard tools/opcodes/*)
	python3 tools/gen_decoder.py

build/main.o.opt: src/main.c src/decode.c
	clang src/main.c -o build/main.o -O0 -emit-llvm -c -Xclang -disable-O0-optnone -DHLS
	opt build/main.o -passes=mem2reg -o build/main.o.opt

bin/example.bin: bin/example.c
	clang --target=riscv32 -c -march=rv32i bin/example.c -o bin/example.o -fPIC -ffreestanding
	clang -S --target=riscv32 -c -march=rv32i bin/example.c -o bin/example.asm -fPIC -ffreestanding
	ld.lld bin/example.o -o bin/example
	llvm-objcopy -O binary -j .text bin/example bin/example.bin

all: bin/main

sim: bin/main
	./bin/main bin/example.bin

bin/main: src/main.c src/decode.c
	clang src/main.c -o bin/main -DSIM

src/decode.c: tools/decode.tmpl.c tools/gen_decoder.py $(wildcard tools/opcodes/*)
	python3 tools/gen_decoder.py

build/main.o.opt: src/main.c src/decode.c
	clang src/main.c -o build/main.o -O0 -emit-llvm -c -Xclang -disable-O0-optnone -DHLS
	opt build/main.o -passes=mem2reg -o build/main.o.opt



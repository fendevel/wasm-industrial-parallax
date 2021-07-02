all: ./script/game.js ./objs/main.o ./objs/walloc.o ./script/main.wasm ./objs/main.wat 

./objs/main.o: ./source/main.cpp
#	clang -Xclang -target-abi -Xclang experimental-mv -g3 -O3 -std=c++20 --target=wasm32-unknown-unknown -fPIC -Wl,--shared -Wl,--allow-undefined -Wl,--no-entry -nostdlib -msimd128 -mbulk-memory -mmultivalue $< -o ./objs/$@
	clang -Xclang -target-abi -Xclang experimental-mv -std=c++20 -g3 -O3 --target=wasm32-unknown-unknown -fPIC -msimd128 -mbulk-memory -mmultivalue -nostdlib -c $< -o $@

./objs/walloc.o: ./source/walloc.c
	clang -Xclang -target-abi -Xclang experimental-mv -g3 -std=c17 --target=wasm32-unknown-unknown -fPIC -msimd128 -mbulk-memory -mmultivalue -nostdlib -c $< -o $@
	
./script/main.wasm: ./objs/walloc.o ./objs/main.o
	wasm-ld --import-memory --no-entry -o $@ $^

./objs/main.wat: ./script/main.wasm
	wasm2wat --enable-all $< > $@

./script/game.js: ./source/game.ts
	tsc --outDir ./script/

.PHONY: copy
copy:
	cp -t $(INSTALLDIR) -r audio image script

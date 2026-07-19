format:
	@find src/ -regex '.*\.\(cpp\|hpp\|c\|h\|cc\|hh\|cxx\|hxx\)' -exec clang-format -style=file -i {} \;

clean:
	rm -rf build/*

.PHONY: build build-debug configure configure-debug

build: src
	@cmake --build build/release -j 8

build-debug: src
	@cmake --build build/debug -j 8

configure: src
	@cmake . -DCMAKE_BUILD_TYPE=Release -B build/release

configure-gmsh: src
	@cmake . -DCMAKE_BUILD_TYPE=Release -DBUILD_GMSH=ON -B build/release

configure-debug: src
	@cmake . -DCMAKE_BUILD_TYPE=Debug -B build/debug

database:
	ln -sf build/release/compile_commands.json compile_commands.json

database-debug:
	ln -sf build/debug/compile_commands.json compile_commands.json

run:
	@build/release/bin/renderer-app

run-debug:
	@build/debug/bin/renderer-app

meshtest:
	@build/release/bin/mesh-test

shaderc: shaders/*.frag shaders/*.vert
	@shaders/compile.sh

valgrind:
	@valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes build/debug/bin/renderer-app

valgrind-release:
	@valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes build/release/bin/renderer-app

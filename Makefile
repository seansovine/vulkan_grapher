format:
	@find src/ -regex '.*\.\(cpp\|hpp\|c\|h\|cc\|hh\|cxx\|hxx\)' -exec clang-format -style=file -i {} \;

.PHONY: build build-debug configure configure-debug

build: src
	@cmake --build build/release

build-debug: src
	@cmake --build build/debug

configure: src
	@cmake . -DCMAKE_BUILD_TYPE=Release -B build/release

configure-debug: src
	@cmake . -DCMAKE_BUILD_TYPE=Debug -B build/debug

run:
	@build/release/src/renderer-app

run-debug:
	@build/debug/src/renderer-app

meshtest:
	@build/src/dev/mesh-test

shaderc: shaders/*.frag shaders/*.vert
	@shaders/compile.sh

valgrind:
	@valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes build/debug/src/renderer-app

valgrind-release:
	@valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes build/release/src/renderer-app

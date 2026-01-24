format:
	@find src/ -regex '.*\.\(cpp\|hpp\|c\|h\|cc\|hh\|cxx\|hxx\)' -exec clang-format -style=file -i {} \;

.PHONY: build configure configure-debug
build: src
	@cmake --build build/

configure: src
	@cmake . -DCMAKE_BUILD_TYPE=Release -B build

configure-debug: src
	@cmake . -DCMAKE_BUILD_TYPE=Debug -B build

run:
	@build/src/renderer-app

meshtest:
	@build/src/dev/mesh-test

shaderc: shaders/*.frag shaders/*.vert
	@shaders/compile.sh

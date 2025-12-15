format:
	@find src/ -regex '.*\.\(cpp\|hpp\|c\|h\|cc\|hh\|cxx\|hxx\)' -exec clang-format -style=file -i {} \;

.PHONY: build
build: configure
	@cmake --build build/

.PHONY: configure
configure: src
	@cmake . -DCMAKE_BUILD_TYPE=Release -B build

run:
	@build/src/renderer-app

meshtest:
	@build/src/dev/mesh-test

shaderc: shaders/*.frag shaders/*.vert
	@shaders/compile.sh

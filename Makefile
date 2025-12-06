format:
	@find src/ -regex '.*\.\(cpp\|hpp\|c\|h\|cc\|hh\|cxx\|hxx\)' -exec clang-format -style=file -i {} \;

.PHONY: build
build: src
	@cmake --build build/

run:
	@build/src/renderer-app

shaderc: shaders/shader.frag shaders/shader.vert
	@shaders/compile.sh

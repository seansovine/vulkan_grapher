#!/usr/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if ! command -v glslangValidator &> /dev/null
then
  echo "glslangValidator was not found!"
else
  echo "Compiling shaders"
  glslangValidator -V "$SCRIPT_DIR/shader.vert" -o "$SCRIPT_DIR/vert.spv"
  glslangValidator -V "$SCRIPT_DIR/shader.frag" -o "$SCRIPT_DIR/frag.spv"
fi

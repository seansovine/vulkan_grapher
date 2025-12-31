#!/usr/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if ! command -v glslangValidator &> /dev/null
then
  echo "glslangValidator was not found!"
else
  echo "Compiling shaders"

  glslangValidator -V "$SCRIPT_DIR/wireframe.vert" -o "$SCRIPT_DIR/wireframe_vert.spv"
  glslangValidator -V "$SCRIPT_DIR/wireframe.frag" -o "$SCRIPT_DIR/wireframe_frag.spv"

  glslangValidator -V "$SCRIPT_DIR/pbr.vert" -o "$SCRIPT_DIR/pbr_vert.spv"
  glslangValidator -V "$SCRIPT_DIR/pbr.frag" -o "$SCRIPT_DIR/pbr_frag.spv"

  glslangValidator -V "$SCRIPT_DIR/pbr2.vert" -o "$SCRIPT_DIR/pbr2_vert.spv"
  glslangValidator -V "$SCRIPT_DIR/pbr2.frag" -o "$SCRIPT_DIR/pbr2_frag.spv"
fi

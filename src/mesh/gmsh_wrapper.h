#ifndef GMSH_WRAPPER_H_
#define GMSH_WRAPPER_H_

#include "mesh.h"

#include <gmsh.h>

namespace gmsh_wrapper {

struct VertsAndIndices {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

VertsAndIndices runGmsh(const std::string &functionExpr);

} // namespace gmsh_wrapper

#endif // GMSH_WRAPPER_H_

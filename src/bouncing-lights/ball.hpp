#ifndef BALL_H
#define BALL_H

#include <unordered_map>

#include "tgl/tgl.h"

extern "C" {
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/renderer.h"
#include "math/vector.h"
}

namespace BouncingLights {

class BallRenderer {
    ilG_renderman *rm = nullptr;
    ilG_mesh mesh;
    ilG_matid mat;
    GLuint mvp_loc, imt_loc, col_loc;

public:
    void free();
    bool build(ilG_renderman *rm, char **error);
    void draw(il_mat *mvp, il_mat *imt, il_vec3 *col, size_t count);
};

}

#endif

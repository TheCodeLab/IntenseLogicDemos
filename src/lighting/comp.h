#ifndef DEMO_COMP_H
#define DEMO_COMP_H

extern "C" {
#include "graphics/renderer.h"
}

class Computer {
    ilG_renderman *rm;
    ilG_matid mat;
    tgl_vao vao;
    GLuint v_vbo, n_vbo, t_vbo;
    GLuint mvp_loc, imt_loc;
    ilG_tex tex_albedo, tex_normal, tex_refraction, tex_emission;

public:
    void free();
    bool build(ilG_renderman *rm, char **error);
    void draw(il_mat mvp, il_mat imt);
};


#endif

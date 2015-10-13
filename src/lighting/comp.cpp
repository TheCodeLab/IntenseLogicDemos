#include "comp.h"

extern "C" {
#include "graphics/context.h"
#include "graphics/floatspace.h"
#include "graphics/material.h"
#include "graphics/renderer.h"
#include "graphics/transform.h"
#include "graphics/tex.h"
#include "math/matrix.h"
#include "util/log.h"
}

#include "tgl/tgl.h"

static const float cube[] = {
    // front
    -1.0, -1.0,  1.0,
     1.0, -1.0,  1.0,
     1.0,  1.0,  1.0,
     1.0,  1.0,  1.0,
    -1.0,  1.0,  1.0,
    -1.0, -1.0,  1.0,
    // top
    -1.0,  1.0,  1.0,
     1.0,  1.0,  1.0,
     1.0,  1.0, -1.0,
     1.0,  1.0, -1.0,
    -1.0,  1.0, -1.0,
    -1.0,  1.0,  1.0,
    // back
     1.0, -1.0, -1.0,
    -1.0, -1.0, -1.0,
    -1.0,  1.0, -1.0,
    -1.0,  1.0, -1.0,
     1.0,  1.0, -1.0,
     1.0, -1.0, -1.0,
    // bottom
    -1.0, -1.0, -1.0,
     1.0, -1.0, -1.0,
     1.0, -1.0,  1.0,
     1.0, -1.0,  1.0,
    -1.0, -1.0,  1.0,
    -1.0, -1.0, -1.0,
    // left
    -1.0, -1.0, -1.0,
    -1.0, -1.0,  1.0,
    -1.0,  1.0,  1.0,
    -1.0,  1.0,  1.0,
    -1.0,  1.0, -1.0,
    -1.0, -1.0, -1.0,
    // right
     1.0, -1.0,  1.0,
     1.0, -1.0, -1.0,
     1.0,  1.0, -1.0,
     1.0,  1.0, -1.0,
     1.0,  1.0,  1.0,
     1.0, -1.0,  1.0,
};

static const float cube_n[] = {
    // front
     0.0,  0.0,  1.0,
     0.0,  0.0,  1.0,
     0.0,  0.0,  1.0,
     0.0,  0.0,  1.0,
     0.0,  0.0,  1.0,
     0.0,  0.0,  1.0,
    // top
     0.0,  1.0,  0.0,
     0.0,  1.0,  0.0,
     0.0,  1.0,  0.0,
     0.0,  1.0,  0.0,
     0.0,  1.0,  0.0,
     0.0,  1.0,  0.0,
    // back
     0.0,  0.0, -1.0,
     0.0,  0.0, -1.0,
     0.0,  0.0, -1.0,
     0.0,  0.0, -1.0,
     0.0,  0.0, -1.0,
     0.0,  0.0, -1.0,
    // bottom
     0.0, -1.0,  0.0,
     0.0, -1.0,  0.0,
     0.0, -1.0,  0.0,
     0.0, -1.0,  0.0,
     0.0, -1.0,  0.0,
     0.0, -1.0,  0.0,
    // left
    -1.0,  0.0,  0.0,
    -1.0,  0.0,  0.0,
    -1.0,  0.0,  0.0,
    -1.0,  0.0,  0.0,
    -1.0,  0.0,  0.0,
    -1.0,  0.0,  0.0,
    // right
     1.0,  0.0,  0.0,
     1.0,  0.0,  0.0,
     1.0,  0.0,  0.0,
     1.0,  0.0,  0.0,
     1.0,  0.0,  0.0,
     1.0,  0.0,  0.0,
};

static const float cube_t[] = {
    // front
    0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    1.0, 1.0,
    0.0, 1.0,
    0.0, 0.0,
    // top
    0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    1.0, 1.0,
    0.0, 1.0,
    0.0, 0.0,
    // back
    0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    1.0, 1.0,
    0.0, 1.0,
    0.0, 0.0,
    // bottom
    0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    1.0, 1.0,
    0.0, 1.0,
    0.0, 0.0,
    // left
    0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    1.0, 1.0,
    0.0, 1.0,
    0.0, 0.0,
    // right
    0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    1.0, 1.0,
    0.0, 1.0,
    0.0, 0.0,
};

enum {
    ATTRIB_POSITION,
    ATTRIB_NORMAL,
    ATTRIB_TEXCOORD,
};

enum {
    TEX_ALBEDO,
    TEX_NORMAL,
    TEX_REFRACTION,
    // TEX_GLOSS,
    TEX_EMISSION,
};

void Computer::draw(il_mat mvp, il_mat imt)
{
    ilG_material *mat = ilG_renderman_findMaterial(rm, this->mat);

    ilG_tex_bind(&tex_albedo);
    ilG_tex_bind(&tex_normal);
    ilG_tex_bind(&tex_refraction);
    ilG_tex_bind(&tex_emission);

    ilG_material_bind(mat);
    tgl_vao_bind(&vao);
    ilG_material_bindMatrix(mat, mvp_loc, mvp);
    ilG_material_bindMatrix(mat, imt_loc, imt);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Computer::free()
{
    ilG_renderman_delMaterial(rm, mat);
    tgl_vao_free(&vao);
    glDeleteBuffers(1, &v_vbo);
    glDeleteBuffers(1, &n_vbo);
    glDeleteBuffers(1, &t_vbo);
}

bool Computer::build(ilG_renderman *rm, char **error)
{
    this->rm = rm;

    ilG_material m;
    ilG_material_init(&m);
    ilG_material_name(&m, "Computer Shader");
    ilG_material_arrayAttrib(&m, ATTRIB_POSITION,     "in_Position");
    ilG_material_arrayAttrib(&m, ATTRIB_NORMAL,       "in_Normal");
    ilG_material_arrayAttrib(&m, ATTRIB_TEXCOORD,     "in_Texcoord");
    ilG_material_fragData(&m, ILG_CONTEXT_ALBEDO,     "out_Albedo");
    ilG_material_fragData(&m, ILG_CONTEXT_NORMAL,     "out_Normal");
    ilG_material_fragData(&m, ILG_CONTEXT_REFRACTION, "out_Refraction");
    ilG_material_fragData(&m, ILG_CONTEXT_GLOSS,      "out_Gloss");
    ilG_material_fragData(&m, ILG_CONTEXT_EMISSION,   "out_Emission");
    ilG_material_textureUnit(&m, TEX_ALBEDO,          "tex_Albedo");
    ilG_material_textureUnit(&m, TEX_NORMAL,          "tex_Normal");
    ilG_material_textureUnit(&m, TEX_REFRACTION,      "tex_Reflect");
    ilG_material_textureUnit(&m, TEX_EMISSION,        "tex_Emission");
    if (!ilG_renderman_addMaterialFromFile(rm, m, "comp.vert", "comp.frag", &mat, error)) {
        return false;
    }
    ilG_material *mat = ilG_renderman_findMaterial(rm, this->mat);
    mvp_loc = ilG_material_getLoc(mat, "mvp");
    imt_loc = ilG_material_getLoc(mat, "imt");

    tgl_vao_init(&vao);
    tgl_vao_bind(&vao);
    glGenBuffers(1, &v_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, v_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glGenBuffers(1, &n_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, n_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_n), cube_n, GL_STATIC_DRAW);
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    glGenBuffers(1, &t_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, t_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_t), cube_t, GL_STATIC_DRAW);
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(ATTRIB_TEXCOORD);

    ilG_tex *texes[4] = {
        &tex_albedo,
        &tex_normal,
        &tex_refraction,
        &tex_emission
    };
    static const char *const files[] = {
        "comp1a.png",
        "comp1_n.png",
        "comp1_s.png",
        "comp1a_g.png"
    };
    for (unsigned i = 0; i < 4; i++) {
        ilA_imgerr err;
        extern ilA_fs demo_fs;
        if ((err = ilG_tex_loadfile(texes[i], &demo_fs, files[i]))) {
            il_error("%s", ilA_img_strerror(err));
            return false;
        }
        ilG_tex_build(texes[i]);
        texes[i]->unit = i;
    }

    return true;
}

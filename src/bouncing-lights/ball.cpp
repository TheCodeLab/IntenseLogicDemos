#include "ball.hpp"

#include "Demo.h"

extern "C" {
#include "graphics/context.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/renderer.h"
#include "graphics/transform.h"
#include "math/matrix.h"
}

enum {
    MVP = 0,
    IMT = 1
};

using namespace BouncingLights;

void BallRenderer::free()
{
    ilG_mesh_free(&mesh);
    ilG_renderman_delMaterial(rm, mat);
}

void BallRenderer::draw(il_mat *mvp, il_mat *imt, il_vec3 *col, size_t count)
{
    ilG_material *mat = ilG_renderman_findMaterial(rm, this->mat);
    ilG_mesh_bind(&mesh);
    ilG_material_bind(mat);
    for (unsigned i = 0; i < count; i++) {
        ilG_material_bindMatrix(mat, mvp_loc, mvp[i]);
        ilG_material_bindMatrix(mat, imt_loc, imt[i]);
        il_vec3 c = col[i];
        glUniform3f(col_loc, c.x, c.y, c.z);
        ilG_mesh_draw(&mesh);
    }
}

bool BallRenderer::build(ilG_renderman *rm, char **error)
{
    this->rm = rm;

    ilG_material m;
    ilG_material_init(&m);
    ilG_material_name(&m, "Ball Material");
    ilG_material_fragData(&m, ILG_CONTEXT_NORMAL, "out_Normal");
    ilG_material_fragData(&m, ILG_CONTEXT_ALBEDO, "out_Albedo");
    ilG_material_arrayAttrib(&m, ILG_MESH_POS, "in_Position");
    if (!ilG_renderman_addMaterialFromFile(rm, m, "glow.vert", "glow.frag", &mat, error)) {
        return false;
    }
    ilG_material *mat = ilG_renderman_findMaterial(rm, this->mat);
    mvp_loc = ilG_material_getLoc(mat, "mvp");
    imt_loc = ilG_material_getLoc(mat, "imt");
    col_loc = ilG_material_getLoc(mat, "col");

    if (!ilG_mesh_fromfile(&mesh, &demo_fs, "sphere.obj")) {
        return false;
    }
    if (!ilG_mesh_build(&mesh)) {
        return false;
    }

    return true;
}

#include <SDL.h>
#include <time.h>
#include <math.h>

#include "Demo.h"

extern "C" {
#include "graphics/arrayattrib.h"
#include "graphics/floatspace.h"
#include "graphics/material.h"
#include "tgl/tgl.h"
#include "graphics/renderer.h"
#include "math/matrix.h"
#include "util/log.h"
}

int main(int argc, char **argv)
{
    demoLoad(argc, argv);
    Window window = createWindow("Simple Rainbow Quad");
    ilG_renderman rm[1];
    ilG_matid mat;
    tgl_quad quad;
    tgl_vao vao;

    memset(rm, 0, sizeof(*rm));

    char *error;
    ilG_material m;
    ilG_material_init(&m);
    ilG_material_name(&m, "Rainbow Quad Shader");
    ilG_material_arrayAttrib(&m, ILG_ARRATTR_POSITION, "in_Position");
    if (!ilG_renderman_addMaterialFromFile(rm, m, "id2d.vert", "rainbow2d.frag", &mat, &error)) {
        il_error("addMaterial: %s", error);
        free(error);
        return 1;
    }
    ilG_material *mptr = ilG_renderman_findMaterial(rm, mat);
    tgl_vao_init(&vao);
    tgl_vao_bind(&vao);
    tgl_quad_init(&quad, ILG_ARRATTR_POSITION);

    ilG_material_bind(mptr);
    tgl_vao_bind(&vao);

    while (1) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                ilG_renderman_delMaterial(rm, mat);
                tgl_vao_free(&vao);
                tgl_quad_free(&quad);
                window.close();
                return 0;
            }
        }
        window.resize();
        tgl_quad_draw_once(&quad);
        window.swap();
    }
}

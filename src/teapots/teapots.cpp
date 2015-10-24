#include <SDL.h>
#include <time.h>
#include <math.h>
#include <chrono>

#include "Demo.h"
#include "Graphics.h"

extern "C" {
#include "asset/node.h"
#include "graphics/graphics.h"
#include "graphics/floatspace.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/renderer.h"
#include "graphics/tex.h"
#include "graphics/transform.h"
#include "math/matrix.h"
#include "util/log.h"
}

#ifndef M_PI
#define M_PI 3.1415926535
#endif

struct Teapot {
    ilG_renderman *rm;
    ilG_matid mat;
    ilG_mesh mesh;
    ilG_tex tex;
    GLuint mvp_loc, imt_loc;

    void free() {
        ilG_renderman_delMaterial(rm, mat);
        ilG_mesh_free(&mesh);
        ilG_tex_free(&tex);
    }
    void draw(il_mat mvp, il_mat imt) {
        ilG_material *mat = ilG_renderman_findMaterial(rm, this->mat);
        ilG_material_bind(mat);
        ilG_mesh_bind(&mesh);
        ilG_tex_bind(&tex, 0);
        ilG_material_bindMatrix(mat, mvp_loc, mvp);
        ilG_material_bindMatrix(mat, imt_loc, imt);
        ilG_mesh_draw(&mesh);
    }
    bool build(ilG_renderman *rm, char **error) {
        this->rm = rm;
        ilG_material m;
        ilG_material_init(&m);
        ilG_material_name(&m, "Teapot Material");
        ilG_material_arrayAttrib(&m, ILG_MESH_POS, "in_Position");
        ilG_material_arrayAttrib(&m, ILG_MESH_TEX, "in_Texcoord");
        ilG_material_arrayAttrib(&m, ILG_MESH_NORM, "in_Normal");
        ilG_material_arrayAttrib(&m, ILG_MESH_DIFFUSE, "in_Diffuse");
        ilG_material_arrayAttrib(&m, ILG_MESH_SPECULAR, "in_Specular");
        ilG_material_textureUnit(&m, 0, "tex");
        ilG_material_fragData(&m, ILG_GBUFFER_ALBEDO, "out_Albedo");
        ilG_material_fragData(&m, ILG_GBUFFER_NORMAL, "out_Normal");
        ilG_material_fragData(&m, ILG_GBUFFER_REFRACTION, "out_Refraction");
        ilG_material_fragData(&m, ILG_GBUFFER_GLOSS, "out_Gloss");
        if (!ilG_renderman_addMaterialFromFile(rm, m, "teapot.vert", "teapot.frag", &mat, error)) {
            return false;
        }
        ilG_material *mat = ilG_renderman_findMaterial(rm, this->mat);
        mvp_loc = ilG_material_getLoc(mat, "mvp");
        imt_loc = ilG_material_getLoc(mat, "imt");

        if (!ilG_mesh_fromfile(&mesh, &demo_fs, "teapot.obj")) {
            return false;
        }

        ilG_tex_loadfile(&tex, &demo_fs, "white-marble-texture.png");
        return true;
    }
};

struct Teapots : public Drawable {
    Teapots(unsigned object, Teapot &teapot)
        : object(object), teapot(teapot) {}

    unsigned object;
    Teapot &teapot;

    void draw(Graphics &graphics) override {
        auto mvp = graphics.objmats(&object, ILG_MVP, 1);
        auto imt = graphics.objmats(&object, ILG_IMT, 1);
        teapot.draw(mvp.front(), imt.front());
    }
    const char *name() override {
        return "Teapots";
    }
};

int main(int argc, char **argv)
{
    demoLoad(argc, argv);
    Window window = createWindow("Teapots");
    Graphics graphics(window);
    Graphics::Flags flags;
    if (!graphics.init(flags)) {
        return 1;
    }
    il_pos pos = il_pos_new(&graphics.space);
    il_pos_setPosition(&pos, il_vec3_new(0, -4, 0));
    Teapot teapot;
    Teapots drawable(pos.id, teapot);
    graphics.drawables.push_back(&drawable);

    char *error;
    if (!teapot.build(graphics.rm, &error)) {
        il_error("Teapot: %s", error);
        free(error);
        return 1;
    }

    il_pos_setPosition(&graphics.space.camera, il_vec3_new(0, 0, 20));

    il_pos lightp = il_pos_new(&graphics.space);
    il_pos_setPosition(&lightp, il_vec3_new(20, 3, 20));

    ilG_light lightl;
    lightl.color = il_vec3_new(.8f*2, .7f*2, .2f*2);
    lightl.radius = 50;

    State state;

    unsigned lightp_id = lightp.id;
    state.sunlight_locs = &lightp_id;
    state.sunlight_lights = &lightl;
    state.sunlight_count = 1;

    typedef std::chrono::steady_clock clock;
    typedef std::chrono::duration<double> duration;

    clock::time_point start = clock::now();
    while (1) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                il_log("Stopping");
                window.close();
                return 0;
            }
        }

        duration delta = clock::now() - start;
        const double secs = 5.0;
        const int scale = 20;
        il_vec3 v;
        v.x = float(sin(delta.count() * M_PI * 2.0 / secs) * scale);
        v.y = 0.f;
        v.z = float(cos(delta.count() * M_PI * 2.0 / secs) * scale);
        il_quat q = il_quat_fromAxisAngle(0, 1, 0, float(delta.count() * M_PI * 2.0 / secs));
        il_pos_setPosition(&graphics.space.camera, v);
        il_pos_setRotation(&graphics.space.camera, q);

        graphics.draw(state);
    }
}

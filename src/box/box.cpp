#include <SDL.h>
#include <time.h>
#include <math.h>
#include <chrono>

#include "Demo.h"

extern "C" {
#include "graphics/floatspace.h"
#include "graphics/material.h"
#include "graphics/renderer.h"
#include "graphics/transform.h"
#include "math/matrix.h"
#include "util/log.h"
}

#ifndef M_PI
#define M_PI 3.1415926535
#endif

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

using namespace std;

int main(int argc, char **argv)
{
    demoLoad(argc, argv);
    Window window = createWindow("Box");
    ilG_renderman rm[1];
    ilG_matid mat;
    GLuint vbo, vao;
    GLuint pos_loc;
    GLuint mvp_loc;

    memset(rm, 0, sizeof(*rm));

    char *error;
    ilG_material m;
    ilG_material_init(&m);
    ilG_material_name(&m, "Box Shader");
    ilG_material_arrayAttrib(&m, 0, "in_Position");
    ilG_material_fragData(&m, ILG_GBUFFER_ALBEDO, "out_Color");
    if (!ilG_renderman_addMaterialFromFile(rm, m, "box.vert", "box.frag", &mat, &error)) {
        il_error("Box Shader: %s", error);
        free(error);
        return 1;
    }
    ilG_material *mptr = ilG_renderman_findMaterial(rm, mat);
    pos_loc = ilG_material_getLoc(mptr, "in_Position");
    mvp_loc = ilG_material_getLoc(mptr, "mvp");

    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindVertexArray(vao);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    ilG_floatspace fs;
    ilG_floatspace_init(&fs, 1);
    fs.projection = il_mat_perspective(float(M_PI / 4.f), 4.f/3, .5f, 200.f);

    il_pos_setPosition(&fs.camera, il_vec3_new(0, 0, 5));

    il_pos boxp = il_pos_new(&fs);

    ilG_material_bind(mptr);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnable(GL_DEPTH_TEST);

    typedef chrono::steady_clock clock;
    typedef chrono::duration<double> duration;

    clock::time_point start = clock::now();
    while (1) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                il_log("Stopping");
                ilG_renderman_delMaterial(rm, mat);
                glDeleteBuffers(1, &vao);
                glDeleteBuffers(1, &vbo);
                return 0;
            }
        }

        duration delta = clock::now() - start;
        il_vec3 v;
        const float secs = 5.f;
        const float dist = 5.f;
        v.x = float(sin(delta.count() * M_PI * 2.0 / secs) * dist);
        v.y = 0.f;
        v.z = float(cos(delta.count() * M_PI * 2.0 / secs) * dist);
        il_quat q = il_quat_fromAxisAngle(0, 1, 0, float(delta.count() * M_PI * 2 / secs));
        il_pos_setPosition(&fs.camera, v);
        il_pos_setRotation(&fs.camera, q);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        il_mat mvp;
        unsigned id = boxp.id;
        ilG_floatspace_objmats(&fs, &mvp, &id, ILG_MVP, 1);
        ilG_material_bindMatrix(mptr, mvp_loc, mvp);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        window.swap();
    }
}

#include <SDL.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "Demo.h"

extern "C" {
#include "graphics/context.h"
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
    ilG_material_fragData(&m, ILG_CONTEXT_ALBEDO, "out_Color");
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
    ilG_floatspace_build(&fs, rm);
    fs.projection = il_mat_perspective(M_PI / 4.0, 4.0/3, .5, 200);

    il_pos_setPosition(&fs.camera, il_vec3_new(0, 0, 5));

    il_pos boxp = il_pos_new(&fs);

    ilG_material_bind(mptr);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnable(GL_DEPTH_TEST);

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

        struct timeval ts;
        gettimeofday(&ts, NULL);
        int secs = 5;
        float delta = ((float)(ts.tv_sec%secs) + ts.tv_usec / 1000000.0) / secs;
        il_vec3 v;
        v.x = sinf(delta * M_PI * 2) * 5;
        v.y = 0;
        v.z = cosf(delta * M_PI * 2) * 5;
        il_quat q = il_quat_fromAxisAngle(0, 1, 0, delta * M_PI * 2);
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

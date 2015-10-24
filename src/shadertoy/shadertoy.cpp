#include <uv.h>
#include <SDL.h>
#include <assert.h>
#include <chrono>

#include "Demo.h"

extern "C" {
#include "tgl/tgl.h"
#include "asset/node.h"
#include "graphics/graphics.h"
#include "graphics/material.h"
#include "graphics/renderer.h"
#include "util/log.h"
}

struct Shader {
    Shader(ilG_renderman *rm)
        : rm(rm) {}

    ilG_matid mat;
    ilG_renderman *rm;
    bool linked = false;
    /* The shadertoy.com uniforms:
      uniform vec3      iResolution;           // viewport resolution (in pixels)
      uniform float     iGlobalTime;           // shader playback time (in seconds)
      uniform float     iChannelTime[4];       // channel playback time (in seconds)
      uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)
      uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
      uniform samplerXX iChannel0..3;          // input channel. XX = 2D/Cube
      uniform vec4      iDate;                 // (year, month, day, time in seconds)
      uniform float     iSampleRate;           // sound sample rate (i.e., 44100)
    */
    GLint iResolution, iGlobalTime, iMouse;

    bool compile(char **error) {
        ilG_material *mat = ilG_renderman_findMaterial(rm, this->mat);
        ilG_shader *vert = ilG_renderman_findShader(rm, mat->vert);
        ilG_shader *frag = ilG_renderman_findShader(rm, mat->frag);
        if (ilG_shader_compile(frag, error)
            && ilG_material_link(mat, vert, frag, error)) {
            linked = true;
            iResolution = ilG_material_getLoc(mat, "iResolution");
            iGlobalTime = ilG_material_getLoc(mat, "iGlobalTime");
            iMouse = ilG_material_getLoc(mat, "iMouse");
            return true;
        }
        return false;
    }

    void bind(unsigned width, unsigned height, float time, int mouse[4]) {
        ilG_material *mat = ilG_renderman_findMaterial(rm, this->mat);
        ilG_material_bind(mat);
        glUniform2f(iResolution, GLfloat(width), GLfloat(height));
        glUniform1f(iGlobalTime, time);
        glUniform4iv(iMouse, 1, mouse);
    }
};

void event_cb(uv_fs_event_t *handle,
              const char *filename,
              int events,
              int status)
{
    (void)events, (void)status, (void)filename;
    auto &shader = *reinterpret_cast<Shader*>(handle->data);
    char *error;
    if (!shader.compile(&error)) {
        il_error("Link failed: %s", error);
        free(error);
    } else {
        il_log("Shader reloaded");
    }
}

int main(int argc, char **argv)
{
    demoLoad(argc, argv);
    Window window = createWindow("Shader Toy");
    ilA_file file;
    uv_loop_t loop;
    ilG_renderman rm[1];
    Shader shader(rm);
    tgl_quad quad;
    tgl_vao vao;
    int mouse[4];
    bool paused = false;
    uv_fs_event_t fsev;

    memset(rm, 0, sizeof(*rm));

    ilA_adddir(&demo_fs, "shadertoys", -1);
    ilG_shaders_addPath("shadertoys");
    if (demo_shader.empty()) {
        fprintf(stderr, "Pass a shader with -f\n");
        return 1;
    }
    if (!ilA_fileopen(&ilG_shaders, &file, demo_shader.c_str(), -1)) {
        ilA_printerror(&file.err);
        return 1;
    }
    il_log("Shader path: %s", file.name);

    uv_loop_init(&loop);
    uv_fs_event_init(&loop, &fsev);
    fsev.data = &shader;
    uv_fs_event_start(&fsev, event_cb, file.name, 0);

    char *error;
    ilG_material m[1];
    ilG_material_init(m);
    ilG_material_name(m, "Shader Toy");
    ilG_material_arrayAttrib(m, ILG_ARRATTR_POSITION, "in_Position");
    ilG_shader vert, frag;
    if (!ilG_shader_file(&vert, "id2d.vert", GL_VERTEX_SHADER, &error)) {
        il_error("id2d.vert: %s", error);
        free(error);
        return 1;
    }
    if (!ilG_shader_compile(&vert, &error)) {
        il_error("id2d.vert: %s", error);
        free(error);
        return 1;
    }
    ilG_shader_load(&frag, file, GL_FRAGMENT_SHADER);
    m->vert = ilG_renderman_addShader(rm, vert);
    m->frag = ilG_renderman_addShader(rm, frag);
    shader.mat = ilG_renderman_addMaterial(rm, *m);
    if (!shader.compile(&error)) {
        il_error("%s", error);
        free(error);
        return 1;
    }
    tgl_vao_init(&vao);
    tgl_vao_bind(&vao);
    tgl_quad_init(&quad, ILG_ARRATTR_POSITION);

    typedef std::chrono::steady_clock clock;
    typedef std::chrono::duration<double> duration;
    clock::time_point start_real = clock::now();
    float mono_last = 0.0, mono_start = 0.0, speed = 1.0;
    while (1) {
        uv_run(&loop, UV_RUN_NOWAIT);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                il_log("Stopping");
                uv_fs_event_stop(&fsev);
                uv_close(reinterpret_cast<uv_handle_t*>(&fsev), nullptr);
                uv_run(&loop, UV_RUN_DEFAULT);

                ilG_renderman_delMaterial(rm, shader.mat);
                tgl_vao_free(&vao);
                tgl_quad_free(&quad);
                window.close();
                return 0;
            case SDL_MOUSEMOTION:
                mouse[0] = ev.motion.x;
                mouse[1] = ev.motion.y;
                if (ev.motion.state & SDL_BUTTON_LMASK) {
                    mouse[2] = ev.motion.x;
                    mouse[3] = ev.motion.y;
                }
                break;
            case SDL_KEYDOWN:
                if (ev.key.state != SDL_PRESSED) {
                    break;
                }
                switch (ev.key.keysym.sym) {
                case SDLK_r:
                    il_log("Replay");
                    start_real = clock::now();
                    mono_start = 0.0;
                    mono_last = 0.0;
                    break;
                case SDLK_p:
                    paused = !paused;
                    il_log("%s", paused? "Paused" : "Unpaused");
                    start_real = clock::now();
                    break;
                case SDLK_LEFT:
                    speed /= 2;
                    il_log("Speed: %f", speed);
                    break;
                case SDLK_RIGHT:
                    speed *= 2;
                    il_log("Speed: %f", speed);
                    break;
                }
                break;
            }
        }

        auto s = window.resize();

        float tf;
        if (paused) {
            tf = mono_last;
        } else {
            auto now = clock::now();
            duration delta = now - start_real;
            start_real = now;
            mono_start += float(delta.count() * speed);
            tf = mono_last = mono_start;
        }
        shader.bind(s.first, s.second, tf, mouse);

        tgl_vao_bind(&vao);
        tgl_quad_draw_once(&quad);

        window.swap();
    }
}

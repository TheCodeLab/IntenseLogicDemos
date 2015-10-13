#include <uv.h>
#include <SDL.h>
#include <assert.h>

extern "C" {
#include "tgl/tgl.h"
#include "asset/node.h"
#include "graphics/context.h"
#include "graphics/graphics.h"
#include "graphics/material.h"
#include "graphics/renderer.h"
#include "util/timer.h"
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
        glUniform2f(iResolution, width, height);
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

extern const char *demo_shader;
extern ilA_fs demo_fs;
extern "C" void demo_start()
{
    SDL_Window *window;
    SDL_GLContext context;
    ilA_file file;
    uv_loop_t loop;
    ilG_renderman rm[1];
    Shader shader(rm);
    tgl_quad quad;
    tgl_vao vao;
    struct timeval start_real;
    int mouse[4];
    bool paused = false;
    float mono_last, mono_start, speed = 1.0;
    uv_fs_event_t fsev;

    memset(rm, 0, sizeof(*rm));

    ilA_adddir(&demo_fs, "shadertoys", -1);
    ilG_shaders_addPath("shadertoys");
    if (!ilA_fileopen(&ilG_shaders, &file, demo_shader, -1)) {
        ilA_printerror(&file.err);
        return;
    }
    il_log("Shader path: %s", file.name);

    uv_loop_init(&loop);
    uv_fs_event_init(&loop, &fsev);
    fsev.data = &shader;
    uv_fs_event_start(&fsev, event_cb, file.name, file.namelen);

    unsigned msaa = 0;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, msaa != 0);
    if (msaa) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    rm->width = 800;
    rm->height = 800;
    window = SDL_CreateWindow(
        "Shader Toy",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        rm->width, rm->height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        il_error("SDL_CreateWindow: %s", SDL_GetError());
        return;
    }
    context = SDL_GL_CreateContext(window);
    if (!context) {
        il_error("SDL_GL_CreateContext: %s", SDL_GetError());
        return;
    }
    if (epoxy_gl_version() < 32) {
        il_error("Expected GL 3.2, got %u", epoxy_gl_version());
        return;
    }

    char *error;
    ilG_material m[1];
    ilG_material_init(m);
    ilG_material_name(m, "Shader Toy");
    ilG_material_arrayAttrib(m, ILG_ARRATTR_POSITION, "in_Position");
    ilG_shader vert, frag;
    if (!ilG_shader_file(&vert, "id2d.vert", GL_VERTEX_SHADER, &error)) {
        il_error("id2d.vert: %s", error);
        free(error);
        return;
    }
    if (!ilG_shader_compile(&vert, &error)) {
        il_error("id2d.vert: %s", error);
        free(error);
        return;
    }
    ilG_shader_load(&frag, file, GL_FRAGMENT_SHADER);
    m->vert = ilG_renderman_addShader(rm, vert);
    m->frag = ilG_renderman_addShader(rm, frag);
    shader.mat = ilG_renderman_addMaterial(rm, *m);
    if (!shader.compile(&error)) {
        il_error("%s", error);
        free(error);
        return;
    }
    tgl_vao_init(&vao);
    tgl_vao_bind(&vao);
    tgl_quad_init(&quad, ILG_ARRATTR_POSITION);
    gettimeofday(&start_real, NULL);

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
                return;
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
                    gettimeofday(&start_real, NULL);
                    mono_start = 0.0;
                    mono_last = 0.0;
                    break;
                case SDLK_p:
                    paused = !paused;
                    il_log("%s", paused? "Paused" : "Unpaused");
                    gettimeofday(&start_real, NULL);
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

        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);

        struct timeval now, tv;
        float tf;
        if (paused) {
            tf = mono_last;
        } else {
            gettimeofday(&now, NULL);
            timersub(&now, &start_real, &tv);
            start_real = now;
            mono_start += ((float)tv.tv_sec + (float)tv.tv_usec/1000000.0) * speed;
            tf = mono_last = mono_start;
        }
        shader.bind(width, height, tf, mouse);

        tgl_vao_bind(&vao);
        tgl_quad_draw_once(&quad);

        SDL_GL_SwapWindow(window);
    }
}

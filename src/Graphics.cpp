#include "Graphics.h"

extern "C" {
#include "graphics/transform.h"
#include "util/logger.h"
#include "util/log.h"
}

using namespace std;

void Graphics::free()
{
    ilG_shape_free(&box);
    ilG_shape_free(&ico);
    ilG_skybox_free(&skybox);
    ilG_ambient_free(&ambient);
    ilG_lighting_free(&sun);
    ilG_lighting_free(&point);
    ilG_tonemapper_free(&tonemapper);

    initialized = false;
}

bool Graphics::init(const Flags &flags)
{
    if (initialized) {
        free();
    } else {
        ilG_renderman_init(rm);
        ilG_floatspace_init(&space, 2);
    }
    initialized = true;

    ilG_renderman_setup(rm, flags.msaa, flags.hdr);
    ilG_renderman_resize(rm, 800, 600);
    ilG_floatspace_build(&space, rm);
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);

    ilG_box(&box);
    ilG_icosahedron(&ico);

    {
        ilG_tex skytex;
        ilA_img skyfaces[6];
        static const char *const files[6] = {
            "north.png",
            "south.png",
            "up.png",
            "down.png",
            "west.png",
            "east.png"
        };
        bool cont = true;
        for (unsigned i = 0; i < 6; i++) {
            ilA_imgerr res = ilA_img_loadfile(skyfaces + i, &demo_fs, files[i]);
            if (res) {
                il_log("%s: %s", files[i], ilA_img_strerror(res));
                cont = false;
                break;
            }
        }
        if (!cont) {
            return false;
        }
        ilG_tex_loadcube(&skytex, skyfaces);
        char *error;
        if (!ilG_skybox_build(&skybox, rm, skytex, &box, &error)) {
            il_error("skybox: %s", error);
            ::free(error);
            return false;
        }
    }

    char *error;
    if (!ilG_ambient_build(&ambient, rm, &error)) {
        il_error("ambient: %s", error);
        ::free(error);
        return false;
    }
    if (!ilG_lighting_build(&sun, rm, &ico, ILG_SUN, flags.msaa, &error)) {
        il_error("sunlighting: %s", error);
        ::free(error);
        return false;
    }
    if (!ilG_lighting_build(&point, rm, &ico, ILG_POINT, flags.msaa, &error)) {
        il_error("lighting: %s", error);
        ::free(error);
        return false;
    }
    if (!ilG_tonemapper_build(&tonemapper, rm, flags.msaa, &error)) {
        il_error("tonemapper: %s", error);
        ::free(error);
        return false;
    }
    return true;
}

void Graphics::draw(State &state)
{
    SDL_SetWindowGrab(window.window, SDL_bool(state.mouse_grab));
    SDL_SetRelativeMouseMode(SDL_bool(state.mouse_grab));
    SDL_GL_SetSwapInterval(state.vsync);

    auto s = window.resize();
    unsigned width = s.first, height = s.second;
    if (unsigned(width) != rm->width || unsigned(height) != rm->height) {
        ilG_renderman_resize(rm, width, height);
        ilG_tonemapper_resize(&tonemapper, width, height);
    }
    space.projection = il_mat_perspective(state.fov, width / float(height), state.zmin, state.zfar);

    il_mat skybox_vp = viewmat(ILG_VIEW_R | ILG_PROJECTION);
    auto slocs = state.sunlight_locs;
    auto plocs = state.point_locs;
    std::vector<il_mat>
        /* ILG_INVERSE | ILG_VIEW_R | ILG_PROJECTION
           ILG_MODEL_T | ILG_VIEW_T
           ILG_MODEL_T | ILG_VP */
        sun_ivp = objmats(slocs, ILG_INVERSE | ILG_VIEW_R | ILG_PROJECTION, state.sunlight_count),
        sun_mv  = objmats(slocs, ILG_MODEL_T | ILG_VIEW_T,                  state.sunlight_count),
        sun_vp  = objmats(slocs, ILG_MODEL_T | ILG_VP,                      state.sunlight_count),
        pnt_ivp = objmats(plocs, ILG_INVERSE | ILG_VIEW_R | ILG_PROJECTION, state.point_count),
        pnt_mv  = objmats(plocs, ILG_MODEL_T | ILG_VIEW_T,                  state.point_count),
        pnt_vp  = objmats(plocs, ILG_MODEL_T | ILG_VP,                      state.point_count);

    const float fovsquared = state.fov * state.fov;
    ambient.color = state.ambient_col;
    ambient.fovsquared = fovsquared;
    sun.gbuffer = &rm->gbuffer;
    sun.accum = &rm->accum;
    sun.width = width;
    sun.height = height;
    sun.fovsquared = fovsquared;
    point.gbuffer = &rm->gbuffer;
    point.accum = &rm->accum;
    point.width = width;
    point.height = height;
    point.fovsquared = fovsquared;
    tonemapper.exposure = state.exposure;
    tonemapper.gamma = state.gamma;

#define push(n) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, n)
#define pop() glPopDebugGroup()
#define with(n) for (bool cont = (push(n), true); cont; pop(), cont = false)

    with("Geometry") {
        ilG_geometry_bind(&rm->gbuffer);
    }
    with("Skybox") {
        ilG_skybox_draw(&skybox, skybox_vp);
    }
    for (auto &d : drawables) {
        with(d->name()) {
            d->draw(*this);
        }
    }
    with("Ambient Lighting") {
        ilG_ambient_draw(&ambient);
    }
    with("Sunlights") {
        ilG_lighting_draw(&sun, sun_ivp.data(), sun_mv.data(), sun_vp.data(),
                          state.sunlight_lights, state.sunlight_count);
    }
    with("Point Lights") {
        ilG_lighting_draw(&point, pnt_ivp.data(), pnt_mv.data(), pnt_vp.data(),
                          state.point_lights, state.point_count);
    }
    with("Tone Mapping") {
        ilG_tonemapper_draw(&tonemapper);
    }
    window.swap();

#undef push
#undef pop
#undef with
}

il_mat Graphics::viewmat(int type)
{
    return ilG_floatspace_viewmat(&space, type);
}

std::vector<il_mat> Graphics::objmats(unsigned *objects, int type, unsigned count)
{
    std::vector<il_mat> mats;
    mats.reserve(count);
    ilG_floatspace_objmats(&space, mats.data(), objects, type, count);
    return mats;
}

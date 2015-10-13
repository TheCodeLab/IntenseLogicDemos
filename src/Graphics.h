#ifndef DEMO_GRAPHICS_H
#define DEMO_GRAPHICS_H

#include <SDL.h>
#include <vector>
#include <string>

#include "Demo.h"

extern "C" {
#include "graphics/renderer.h"
#include "graphics/floatspace.h"
}

struct State {
    bool mouse_grab = false;
    bool vsync = true;
    il_mat projection;
    float fov = M_PI / 4;
    float zmin = 0.5, zfar = 1024.0;
    il_vec3 ambient_col = il_vec3_new(.2,.2,.2);
    float exposure = 1.0, gamma = 1.0;

    unsigned *sunlight_locs = nullptr;
    ilG_light *sunlight_lights = nullptr;
    size_t sunlight_count = 0;
    unsigned *point_locs = nullptr;
    ilG_light *point_lights = nullptr;
    size_t point_count = 0;
};

class Graphics;

class Drawable {
public:
    virtual void draw(Graphics &graphics) = 0;
    virtual const char *name() {
        return "Untitled";
    }
};

class Graphics {
public:
    struct Flags {
        bool debug = false;
        bool srgb = false;
        bool hdr = true;
        unsigned msaa = 0;
    };

    Graphics(Window &window)
        : window(window) {}

    void free();
    bool init(const Flags &flags);
    void draw(State &state);
    il_mat viewmat(int type);
    std::vector<il_mat> objmats(unsigned *objects, int type, unsigned count);

    Window &window;
    ilG_renderman rm[1];
    ilG_floatspace space;
    ilG_shape box, ico;
    ilG_skybox skybox;
    std::vector<Drawable*> drawables;
    ilG_ambient ambient;
    ilG_lighting sun, point;
    ilG_tonemapper tonemapper;
    bool initialized = false;
};

#endif

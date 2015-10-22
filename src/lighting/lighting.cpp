#include <SDL.h>
#include <time.h>
#include <math.h>
#include <chrono>

#include "Graphics.h"
#include "comp.h"

extern "C" {
#include "graphics/transform.h"
#include "util/log.h"
}

#ifndef M_PI
#define M_PI 3.1415926535
#endif

class ComputerRenderer : public Drawable {
public:
    ComputerRenderer(unsigned object, Computer &comp)
        : object(object), comp(comp) {}

    void draw(Graphics &graphics) override {
        auto mvp = graphics.objmats(&object, ILG_MVP, 1);
        auto imt = graphics.objmats(&object, ILG_IMT, 1);
        comp.draw(mvp.front(), imt.front());
    }

private:
    unsigned object;
    Computer &comp;
};

int main(int argc, char **argv)
{
    demoLoad(argc, argv);
    auto window = createWindow("Lighting");
    Graphics graphics(window);
    Graphics::Flags flags;
    if (!graphics.init(flags)) {
        return 1;
    }
    Computer comp;
    char *error;
    if (!comp.build(graphics.rm, &error)) {
        il_error("Computer: %s", error);
        free(error);
        return 1;
    }
    il_pos compp = il_pos_new(&graphics.space);
    ComputerRenderer compr(compp.id, comp);
    graphics.drawables.push_back(&compr);

    il_pos_setPosition(&graphics.space.camera, il_vec3_new(0, 0, 20));

    il_pos lightp = il_pos_new(&graphics.space);
    il_pos_setPosition(&lightp, il_vec3_new(20, 3, 20));

    ilG_light lightl;
    lightl.color = il_vec3_new(.8f*2, .7f*2, .2f*2);
    lightl.radius = 50;

    State state;

    unsigned lightp_id = lightp.id;
    state.sunlight_lights = &lightl;
    state.sunlight_locs = &lightp_id;
    state.sunlight_count = 1;

    typedef std::chrono::steady_clock clock;
    typedef std::chrono::duration<double> duration;

    clock::time_point start = clock::now();
    while (1) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                return 0;
            }
        }

        duration delta = clock::now() - start;
        int secs = 10;
        const float dist = 10.0;
        il_vec3 v;
        v.x = float(sin(delta.count() * M_PI * 2 / secs) * dist);
        v.y = 0.f;
        v.z = float(cos(delta.count() * M_PI * 2 / secs) * dist);
        il_quat q = il_quat_fromAxisAngle(0, 1, 0, float(delta.count() * M_PI * 2 / secs));
        il_pos_setPosition(&graphics.space.camera, v);
        il_pos_setRotation(&graphics.space.camera, q);

        graphics.draw(state);
    }
}

#ifndef DEMO_H
#define DEMO_H

#include <SDL.h>
#include <utility>
#include <string>

#include "tgl/tgl.h"

extern "C" {
#include "asset/node.h"
}

void demoLoad(int argc, char **argv);

struct Window {
    SDL_Window *window;
    SDL_GLContext context;

    std::pair<int, int> resize() {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);
        return std::make_pair(width, height);
    }
    void close() {
        SDL_DestroyWindow(window);
    }
    void swap() {
        SDL_GL_SwapWindow(window);
    }
};
Window createWindow(const char *title, unsigned msaa = 0);

extern ilA_fs demo_fs;
extern std::string demo_shader;

#endif

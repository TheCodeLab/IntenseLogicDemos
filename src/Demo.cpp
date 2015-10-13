#include "Demo.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fenv.h>
#include <vector>
#include <string>

#include "tgl/tgl.h"

extern "C" {
#include "util/logger.h"
#include "util/log.h"
#include "util/version.h"
#include "util/opt.h"
#include "graphics/graphics.h"
}

using namespace std;

enum ArgType {
    NO_ARG,
    REQUIRED,
    OPTIONAL
};

const struct {
    ArgType arg;
    char s;
    const char *l, *h;
} help[] = {
    {NO_ARG,    'h', "help",    "Prints this message and exits"},
    {NO_ARG,    'v', "version", "Prints the version and exits"},
    {REQUIRED,  'd', "data",    "Adds a directory to look for data files"},
    {REQUIRED,  's', "shaders", "Adds a directory to look for GLSL shaders"},
    {REQUIRED,  'f', "shader",  "ShaderToy demo: Select shader to load"},
    {NO_ARG,      0, "fpe",     "Enable trapping on floating point exceptions"},
    {NO_ARG,      0, NULL,      NULL}
};

void demoLoad(int argc, char **argv)
{
    size_t i;
    il_opts opts = il_opt_parse(argc, argv);
    il_modopts *main_opts = il_opts_lookup(&opts, const_cast<char*>(""));

    ilA_adddir(&demo_fs, "assets", -1);

    for (i = 0; main_opts && i < main_opts->args.length; i++) {
        il_opt *opt = &main_opts->args.data[i];
        char *arg = strndup(opt->arg.str, opt->arg.len);
#define option(s, l) if (il_string_cmp(opt->name, il_string_new(const_cast<char*>(s))) || \
                         il_string_cmp(opt->name, il_string_new(const_cast<char*>(l))))
        option("h", "help") {
            printf("IntenseLogic %s\n", il_version);
            printf("Usage: %s [OPTIONS]\n\n", argv[0]);
            printf("Options:\n");
            for (i = 0; help[i].l; i++) {
                static const char *const arg_strs[] = {
                    "",
                    "[=arg]",
                    "=arg"
                };
                char longbuf[64];
                sprintf(longbuf, "%s%s%s",
                        help[i].l? "-" : " ",
                        help[i].l? help[i].l : "",
                        arg_strs[help[i].arg]
                );
                printf(" %c%c %-18s %s\n",
                       help[i].s? '-' : ' ',
                       help[i].s? help[i].s : ' ',
                       longbuf,
                       help[i].h
                );
            }
            exit(0);
        }
        option("v", "version") {
            printf("IntenseLogic %s\n", il_version);
            printf("Commit: %s\n", il_commit);
            printf("Built: %s\n", il_build_date);
            exit(0);
        }
        option("d", "data") {
            ilA_adddir(&demo_fs, arg, -1);
        }
        option("s", "shaders") {
            ilG_shaders_addPath(arg);
        }
        option("f", "shader") {
            demo_shader = arg;
            continue; // don't free arg
        }
        option("", "fpe") {
            feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
            il_log("Floating point exceptions enabled");
        }
        free(arg);
    }

    ilG_shaders_addPath("shaders");
    ilG_shaders_addPath("IntenseLogic/shaders");

    il_log("Initializing engine.");
    il_log("IntenseLogic %s", il_version);
    il_log("IL Commit: %s", il_commit);
    il_log("Built %s", il_build_date);

    il_load_ilgraphics();
}

struct DebugGroupStack {
    vector<string> entries;
};

static GLvoid error_cb(GLenum esource, GLenum etype, GLuint id, GLenum eseverity,
                       GLsizei length, const GLchar* message, const GLvoid* user)
{
    auto &stack = *reinterpret_cast<DebugGroupStack*>(const_cast<void*>(user));
    const char *ssource;
    switch(esource) {
        case GL_DEBUG_SOURCE_API_ARB:               ssource=" API";              break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:     ssource=" Window System";    break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:   ssource=" Shader Compiler";  break;
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:       ssource=" Third Party";      break;
        case GL_DEBUG_SOURCE_APPLICATION_ARB:       ssource=" Application";      break;
        case GL_DEBUG_SOURCE_OTHER_ARB:             ssource="";            break;
        default: ssource="???";
    }
    const char *stype;
    switch(etype) {
        case GL_DEBUG_TYPE_ERROR_ARB:               stype=" error";                 break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: stype=" deprecated behaviour";  break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  stype=" undefined behaviour";   break;
        case GL_DEBUG_TYPE_PORTABILITY_ARB:         stype=" portability issue";     break;
        case GL_DEBUG_TYPE_PERFORMANCE_ARB:         stype=" performance issue";     break;
        case GL_DEBUG_TYPE_OTHER_ARB:               stype="";                       break;
        case GL_DEBUG_TYPE_PUSH_GROUP: {
            stack.entries.push_back(message);
            return;
        }
        case GL_DEBUG_TYPE_POP_GROUP: {
            stack.entries.pop_back();
            return;
        }
        default: stype="???";
    }
    const char *sseverity;
    switch(eseverity) {
        case GL_DEBUG_SEVERITY_HIGH_ARB:    sseverity="high";   break;
        case GL_DEBUG_SEVERITY_MEDIUM_ARB:  sseverity="medium"; break;
        case GL_DEBUG_SEVERITY_LOW_ARB:     sseverity="low";    break;
        default: sseverity="???";
    }
    string msg(message, length);
    if (msg.back() == '\n') {
        msg.pop_back();
    }

    string groups;
    if (!stack.entries.empty()) {
        groups += " in " + stack.entries.front();
        for (auto it = stack.entries.begin()+1; it != stack.entries.end(); it++) {
            groups += ".";
            groups += *it;
        }
    }

    string buf = string(sseverity) + stype + " #" + to_string(id) + groups + ": " + msg;
    string source = string("OpenGL") + ssource;

    il_logmsg lmsg;
    memset(&lmsg, 0, sizeof(il_logmsg));
    lmsg.level = IL_NOTIFY;
    lmsg.msg = il_string_bin(const_cast<char*>(buf.data()), buf.size());
    lmsg.func = il_string_bin(const_cast<char*>(source.data()), source.size());

    il_logger_log(il_logger_cur(), lmsg);
}

Window createWindow(const char *title, unsigned msaa)
{
    Window window;
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
    window.window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window.window) {
        il_error("SDL_CreateWindow: %s", SDL_GetError());
        exit(1);
    }
    window.context = SDL_GL_CreateContext(window.window);
    if (!window.context) {
        il_error("SDL_GL_CreateContext: %s", SDL_GetError());
        exit(1);
    }
    if (epoxy_gl_version() < 32) {
        il_error("Expected GL 3.2, got %u", epoxy_gl_version());
        exit(1);
    }

    if (TGL_EXTENSION(KHR_debug)) {
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PUSH_GROUP, GL_DONT_CARE, 0, NULL, true);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_POP_GROUP, GL_DONT_CARE, 0, NULL, true);
        glDebugMessageCallback(&error_cb, new DebugGroupStack);
        glEnable(GL_DEBUG_OUTPUT);
        il_log("KHR_debug present, enabling advanced errors");
        tgl_check("glDebugMessageCallback()");
    } else {
        il_log("KHR_debug missing");
    }

    return window;
}

ilA_fs demo_fs;
const char *demo_shader;

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btIDebugDraw.h>
#include <vector>

#include "tgl/tgl.h"

extern "C" {
#include "graphics/renderer.h"
#include "graphics/material.h"
}

namespace BouncingLights {

struct Vertex {
    Vertex(const btVector3 &pos, const btVector3 &col) :
        pos(pos),
        col(col)
    {}
    btVector3 pos;
    btVector3 col;
};

class DebugDraw : public btIDebugDraw {
    ilG_matid mat;
    std::vector<Vertex> lines;
    GLuint vbo, vao;
    GLuint vp_loc;
    int debugMode = DBG_DrawAabb;
    ilG_renderman *rm = nullptr;

public:
    void free();
    bool build(ilG_renderman *rm, char **error);
    void draw(il_mat vp);
    // call before running bullet
    void begin() {
        lines.clear();
    }

    // btIDebugDraw
    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;
    void drawLine(const btVector3 &from, const btVector3 &to,
                  const btVector3 &fromColor, const btVector3 &toColor) override;
    void reportErrorWarning(const char *str) override;
    void draw3dText(const btVector3 &location, const char *textString) override;
    void setDebugMode(int debugMode) override;
    int getDebugMode() const override;
    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance,
                          int lifeTime, const btVector3 &color) override;
};

}

#include "debugdraw.hpp"

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btIDebugDraw.h>

using namespace std;
using namespace BouncingLights;

extern "C" {
#include "graphics/transform.h"
#include "math/matrix.h"
#include "graphics/renderer.h"
#include "graphics/arrayattrib.h"
#include "util/log.h"
}

void DebugDraw::draw(il_mat vp)
{
    ilG_material *mat = ilG_renderman_findMaterial(rm, this->mat);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    ilG_material_bind(mat);
    ilG_material_bindMatrix(mat, vp_loc, vp);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(Vertex), lines.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, lines.size());
}

void DebugDraw::free()
{
    ilG_renderman_delMaterial(rm, mat);
}

bool DebugDraw::build(ilG_renderman *rm, char **error)
{
    this->rm = rm;

    ilG_material m;
    ilG_material_init(&m);
    ilG_material_name(&m, "Bullet Line Renderer");
    ilG_material_arrayAttrib(&m, ILG_ARRATTR_POSITION, "in_Position");
    ilG_material_arrayAttrib(&m, ILG_ARRATTR_AMBIENT, "in_Ambient");
    if (!ilG_renderman_addMaterialFromFile(rm, m, "bullet-debug.vert", "bullet-debug.frag", &mat, error)) {
        return false;
    }
    vp_loc = ilG_material_getLoc(ilG_renderman_findMaterial(rm, mat), "vp");

    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindVertexArray(vao);
    glVertexAttribPointer(ILG_ARRATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, pos));
    glVertexAttribPointer(ILG_ARRATTR_AMBIENT, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, col));
    glEnableVertexAttribArray(ILG_ARRATTR_POSITION);
    glEnableVertexAttribArray(ILG_ARRATTR_AMBIENT);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

    return true;
}

void DebugDraw::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
{
    lines.push_back(Vertex(from, color));
    lines.push_back(Vertex(to, color));
}

void DebugDraw::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor)
{
    lines.push_back(Vertex(from, fromColor));
    lines.push_back(Vertex(to, toColor));
}

void DebugDraw::reportErrorWarning(const char *str)
{
    il_warning("bullet: %s", str);
}

void DebugDraw::draw3dText(const btVector3 &location, const char *textString)
{
    (void)location; (void)textString;
}

void DebugDraw::setDebugMode(int mode)
{
    debugMode = mode;
}

int DebugDraw::getDebugMode() const
{
    return debugMode;
}

void DebugDraw::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color)
{
    (void)PointOnB; (void)normalOnB; (void)distance; (void)lifeTime; (void)color;
}

#include "bulletspace.hpp"

#include <cassert>

extern "C" {
#include "graphics/transform.h"
}

using namespace BouncingLights;

il_mat BulletSpace::viewmat(int type)
{
    il_mat m = type & ILG_PROJECTION? projection : il_mat_identity();
    btTransform camera = ghost.getWorldTransform();
    btQuaternion rot = camera.getRotation();
    if (type & ILG_VIEW_R) {
        m = il_mat_mul(m, il_mat_rotate(il_quat_new(rot.x(),rot.y(),rot.z(),rot.w())));
    }
    btVector3 pos = camera.getOrigin();
    if (type & ILG_VIEW_T) {
        il_vec4 v = il_vec4_new(pos.x(),pos.y(),pos.z(), 1.0);
        v.x = -v.x;
        v.y = -v.y;
        v.z = -v.z;
        m = il_mat_mul(m, il_mat_translate(v));
    }
    if (type & ILG_INVERSE) {
        m = il_mat_invert(m);
    }
    if (type & ILG_TRANSPOSE) {
        m = il_mat_transpose(m);
    }
    return m;
}

void BulletSpace::objmats(il_mat *out, BodyID *in, int type, size_t count)
{
#define mattype(matty) for (unsigned i = 0; i < count && (type & matty); i++)
    il_mat proj = projection;
    for (unsigned i = 0; i < count && !(type & ILG_PROJECTION); i++) {
        out[i] = il_mat_identity();
    }
    mattype(ILG_PROJECTION) {
        out[i] = proj;
    }
    btTransform camera = ghost.getWorldTransform();
    btQuaternion rot = camera.getRotation();
    il_mat viewr = il_mat_rotate(il_quat_new(rot.x(),rot.y(),rot.z(),rot.w()));
    mattype(ILG_VIEW_R) {
        out[i] = il_mat_mul(out[i], viewr);
    }
    btVector3 pos = camera.getOrigin();
    il_vec4 viewt_v = il_vec4_new(pos.x(),pos.y(),pos.z(), 1.0);
    viewt_v.x = -viewt_v.x;
    viewt_v.y = -viewt_v.y;
    viewt_v.z = -viewt_v.z;
    il_mat viewt = il_mat_translate(viewt_v);
    mattype(ILG_VIEW_T) {
        out[i] = il_mat_mul(out[i], viewt);
    }
    mattype(ILG_MODEL_T) {
        il_mat modelt = il_mat_translate(il_vec3_to_vec4(this->pos(in[i].value()), 1.0));
        out[i] = il_mat_mul(out[i], modelt);
    }
    mattype(ILG_MODEL_R) {
        il_mat modelr = il_mat_rotate(this->rot(in[i].value()));
        out[i] = il_mat_mul(out[i], modelr);
    }
    mattype(ILG_MODEL_S) {
        il_mat models = il_mat_scale(il_vec3_to_vec4(scale[in[i].value()], 1.0));
        out[i] = il_mat_mul(out[i], models);
    }
    mattype(ILG_INVERSE) {
        out[i] = il_mat_invert(out[i]);
    }
    mattype(ILG_TRANSPOSE) {
        out[i] = il_mat_transpose(out[i]);
    }
}

il_vec3 BulletSpace::pos(unsigned id)
{
    btVector3 vec = trans[id].getOrigin();
    il_vec3 v;
    v.x = vec.getX();
    v.y = vec.getY();
    v.z = vec.getZ();
    return v;
}

il_quat BulletSpace::rot(unsigned id)
{
    btQuaternion rot = trans[id].getRotation();
    il_quat q;
    q.x = rot.getX();
    q.y = rot.getY();
    q.z = rot.getZ();
    q.w = rot.getW();
    return q;
}

BulletSpace::BulletSpace(btPairCachingGhostObject &ghost,
                         btDispatcher *dispatcher,
                         btBroadphaseInterface *cache,
                         btConstraintSolver *solver,
                         btCollisionConfiguration *config)
    : world(dispatcher, cache, solver, config),
      ghost(ghost) {}

BulletSpace::BodyID BulletSpace::add(const btRigidBody::btRigidBodyConstructionInfo &info)
{
    if (freelist.size() > 0) {
        unsigned i = freelist.back();
        bodies[i] = btRigidBody(info);
        freelist.pop_back();
        world.addRigidBody(&bodies[i]);
        return BulletSpace::BodyID(i);
    }
    bodies.emplace_back(btRigidBody(info));
    world.addRigidBody(&bodies.back());
    scale.resize(bodies.size(), il_vec3_new(1,1,1));
    trans.resize(bodies.size());
    return BulletSpace::BodyID(bodies.size() - 1);
}

void BulletSpace::del(BulletSpace::BodyID body)
{
    world.removeRigidBody(&bodies.at(body.id));
    freelist.push_back(body.id);
    trans.at(body.id).setIdentity();
}

int BulletSpace::step(float by, int maxsubs, float fixed)
{
    int res = world.stepSimulation(by, maxsubs, fixed);

    assert(bodies.size() == trans.size());
    unsigned i = 0;
    for (auto it = bodies.begin(); it != bodies.end(); it++, i++) {
        it->getMotionState()->getWorldTransform(trans[i]);
    }
    return res;
}

#ifndef BULLETSPACE_H
#define BULLETSPACE_H

#include <vector>
#include <deque>
#include <memory>
#include <mutex>
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

extern "C" {
#include "math/matrix.h"
}

namespace BouncingLights {

struct BulletSpace {
    std::deque<btRigidBody> bodies;
    std::vector<btTransform> trans;
    std::vector<il_vec3> scale;
    std::vector<unsigned> freelist;

    il_vec3 pos(unsigned id);
    il_quat rot(unsigned id);

public:
    class BodyID {
        unsigned id;
        friend BulletSpace;

    public:
        BodyID(unsigned id) : id(id) {}

        unsigned value() const {
            return id;
        }
    };

    BulletSpace(btPairCachingGhostObject &ghost, btDispatcher *dispatcher,
                btBroadphaseInterface *cache, btConstraintSolver *solver,
                btCollisionConfiguration *config);

    BodyID add(const btRigidBody::btRigidBodyConstructionInfo &info);
    void del(BodyID id);
    int step(float by, int maxsubs = 1, float fixed = 1/60.f);
    il_mat viewmat(int type);
    void objmats(il_mat *out, BodyID *in, int type, size_t count);
    btRigidBody &getBody(BodyID id) {
        return bodies[id.value()];
    }
    void setBodyScale(BodyID id, il_vec3 v) {
        scale[id.value()] = v;
    }

    btDiscreteDynamicsWorld world;
    btPairCachingGhostObject &ghost;
    il_mat projection;
};

}

#endif

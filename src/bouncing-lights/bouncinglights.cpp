#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <SDL.h>

#include <iostream>
#include <utility>
#include <ctime>
#include <math.h>

#include "tgl/tgl.h"
#include "debugdraw.hpp"
#include "bulletspace.hpp"
#include "ball.hpp"
#include "Demo.h"
#include "Graphics.h"

using namespace std;
using namespace BouncingLights;

extern "C" {
#include "asset/image.h"
#include "graphics/transform.h"
#include "math/matrix.h"
#include "graphics/context-internal.h"
#include "graphics/graphics.h"
#include "graphics/renderer.h"
#include "graphics/tex.h"
#include "util/log.h"
}

#ifndef M_PI
#define M_PI 3.1415926535
#endif

const btScalar arenaWidth = 128;

float rand_float(unsigned *seedp)
{
    return (float)rand_r(seedp) / RAND_MAX;
}

struct Spheres {
    vector<BulletSpace::BodyID> bodies;
    vector<il_vec3> colors;
    vector<ilG_light> lights;
};

Spheres add_objects(BulletSpace &bs, btCollisionShape *shape, size_t count, unsigned *seedp)
{
    Spheres spheres;
    for (size_t i = 0; i < count; i++) {
        // Physics body
        btVector3 position
            (rand_r(seedp) % 128,
             rand_r(seedp) % 32 + 50,
             rand_r(seedp) % 128);
        auto state = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), position));
        float mass = 1.f;
        btVector3 inertia(0,0,0);
        shape->calculateLocalInertia(mass, inertia);
        btRigidBody::btRigidBodyConstructionInfo ballRigidBodyCI(mass, state, shape, inertia);
        auto id = bs.add(ballRigidBodyCI);
        bs.getBody(id).setRestitution(1.0);

        // Color
        float brightness = rand_float(seedp) + 1;
        auto col = il_vec3_new(
            rand_float(seedp) * brightness,
            rand_float(seedp) * brightness,
            rand_float(seedp) * brightness);

        // Light
        ilG_light light;
        light.color = col;
        light.radius = brightness * 10;

        spheres.bodies.push_back(id);
        spheres.colors.push_back(col);
        spheres.lights.push_back(light);
    }
    return spheres;
}

int main(int argc, char **argv)
{
    demoLoad(argc, argv);
    auto window = createWindow("Bouncing Lights");
    Graphics graphics(window);
    Graphics::Flags flags;
    if (!graphics.init(flags)) {
        return 1;
    }

    // Create character controller
    ///////////////////////////////
    btSphereShape playerShape(1);
    btPairCachingGhostObject ghostObject;
    ghostObject.setWorldTransform(btTransform(btQuaternion(0,0,0,1), btVector3(64, 50, 64)));
    ghostObject.setCollisionShape(&playerShape);
    ghostObject.setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
    btKinematicCharacterController player(&ghostObject, &playerShape, .5, 2);
    player.setGravity(0);

    // Create world
    ////////////////
    btDbvtBroadphase broadphase;
    btGhostPairCallback cb;
    broadphase.getOverlappingPairCache()->setInternalGhostPairCallback(&cb);
    btDefaultCollisionConfiguration collisionConfiguration;
    btCollisionDispatcher dispatcher(&collisionConfiguration);
    btSequentialImpulseConstraintSolver solver;
    BulletSpace world(ghostObject, &dispatcher, &broadphase, &solver, &collisionConfiguration);
    world.world.setGravity(btVector3(0,-10,0));
    DebugDraw debugdraw;
    world.world.setDebugDrawer(&debugdraw);
    world.world.addCollisionObject(&ghostObject,
                                   btBroadphaseProxy::CharacterFilter,
                                   btBroadphaseProxy::StaticFilter|btBroadphaseProxy::AllFilter);
    world.world.addAction(&player);

    // Setup invisible arena walls
    ///////////////////////////////
    btStaticPlaneShape groundShape[4] = {
        btStaticPlaneShape(btVector3( 1, 0,  0), 1),
        btStaticPlaneShape(btVector3(-1, 0,  0), 1),
        btStaticPlaneShape(btVector3( 0, 0,  1), 1),
        btStaticPlaneShape(btVector3( 0, 0, -1), 1)
    };
    btDefaultMotionState groundMotionState[4];
    vector<btRigidBody> groundBody;
    btVector3 positions[4] = {
        btVector3(0, 0, 0),
        btVector3(arenaWidth, 0, 0),
        btVector3(0, 0, 0),
        btVector3(0, 0, arenaWidth)
    };
    for (unsigned i = 0; i < 4; i++) {
        groundMotionState[i] = btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),positions[i]));
        btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI
            (0,
             &groundMotionState[i],
             &groundShape[i],
             btVector3(0,0,0) );
        groundBody.emplace_back(groundRigidBodyCI);
        btRigidBody &groundRigidBody = groundBody.back();
        groundRigidBody.setRestitution(0.5);
        world.world.addRigidBody(&groundRigidBody);
    }

    // Create heightmap physics and render stuff
    /////////////////////////////////////////////
    ilA_img hm;
    ilA_imgerr res = ilA_img_loadfile(&hm, &demo_fs, "arena-heightmap.png");
    if (res) {
        il_error("Failed to load heightmap: %s", ilA_img_strerror(res));
        return 1;
    }
    const unsigned height = 50;
    btHeightfieldTerrainShape heightmap_shape
        (hm.width, hm.height, hm.data, height/255.f, 0, height, 1, PHY_UCHAR, false);
    heightmap_shape.setLocalScaling(btVector3(arenaWidth/hm.width, 1, arenaWidth/hm.height));
    btTransform trans = btTransform(btQuaternion(0,0,0,1),
                                    btVector3(arenaWidth/2, height/2, arenaWidth/2));
    btDefaultMotionState heightmap_state(trans);
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI
        (0, &heightmap_state, &heightmap_shape, btVector3(0,0,0));
    auto ground_id = world.add(groundRigidBodyCI);
    world.getBody(ground_id).setRestitution(1.0);
    world.setBodyScale(ground_id, il_vec3_new(128, 50, 128));
    ilA_img norm;
    res = ilA_img_height_to_normal(&norm, &hm);
    if (res) {
        il_error("Failed to create normal map: %s", ilA_img_strerror(res));
        return 1;
    }
    ilG_tex colortex, heighttex, normaltex;
    res = ilG_tex_loadfile(&colortex, &demo_fs, "terrain.png");
    if (res) {
        il_error("Failed to load heightmap texture: %s", ilA_img_strerror(res));
        return 1;
    }
    ilA_img hmc;
    res = ilA_img_copy(&hmc, &hm);
    if (res) {
        il_error("Failed to copy image: %s", ilA_img_strerror(res));
        return 1;
    }
    ilG_tex_loadimage(&heighttex, hmc);
    ilG_tex_loadimage(&normaltex, norm);

    // Create ball renderer and common bullet stuff
    ////////////////////////////////////////////////
    btSphereShape ball_shape(1);
    BallRenderer ball;
    unsigned seed = time(NULL);
    add_objects(world, &ball_shape, 100, &seed);

    float yaw = 0, pitch = 0;
    il_quat rot = il_quat_new(0,0,0,1);
    State state;
    while (1) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                il_log("Stopping");
                return 0;
            case SDL_MOUSEMOTION:
                if (ev.motion.state & SDL_BUTTON_LMASK) {
                    const float s = 0.01;
                    yaw = fmodf(yaw + ev.motion.xrel * s, M_PI * 2);
                    pitch = max((float)-M_PI/2, min((float)M_PI/2, pitch + ev.motion.yrel * s));
                    rot = il_quat_mul
                        (il_quat_fromAxisAngle(0,1,0, -yaw),
                         il_quat_fromAxisAngle(1,0,0, -pitch));
                    ghostObject.setWorldTransform(btTransform(btQuaternion(rot.x,rot.y,rot.z,rot.w),
                                                              ghostObject.getWorldTransform().getOrigin()));
                }
                break;
            }
        }
        {
            const uint8_t *state = SDL_GetKeyboardState(NULL);
            il_vec3 playerwalk = il_vec3_new
                (state[SDL_SCANCODE_D] - state[SDL_SCANCODE_A],
                 state[SDL_SCANCODE_R] - state[SDL_SCANCODE_F],
                 state[SDL_SCANCODE_S] - state[SDL_SCANCODE_W]);
            const float speed = 10 / 60.f; // speed/s over ticks/s
            playerwalk.x *= speed;
            playerwalk.y *= speed;
            playerwalk.z *= speed;
            playerwalk = il_vec3_rotate(playerwalk, rot);
            player.setWalkDirection(btVector3(playerwalk.x, playerwalk.y, playerwalk.z));
        }
        //world.step(1/60.f);
        graphics.draw(state);
    }
}

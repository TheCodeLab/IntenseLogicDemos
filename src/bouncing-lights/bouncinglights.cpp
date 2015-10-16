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

#include <experimental/optional>

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

struct Scene : public Drawable {
    Scene(BulletSpace &space)
        : space(space) {}

    BulletSpace &space;
    vector<BulletSpace::BodyID> bodies;
    vector<il_vec3> colors;
    vector<ilG_light> lights;
    experimental::optional<BulletSpace::BodyID> heightmap_body;
    ilG_heightmap heightmap;
    BallRenderer ball;
    btSphereShape sphere_shape = btSphereShape(1);
    btStaticPlaneShape ground_shape[4] = {
        btStaticPlaneShape(btVector3( 1, 0,  0), 1),
        btStaticPlaneShape(btVector3(-1, 0,  0), 1),
        btStaticPlaneShape(btVector3( 0, 0,  1), 1),
        btStaticPlaneShape(btVector3( 0, 0, -1), 1)
    };
    btDefaultMotionState ground_motion_state[4], heightmap_motion_state;
    experimental::optional<btHeightfieldTerrainShape> heightmap_shape;

    void draw(Graphics &graphics) override {
        (void)graphics;
        il_mat hmvp, himt;
        space.objmats(&hmvp, &*heightmap_body, ILG_MVP, 1);
        space.objmats(&himt, &*heightmap_body, ILG_IMT, 1);
        ilG_heightmap_draw(&heightmap, hmvp, himt);

        vector<il_mat> mvp, imt;
        mvp.resize(bodies.size());
        imt.resize(bodies.size());
        space.objmats(mvp.data(), bodies.data(), ILG_MVP, bodies.size());
        space.objmats(imt.data(), bodies.data(), ILG_IMT, bodies.size());
        ball.draw(mvp.data(), imt.data(), colors.data(), bodies.size());
    }

    bool build(ilG_renderman *rm) {
        // Arena walls
        ///////////////
        vector<btRigidBody> ground_body;
        ground_body.reserve(4);
        btVector3 positions[4] = {
            btVector3(0, 0, 0),
            btVector3(arenaWidth, 0, 0),
            btVector3(0, 0, 0),
            btVector3(0, 0, arenaWidth)
        };
        for (unsigned i = 0; i < 4; i++) {
            ground_motion_state[i] = btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),positions[i]));
            btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI
                (0,
                 &ground_motion_state[i],
                 &ground_shape[i],
                 btVector3(0,0,0) );
            auto id = space.add(groundRigidBodyCI);
            space.getBody(id).setRestitution(0.5);
        }

        // Image
        ///////////////////////
        ilA_img hm;
        ilA_imgerr res = ilA_img_loadfile(&hm, &demo_fs, "arena-heightmap.png");
        if (res) {
            il_error("Failed to load heightmap: %s", ilA_img_strerror(res));
            return false;
        }
        // Physics
        /////////////////////
        const unsigned height = 50;
        heightmap_shape = btHeightfieldTerrainShape(hm.width, hm.height, hm.data, height/255.f, 0,
                                                    height, 1, PHY_UCHAR, false);
        heightmap_shape->setLocalScaling(btVector3(arenaWidth/hm.width, 1, arenaWidth/hm.height));
        btTransform trans = btTransform(btQuaternion(0,0,0,1),
                                        btVector3(arenaWidth/2, height/2, arenaWidth/2));
        heightmap_motion_state = trans;
        btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI
            (0, &heightmap_motion_state, &*heightmap_shape, btVector3(0,0,0));
        heightmap_body = space.add(groundRigidBodyCI);
        space.getBody(*heightmap_body).setRestitution(1.0);
        space.setBodyScale(*heightmap_body, il_vec3_new(128, 50, 128));
        // Rendering
        ///////////////////////
        ilA_img norm;
        res = ilA_img_height_to_normal(&norm, &hm);
        if (res) {
            il_error("Failed to create normal map: %s", ilA_img_strerror(res));
            return false;
        }
        ilG_tex colortex, heighttex, normaltex;
        res = ilG_tex_loadfile(&colortex, &demo_fs, "terrain.png");
        if (res) {
            il_error("Failed to load heightmap texture: %s", ilA_img_strerror(res));
            return false;
        }
        ilA_img hmc;
        res = ilA_img_copy(&hmc, &hm);
        if (res) {
            il_error("Failed to copy image: %s", ilA_img_strerror(res));
            return false;
        }
        ilG_tex_loadimage(&heighttex, hmc);
        ilG_tex_loadimage(&normaltex, norm);
        char *error;
        if (!ilG_heightmap_build(&this->heightmap, rm, hm.width, hm.height,
                                 heighttex, normaltex, colortex, &error)) {
            il_error("heightmap: %s", error);
            free(error);
            return false;
        }

        if (!ball.build(rm, &error)) {
            il_error("ball: %s", error);
            free(error);
            return false;
        }

        return true;
    }

    void populate(size_t count) {
        unsigned seed;
        for (size_t i = 0; i < count; i++) {
            // Physics body
            btVector3 position
                (rand_r(&seed) % 128,
                 rand_r(&seed) % 32 + 50,
                 rand_r(&seed) % 128);
            auto state = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), position));
            float mass = 1.f;
            btVector3 inertia(0,0,0);
            sphere_shape.calculateLocalInertia(mass, inertia);
            btRigidBody::btRigidBodyConstructionInfo ballRigidBodyCI(mass, state, &sphere_shape, inertia);
            auto id = space.add(ballRigidBodyCI);
            space.getBody(id).setRestitution(1.0);

            // Color
            float brightness = rand_float(&seed) + 1;
            auto col = il_vec3_new(
                rand_float(&seed) * brightness,
                rand_float(&seed) * brightness,
                rand_float(&seed) * brightness);

            // Light
            ilG_light light;
            light.color = col;
            light.radius = brightness * 10;

            bodies.push_back(id);
            colors.push_back(col);
            lights.push_back(light);
        }
    }
};

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

    Scene scene(world);
    if (!scene.build(graphics.rm)) {
        return 1;
    }
    scene.populate(100);
    graphics.drawables.push_back(&scene);

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
        world.step(1/60.f);
        graphics.draw(state);
    }
}

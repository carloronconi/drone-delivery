//
// Created by Carlo Ronconi on 26/06/23.
//

#ifndef DRONE_DELIVERY_PLANE_HPP
#define DRONE_DELIVERY_PLANE_HPP

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include "UserInputs.hpp"
#include <ctime>
#include "Damper.hpp"
#include "Wing.hpp"

using namespace glm;
using namespace std;

enum Collision {NONE, GROUND, MESH};

struct ControlsMapping {
    float yaw;
    float pitch;
    float roll;
    float speed;

    /**
     * convert game-supplied inputs in logically simpler controls for a plane
     */
    void map(UserInputs inputs) {
        yaw = - inputs.m.x; // AD
        pitch = inputs.r.y; // left and right arrows
        roll = - inputs.r.x; // up and down arrows
        speed = inputs.m.z; // WS
        //cout << "speed input: " << speed << "\n";
    }
};


class Plane {
public:
    // constants
    // rotation and motion speed
    constexpr static const vec3 CONTROL_SURFACES_ROT_ACCELERATION{radians(10.0), radians(5.0), radians(10.0)};
    constexpr static const float ENGINE_ACCELERATION = 15.0f;
    constexpr static const float THROTTLE_DAMPING = 40;
    constexpr static const float MAX_SPEED = 15.0f;
    constexpr static const float ROT_DAMPING = 5.0;
    // plane physics parameters
    constexpr static const float PLANE_SCALE = 0.1f;
    constexpr static const float MAX_WING_LIFT = 10;
    constexpr static const float BASE = 2;
    constexpr static const float WING_LIFT_ANGLE = glm::radians(15.0f);
    constexpr static const float WING_INEFFICIENCY = 1.1f;
    constexpr static const bool PRINT_DEBUG = false;
    // friction deceleration in plane coordinates (z factor already accounted for in wing inefficiency)
    constexpr static const vec3 FRICTION = vec3(5, 1, 1);
    // all external accelerations including gravity
    // e.g. gravity only would be (0, -9.81, 0)
    // e.g. gravity plus x-wind would be (2.5, -9.81, 0)
    constexpr static const vec3 EXTERNAL_ACCELERATIONS{0, -9.81, 0};
    constexpr static const vec3 INITIAL_SPEED{0, 0, 0.1};
    // collision constants
    constexpr static const float GROUND_COLLISION_ROT = 0.3;
    constexpr static const float COLLISION_DISTANCE = 1.0f;
    constexpr static const vec3 MESH_COLLISION_BOUNCE = {-0.9, -1.1, -0.9};
    constexpr static const int SUCCESSIVE_MESH_COLLISIONS = 3;
    constexpr static const int PREV_COLLISIONS_SIZE = 10;

private:
    // state of the plane in world coordinates
    vec3 position;
    vec3 initialPosition;
    mat4 rotationMat; // doesn't solve flipping issue
    vec3 speed = INITIAL_SPEED;
    mat3 uAxes;
    const Wing& wing;

    // reference to command inputs used to update the world matrix
    UserInputs* inputs;
    ControlsMapping controls;
    // rotation is implemented without explicit rotSpeed term, but uses damper instead - using Damper<quat> introduces world mat deformations
    // so 3 separate dampers are required
    Damper<float> rollDamper = Damper<float>(ROT_DAMPING), yawDamper = Damper<float>(ROT_DAMPING), pitchDamper = Damper<float>(ROT_DAMPING);
    Damper<float> throttleDamper = Damper<float>(THROTTLE_DAMPING);

    // list of vertices of models for which we want collision detection
    const vector<vec3>& verticesToAvoid;

    Collision collision = NONE;
    vector<Collision> prevCollisions;

    /**
     * Collision detection algorithm
     * Simple technique based on plane position and on the vertices of the models that the plane has to avoid: find the vertex with
     * the highest y value among those with x and y "close" to the plane. If its y is higher than the plane's, it means a collision
     * has happened.
     * The big simplification here is that if the model has very few vertices very far apart (e.g. a simple big cube) a collision would
     * not be detected if the plane collided in the middle of the cube's face, because no vertex would be found close to the plane.
     */
    void detectCollisions() {
        glm::vec3 highestPoint = {0.0, -1.0, 0.0};
        for (auto p : verticesToAvoid) {
            // this condition checks a vertical cylinder of points of radius COLLISION_DISTANCE and takes the point inside
            // the cylinder with the highest y value
            if (glm::length(vec2(p.x, p.z) - vec2(position.x, position.z)) < COLLISION_DISTANCE && p.y > highestPoint.y) {
                highestPoint = p;
            }
        }
        if (highestPoint.y > position.y) { // mesh collisions have priority over ground collisions (in case both are happening)
            collision = MESH;
            return;
        }
        if (position.y < 0) {
            collision = GROUND; // simplest case: don't go below the ground
            return;
        }
        collision = NONE;
    }

    /**
     * helps counting multiple close MESH collisions (due to bouncing) as a single one
     * @return number of previous MESH collisions in the last PREV_COLLISIONS_SIZE collisions
     */
    int countPrevMeshCollisions() {
        int count = 0;
        for (auto c : prevCollisions) {
            if (c == MESH) count++;
        }
        return count;
    }

    /**
     * @return true if plane's speed is pointing north
     */
    bool isPointingNorth() {
        return speed.z >= 0;
    }

    /**
     * behaviour of the plane when a collision is detected, depending on the type of collision
     */
    void reactToCollision() {
        switch (collision) {
            case NONE: break;
            /**
             * collision with ground: nullify the vertical speed and position, and level the plane with the ground
             */
            case GROUND: {
                position.y = 0;
                speed.y = 0;

                // careful: euler angles returned by quaternion always keep yaw (y) in [0, 90] and compensate with other two

                // rotate only if previous collision state was NONE (was in the sky)
                if (!prevCollisions.empty() && prevCollisions[0] == NONE) {
                    // if looking back: float yaw = M_PI - eulerAngles(rotation).y;
                    float tempYaw = eulerAngles(quat_cast(rotationMat)).y;
                    float yaw = isPointingNorth()? tempYaw : static_cast<float>(M_PI) - tempYaw;
                    rotationMat = rotate(mat4(1), yaw, vec3{0, 1, 0}); // preserves initial yaw
                }

                //cout << "COLLISION WITH GROUND DETECTED\n";
                break;
            }
            /**
             * collision with mesh (building): "bounce" the plane back and if too many consecutive mesh collisions (>SUCCESSIVE_MESH_COLLISIONS/PREV_COLLISIONS_SIZE)
             * among the last collisions, also teleport the plane back to the center (to avoid "sinking" into buildings)
             */
            case MESH: {
                speed = {speed.x * MESH_COLLISION_BOUNCE.x,
                         speed.y * MESH_COLLISION_BOUNCE.y,
                         speed.z * MESH_COLLISION_BOUNCE.z};
                if(countPrevMeshCollisions() > SUCCESSIVE_MESH_COLLISIONS) {
                    position = {0, 0, 0};
                    speed = {0, 0, 0};
                }
                //cout << "COLLISION WITH BUILDING DETECTED\n";
                cout << "prevMeshCollisions = " << countPrevMeshCollisions() << "\n";
                break;
            }
        }
        if (prevCollisions.size() >= PREV_COLLISIONS_SIZE) prevCollisions.pop_back();
        prevCollisions.insert(prevCollisions.begin(), collision);
    }

    /**
     * [unfinished] makes flying the plane easier by automatically applying a small amount of roll to return the plane back to neutral roll
     * simulates plane with stable V-wing configuration
     * @return value between -1 and 1 similar to stick input
     */
    float computeAutoRollRotation() {
        /**
         * complexity comes from quaternion to euler conversion: different equivalent euler triplets representations for a single
         * euler angle exist, meaning sometimes eulerAngles doesn't return the most "logical" triplet
         * to fix it, try all the cases and see what triplet is returned and change push accordingly
         */
        float roll = eulerAngles(quat_cast(rotationMat)).z;
        float push = 0;
        if (0.0 >= roll && roll < M_PI / 2.0) push = - 2.0 * roll / M_PI; // push linearly harder for bigger roll angles
        else if (3.0 * M_PI / 2.0 <= roll && roll < 2.0 * M_PI) push = 2.0 * roll / (3.0 * M_PI);
        // no push when the plane is flying inverted - with roll between pi/ 2 and 3pi/2
        //cout << "autoroll: " << push << "\n";
        return push;
    }

    void updateUAxes() {
        // stores 4x4 homogeneous rotation matrix as a 3x3 matrix:
        // used to rotate vectors from plane's coordinate system to world coordinate system
        uAxes = rotationMat;
    }

    void printDebugInfo(const map<string, vec3>& additionalInfo) {
        static long lastPrintTime;
        long currTime = time(NULL);

        if (currTime - lastPrintTime < 1.0) return;
        lastPrintTime = currTime;

        cout.width(20); cout << left << "Position";
        cout.width(20); cout << left << "World speed";
        for (const auto& element : additionalInfo) {
            cout.width(20); cout << left << element.first;
        }

        cout << "\n";

        cout.width(20); cout << left << toString(position);
        cout.width(20); cout << left << toString(speed);
        for (const auto& element : additionalInfo) {
            cout.width(20); cout << left << toString(element.second);
        }

        cout << "\n";
    }

    string toString(vec3 vector) {
        return to_string((int)vector.x) + " " + to_string((int)vector.y) + " " + to_string((int)vector.z);
    }

public:
    Plane(const Wing& wing, const vector<vec3>& collisionDetectionVertices = {}, const vec3 &initialPosition = vec3(0),
          const mat4 &initialRotationMat = mat4(1)) :
            wing(wing),
            position(initialPosition),
            initialPosition(initialPosition),
            rotationMat(initialRotationMat),
            verticesToAvoid(collisionDetectionVertices) {}

    void updateInputs(UserInputs* userInputs) {
        inputs = userInputs;
    }

    /**
     * computes the world matrix for a new frame using the command inputs and stores it internally for other functions
     * @return updated world matrix
     */
    mat4 computeWorldMatrix() {
        controls.map(*inputs);
        updateUAxes();

        float wingLift = wing.computeLift((inverse(uAxes) * speed).z);

        // from glm::rotate documentation: returns (and takes as input) rotation MATRIX, or rotation QUATERNION according to signatures!
        rotationMat = rotate(rotationMat, rollDamper.damp(CONTROL_SURFACES_ROT_ACCELERATION.x * wingLift * (controls.roll /* - computeAutoRollRotation()*/) * inputs->deltaT, inputs->deltaT), vec3(1, 0, 0));
        rotationMat = rotate(rotationMat, yawDamper.damp(CONTROL_SURFACES_ROT_ACCELERATION.y * wingLift * controls.yaw * inputs->deltaT, inputs->deltaT), vec3(0, 1, 0));
        rotationMat = rotate(rotationMat, pitchDamper.damp(CONTROL_SURFACES_ROT_ACCELERATION.z * wingLift * controls.pitch * inputs->deltaT, inputs->deltaT), vec3(0, 0, 1));

        speed += inputs->deltaT * EXTERNAL_ACCELERATIONS; // external accelerations (doesn't require multiplying by uAxes: already in world coordinates)

        // friction deceleration and speed limiting are computed in plane space
        vec3 planeSpeed = inverse(uAxes) * speed; // convert speed from world to plane space
        // engine and wing accelerations
        planeSpeed += inputs->deltaT * (
                vec3(0.0f, 0.0f, throttleDamper.damp(controls.speed * ENGINE_ACCELERATION, inputs->deltaT))
                // acceleration due to plane wings generating lift linear with speed
                + vec3(0.0, std::cos(WING_LIFT_ANGLE) * wingLift, - WING_INEFFICIENCY * std::sin(WING_LIFT_ANGLE) * wingLift)
        );
        // plane speed is reduced by dynamic friction in the opposite direction of plane speed
        planeSpeed -= inputs->deltaT * vec3{FRICTION.x * planeSpeed.x, FRICTION.y * planeSpeed.y, FRICTION.z * planeSpeed.z};
        // plane speed magnitude (misleadingly named glm::length) is capped at max speed by multiplying the scalar for the direction of plane speed
        if (glm::length(planeSpeed) > MAX_SPEED) planeSpeed = MAX_SPEED * normalize(planeSpeed);
        speed = uAxes * planeSpeed; // update world speed converting back from plane speed

        position += speed * inputs->deltaT;

        detectCollisions();
        reactToCollision();

        mat4 world =
                translate(mat4(1), position) *
                rotationMat *
                scale(mat4(1), vec3(PLANE_SCALE)); //additional transform to scale down the character in character space


        if (PRINT_DEBUG) {
            map<string, vec3> debugInfo;
            debugInfo["Plane speed"] = planeSpeed;
            printDebugInfo(debugInfo);
        }

        return world;
    }

    void resetState() {
        speed = INITIAL_SPEED;
        position = initialPosition;
        rotationMat = mat4(1);
        prevCollisions.clear();
    }

    /**
     * @return plane position in world coordinates
     */
    vec3& getPositionInWorldCoordinates() {
        return position;
    }

    vec3& getSpeedInWorldCoordinates() {
        return speed;
    }

    /**
     * tells if a life-decreasing detection is detected
     * right now only building collisions are considered life-decreasing, but implementation can change to include ground
     */
    bool isCollisionDetected() {
        return collision == MESH;
    }
};


#endif //DRONE_DELIVERY_PLANE_HPP

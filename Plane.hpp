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

using namespace glm;
using namespace std;

enum Collision {NONE, GROUND, MESH};

struct ControlsMapping {
    float yaw;
    float pitch;
    float roll;
    float speed;

    void map(UserInputs inputs) {
        yaw = - inputs.m.x; // AD
        pitch = inputs.r.y; // left and right arrows
        roll = - inputs.r.x; // up and down arrows
        speed = inputs.m.z; // WS
        //cout << "speed input: " << speed << "\n";
    }
};


class Plane {
private:
    // constants
    // rotation and motion speed
    const float CONTROL_SURFACES_ROT_ACCELERATION = glm::radians(15.0f);
    const float ENGINE_ACCELERATION = 10.0f;
    const float MAX_SPEED = 10.0f;
    // plane physics parameters
    const float PLANE_CENTER_OF_LIFT = 1.5f;
    const float PLANE_SCALE = 0.1f;
    const float MAX_WING_LIFT = 5.5f;
    const float WING_LIFT_ANGLE = glm::radians(30.0f);
    const float WING_INEFFICIENCY = 1.1f;
    const bool PRINT_DEBUG = true;
    const float ROT_DAMPING = 5.0;
    // friction deceleration in plane coordinates (z factor already accounted for in wing inefficiency)
    const vec3 FRICTION = vec3(5, 4, 1);
    // all external accelerations including gravity
    // e.g. gravity only would be (0, -9.81, 0)
    // e.g. gravity plus x-wind would be (2.5, -9.81, 0)
    const vec3 EXTERNAL_ACCELERATIONS{0, -3, 0};

    // state of the plane in world coordinates
    vec3 position, velocity{0, 0, 0};
    quat rotation; vec3 rotSpeed{0, 0, 0};
    mat4 lastWorldMatrix;
    mat3 uAxes;

    // reference to command inputs used to update the world matrix
    UserInputs* inputs;
    ControlsMapping controls;
    Damper<vec3> rotationDamper = Damper<vec3>(ROT_DAMPING, {0, 0, 0});

    // list of vertices of models for which we want collision detection
    const vector<vec3>& verticesToAvoid;
    const float COLLISION_DISTANCE = 1.0f;
    Collision collision = NONE;
    const vec3 MESH_COLLISION_BOUNCE = {-0.9, -1.1, -0.9};
    vector<Collision> prevCollisions;

    /**
     * computes the module of the lift acceleration produced by a wing
     * @param orthogonalSpeed module of the plane speed orthogonal to the wing surface
     * @return lift acceleration produced by the wing
     */
    float wingLiftFunction(float orthogonalSpeed) const {
        if (orthogonalSpeed < - MAX_SPEED) return - MAX_WING_LIFT;
        if (orthogonalSpeed < 0) return orthogonalSpeed;
        if (orthogonalSpeed < MAX_SPEED) { // parabolic curve passing from origin and with maximum in (MAX_SPEED, MAX_WING_LIFT)
            return (- (MAX_WING_LIFT / (MAX_SPEED * MAX_SPEED)) * orthogonalSpeed + 2 * MAX_WING_LIFT / MAX_SPEED) * orthogonalSpeed;
        }
        return MAX_WING_LIFT;
    }

    /**
     * Collision detection algorithm
     * Simple technique based on plane position and on the vertices of the models that the plane has to avoid: find the vertex with
     * the highest y value among those with x and y "close" to the plane. If its y is higher than the plane's, it means a collision
     * has happened.
     * The big simplification here is that if the model has very few vertices very far apart (e.g. a simple big cube) a collision would
     * not be detected if the plane collided in the middle of the cube's face, because no vertex would be found close to the plane.
     */
    void detectCollisions() {
        if (position.y < 0) {
            collision = GROUND;
            return; // simplest case: don't go below the ground
        }
        glm::vec3 highestPoint = {0.0, -1.0, 0.0};
        for (auto p : verticesToAvoid) {
            // this condition checks a vertical cylinder of points of radius COLLISION_DISTANCE and takes the point inside
            // the cylinder with the highest y value
            if (glm::length(vec2(p.x, p.z) - vec2(position.x, position.z)) < COLLISION_DISTANCE && p.y > highestPoint.y) {
                highestPoint = p;
            }
        }
        collision = highestPoint.y > position.y ? MESH : NONE;
    }

    int countPrevMeshCollisions() {
        int count = 0;
        for (auto c : prevCollisions) {
            if (c == MESH) count++;
        }
        return count;
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
                velocity.y = 0;

                rotation *= rotate(quat(1,0,0,0), rotation.x * 0.3f, vec3(- 1, 0, 0))
                            * rotate(quat(1,0,0,0), rotation.z * 0.3f, vec3(0, 0, - 1));
                //cout << "COLLISION WITH GROUND DETECTED\n";
                break;
            }
            /**
             * collision with mesh (building): "bounce" the plane back and if too many consecutive mesh collisions (>3/10)
             * among the last collisions, also teleport the plane back to the center (to avoid "sinking" into buildings)
             */
            case MESH: {
                velocity = {velocity.x * MESH_COLLISION_BOUNCE.x,
                            velocity.y * MESH_COLLISION_BOUNCE.y,
                            velocity.z * MESH_COLLISION_BOUNCE.z};
                if(countPrevMeshCollisions() > 3) {
                    position = {0, 0, 0};
                    velocity = {0, 0, 0};
                }
                //cout << "COLLISION WITH BUILDING DETECTED\n";
                cout << "prevMeshCollisions = " << countPrevMeshCollisions() << "\n";
                break;
            }
        }
        if (prevCollisions.size() >= 10) prevCollisions.pop_back();
        prevCollisions.insert(prevCollisions.begin(), collision);
    }

    void updateUAxes() {
        // unitary-length axes xyz in plane coordinate rotated to xyz axes in world space
        // used to pass from plane's coordinate system to world coordinate system
        uAxes = mat4(rotation) * mat4(1, 0, 0, 1,
                                      0, 1, 0, 1,
                                      0, 0, 1, 1,
                                      0, 0, 0, 1);
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
        cout.width(20); cout << left << toString(velocity);
        for (const auto& element : additionalInfo) {
            cout.width(20); cout << left << toString(element.second);
        }

        cout << "\n";
    }

    string toString(vec3 vector) {
        return to_string((int)vector.x) + " " + to_string((int)vector.y) + " " + to_string((int)vector.z);
    }

public:
    Plane(const vector<vec3>& collisionDetectionVertices = {}, const vec3 &initialPosition = vec3(0), const quat &initialRotation = quat(1, 0, 0, 0)) :
            position(initialPosition),
            rotation(initialRotation),
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

        float wingLift = wingLiftFunction((inverse(uAxes) * velocity).z);
        velocity += inputs->deltaT * EXTERNAL_ACCELERATIONS; // external accelerations (doesn't require multiplying by uAxes: already in world coordinates)
        vec3 planeVelocity = inverse(uAxes) * velocity; // convert speed from world to plane space

        rotSpeed += inputs->deltaT * uAxes * rotationDamper.damp(vec3{ // control surfaces authority only depends on z-velocity
                CONTROL_SURFACES_ROT_ACCELERATION * planeVelocity.z * controls.roll,
                CONTROL_SURFACES_ROT_ACCELERATION * planeVelocity.z * controls.yaw,
                CONTROL_SURFACES_ROT_ACCELERATION * planeVelocity.z * controls.pitch
        }, inputs->deltaT);

        //rotSpeed -= inputs->deltaT * vec3{FRICTION.x * rotSpeed.x, FRICTION.y * rotSpeed.y, FRICTION.z * rotSpeed.z};

        //* rotate(quat(1,0,0,0), CONTROL_SURFACES_ROT_ACCELERATION * wingLift * controls.roll * inputs->deltaT, vec3(1, 0, 0))
        //* rotate(quat(1,0,0,0), CONTROL_SURFACES_ROT_ACCELERATION * wingLift * controls.yaw * inputs->deltaT, vec3(0, 1, 0))
        //* rotate(quat(1,0,0,0), CONTROL_SURFACES_ROT_ACCELERATION * wingLift * controls.pitch * inputs->deltaT, vec3(0, 0, 1))
        // s = s0 + v0 * t + 0.5 * a * t^2
        // v = v0 + a * t

        rotation *=
                rotate(quat(1,0,0,0), inputs->deltaT * rotSpeed.x, vec3(1, 0, 0))
                * rotate(quat(1,0,0,0), inputs->deltaT * rotSpeed.y, vec3(0, 1, 0))
                * rotate(quat(1,0,0,0), inputs->deltaT * rotSpeed.z, vec3(0, 0, 1));

        // friction deceleration and speed limiting are computed in plane space
        // engine and wing accelerations
        planeVelocity += inputs->deltaT * (
                vec3(0.0f, 0.0f, controls.speed) * ENGINE_ACCELERATION
                // acceleration due to plane wings generating lift linear with speed
                + vec3(0.0, std::cos(WING_LIFT_ANGLE) * wingLift, - WING_INEFFICIENCY * std::sin(WING_LIFT_ANGLE) * wingLift)
        );
        // plane speed is reduced by dynamic friction in the opposite direction of plane speed
        planeVelocity -= inputs->deltaT * vec3{FRICTION.x * planeVelocity.x, FRICTION.y * planeVelocity.y, FRICTION.z * planeVelocity.z};
        // plane speed magnitude (misleadingly named glm::length) is capped at max speed by multiplying the scalar for the direction of plane speed
        if (glm::length(planeVelocity) > MAX_SPEED) planeVelocity = MAX_SPEED * normalize(planeVelocity);
        velocity = uAxes * planeVelocity; // update world speed converting back from plane speed

        position += velocity * inputs->deltaT;

        detectCollisions();
        reactToCollision();

        lastWorldMatrix = translate(mat4(1), vec3(position.x, position.y + PLANE_CENTER_OF_LIFT * PLANE_SCALE, position.z)) *
                          rotate(mat4(rotation), glm::radians(270.0f), glm::vec3(0, 1, 0)) *
                          glm::scale(glm::mat4(1), glm::vec3(PLANE_SCALE)) * //additional transform to scale down the character in character space
                          glm::translate(glm::mat4(1), glm::vec3(0.0, - PLANE_CENTER_OF_LIFT, 0.0));


        map<string, vec3> debugInfo;
        debugInfo["Plane speed"] = planeVelocity;
        if (PRINT_DEBUG) printDebugInfo(debugInfo);

        return lastWorldMatrix * rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0,1,0));
    }

    void resetState() {
        velocity = {0, 0, 0};
        position = {0, 0, 0};
        rotation = {1, 0, 0, 0};
    }

    /**
     * @return plane position in world coordinates
     */
    vec3& getPositionInWorldCoordinates() {
        return position;
    }

    vec3& getSpeedInWorldCoordinates() {
        return velocity;
    }

    /**
     * tells if a life-decreasing detection is detected
     * right now only building collisions are consiidered life-decreasing, but implementation can change to include ground
     */
    bool isCollisionDetected() {
        return collision == MESH;
    }
};


#endif //DRONE_DELIVERY_PLANE_HPP

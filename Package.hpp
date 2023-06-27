//
// Created by Carlo Ronconi on 26/06/23.
//

#ifndef DRONE_DELIVERY_PACKAGE_HPP
#define DRONE_DELIVERY_PACKAGE_HPP


#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include "UserInputs.hpp"
#include <ctime>

using namespace glm;
using namespace std;

enum State {held, falling, ground};

class Package {
private:
    // state of the plane in world coordinates
    vec3 position;
    vec3 speed{0, 0, 0};
    vec3 externalAccelerations{0, - 3, 0};
    // e.g. gravity only would be (0, -9.81, 0)
    // e.g. gravity plus x-wind would be (2.5, -9.81, 0)
    mat3 uAxes;

    // friction deceleration in object coordinates
    vec3 planeCoordinatesFriction{5, 1, 1};

    // reference to command inputs used to update the world matrix
    UserInputs& inputs;

    // constants
    // rotation and motion speed
    const float MAX_SPEED = 10.0f;
    // plane physics parameters
    const float PLANE_SCALE = 0.1f;

    // package specific
    const glm::vec3& planePosition;
    const glm::vec3& planeSpeed;
    const glm::vec3& targetPosition;
    bool hitTarget = false;
    State state;
    const float TARGET_PRECISION = 10.0f;

    void updateUAxes() {
        // unitary-length axes xyz in plane coordinate rotated to xyz axes in world space
        // used to pass from plane's coordinate system to world coordinate system
        uAxes = mat4(1, 0, 0, 1,
                     0, 1, 0, 1,
                     0, 0, 1, 1,
                     0, 0, 0, 1);
    }

public:
    Package(UserInputs &inputs, const vec3 &planePosition, const vec3 &planeSpeed, const vec3&  targetPosition) :
            planePosition(planePosition),
            planeSpeed(planeSpeed),
            targetPosition(targetPosition),
            inputs(inputs),
            state(held) {}

    /**
     * computes the world matrix for a new frame using the command inputs and stores it internally for other functions
     * @return updated world matrix
     */
    mat4 computeWorldMatrix() {

        switch (state) {
            case held:
                hitTarget = false;
                if (inputs.handleFire) {
                    position = planePosition;
                    speed = planeSpeed;
                    state = falling;
                } else {
                    position = {0, -2, 0};
                    speed = {0, 0, 0};
                    return translate(mat4(1), position) * glm::scale(glm::mat4(1), glm::vec3(PLANE_SCALE));
                }
            case falling: {
                updateUAxes();

                speed += inputs.deltaT * externalAccelerations; // external accelerations (doesn't require multiplying by uAxes: already in world coordinates)

                // friction deceleration and speed limiting are computed in plane space
                vec3 planeSpeed = inverse(uAxes) * speed; // convert speed from world to plane space
                // plane speed is reduced by acceleration coordinates times deltaT in the opposite direction of plane speed
                planeSpeed -= planeCoordinatesFriction * inputs.deltaT * normalize(planeSpeed);
                // plane speed magnitude (misleadingly named glm::length) is capped at max speed by multiplying the scalar for the direction of plane speed
                if (glm::length(planeSpeed) > MAX_SPEED) planeSpeed = MAX_SPEED * normalize(planeSpeed);
                speed = uAxes * planeSpeed; // update world speed converting back from plane speed

                position += speed * inputs.deltaT;

                if (position.y < 0) { // simple collision detection
                    position.y = 0;
                    speed.y = 0;

                    state = ground;
                } else {
                    return translate(mat4(1), position) * glm::scale(glm::mat4(1), glm::vec3(PLANE_SCALE));
                }
            }
            case ground:
                state = held;
                cout << "Package position: " << position.x << " " << position.y << " " << position.z << "\n";
                cout << "Target position: " << targetPosition.x << " " << targetPosition.y << " " << targetPosition.z << "\n";
                if (glm::length(position - targetPosition) < TARGET_PRECISION) {
                    hitTarget = true; cout << "\nTARGET HIT!\n";
                } else {
                    cout << "\nTarget missed :(\n";
                }
                return translate(mat4(1), position) * glm::scale(glm::mat4(1), glm::vec3(PLANE_SCALE));
        }
    }

    /**
     * @return plane position in world coordinates
     */
    vec3 getPositionInWorldCoordinates() {
        return position;
    }

    bool isTargetHit() {
        return hitTarget;
    }
};


#endif //DRONE_DELIVERY_PACKAGE_HPP

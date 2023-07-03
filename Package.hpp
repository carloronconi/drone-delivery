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
    // constants
    // rotation and motion speed
    const float MAX_SPEED = 15.0;
    // plane physics parameters
    const float SCALE = 0.1;
    const float TARGET_PRECISION = 10.0;
    // friction deceleration in object coordinates
    const vec3 FRICTION = vec3(5, 1, 5); // stronger horizontal friction makes dropping easier

    // state of the plane in world coordinates
    vec3 position;
    vec3 speed{0, 0, 0};
    vec3 externalAccelerations{0, - 3, 0};
    // e.g. gravity only would be (0, -9.81, 0)
    // e.g. gravity plus x-wind would be (2.5, -9.81, 0)
    // uAxes are actually fixed here, because we don't need to keep track of the package rotation
    mat3 uAxes = mat4(1, 0, 0, 1,
                      0, 1, 0, 1,
                      0, 0, 1, 1,
                      0, 0, 0, 1);

    // reference to command inputs used to update the world matrix
    UserInputs* inputs;

    // package specific
    const glm::vec3& planePosition;
    const glm::vec3& planeSpeed;
    const glm::vec3& targetPosition;
    bool hitTarget = false;
    State state;

public:
    Package(const vec3 &planePosition, const vec3 &planeSpeed, const vec3&  targetPosition) :
            planePosition(planePosition),
            planeSpeed(planeSpeed),
            targetPosition(targetPosition),
            state(held) {}

    void updateInputs(UserInputs* userInputs) {
        inputs = userInputs;
    }

    /**
     * computes the world matrix for a new frame using the command inputs and stores it internally for other functions
     * @return updated world matrix
     */
    mat4 computeWorldMatrix() {
        switch (state) {
            case held:
                hitTarget = false;
                if (inputs->handleFire) {
                    position = planePosition;
                    speed = planeSpeed;
                    state = falling;
                } else {
                    position = {0, -2, 0};
                    speed = {0, 0, 0};
                    return translate(mat4(1), position) * glm::scale(glm::mat4(1), glm::vec3(SCALE));
                }
            case falling: {
                speed += inputs->deltaT * externalAccelerations; // external accelerations (doesn't require multiplying by uAxes: already in world coordinates)

                // friction deceleration and speed limiting are computed in plane space
                vec3 planeSpeed = inverse(uAxes) * speed; // convert speed from world to plane space
                // plane speed is reduced by acceleration coordinates times deltaT in the opposite direction of plane speed
                planeSpeed -= FRICTION * inputs->deltaT * normalize(planeSpeed);
                // plane speed magnitude (misleadingly named glm::length) is capped at max speed by multiplying the scalar for the direction of plane speed
                if (glm::length(planeSpeed) > MAX_SPEED) planeSpeed = MAX_SPEED * normalize(planeSpeed);
                speed = uAxes * planeSpeed; // update world speed converting back from plane speed

                position += speed * inputs->deltaT;

                if (position.y < 0) { // simple collision detection
                    position.y = 0;
                    speed.y = 0;

                    state = ground;
                } else {
                    return translate(mat4(1), position) * glm::scale(glm::mat4(1), glm::vec3(SCALE));
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
                return translate(mat4(1), position) * glm::scale(glm::mat4(1), glm::vec3(SCALE));
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

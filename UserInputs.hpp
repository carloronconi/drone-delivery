//
// Created by Carlo Ronconi on 26/06/23.
//

#ifndef DRONE_DELIVERY_USERINPUTS_HPP
#define DRONE_DELIVERY_USERINPUTS_HPP

#include <glm/vec3.hpp>
#include "DataStructs.hpp"

struct UserInputs {
    float deltaT;
    glm::vec3 m;
    glm::vec3 r;
    bool fire = false;
    bool handleFire;
    bool handleNext;
    GameState& gameState;

public:
    UserInputs(BaseProject* project, GameState& gameState):
    gameState(gameState) {
        project->getSixAxis(deltaT, m, r, fire);

        // To debounce the pressing of the fire button, and start the event when the key is released
        static bool wasFire = false;
        handleFire = (wasFire && (!fire)) && gameState == PLAYING;
        handleNext = (wasFire && (!fire)) && gameState != PLAYING;
        wasFire = fire;
    }
};

#endif //DRONE_DELIVERY_USERINPUTS_HPP

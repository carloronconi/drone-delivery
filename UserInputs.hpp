//
// Created by Carlo Ronconi on 26/06/23.
//

#ifndef DRONE_DELIVERY_USERINPUTS_HPP
#define DRONE_DELIVERY_USERINPUTS_HPP

#include <glm/vec3.hpp>
#include "DataStructs.hpp"

struct UserInputs {
    float deltaT;
    glm::vec3 m; // {DA, RF, WS} e.g. F = (m.y == -1)
    glm::vec3 r; // {DownUp, RightLeft, QE}
    bool fire = false;
    bool handleFire;
    bool handleNext;
    bool handleR;
    bool handleF;
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

        static bool shouldHandleR;
        if (!shouldHandleR && m.y == 1) {
            shouldHandleR = true;
        }
        else if (shouldHandleR && m.y == 1) {
            shouldHandleR = false;
        }
        handleR = shouldHandleR;
    }
};

#endif //DRONE_DELIVERY_USERINPUTS_HPP

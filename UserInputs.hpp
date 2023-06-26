//
// Created by Carlo Ronconi on 26/06/23.
//

#ifndef DRONE_DELIVERY_USERINPUTS_HPP
#define DRONE_DELIVERY_USERINPUTS_HPP

#include <glm/vec3.hpp>

struct UserInputs {
    float deltaT;
    glm::vec3 m;
    glm::vec3 r;
    bool fire;

public:
    UserInputs(BaseProject* project) {
        project->getSixAxis(deltaT, m, r, fire);
    }
};

#endif //DRONE_DELIVERY_USERINPUTS_HPP

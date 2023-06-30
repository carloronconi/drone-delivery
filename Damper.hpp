//
// Created by Carlo Ronconi on 30/06/23.
//

#ifndef DRONE_DELIVERY_DAMPER_HPP
#define DRONE_DELIVERY_DAMPER_HPP

#include <cmath>

template<class T>
class Damper {
private:
    const float DAMP;
    T prev;
    T start;

public:
    Damper(float damp = 50, T start = T()):
    DAMP(damp),
    prev(start),
    start(start){}

    T damp(T next, float deltaT) {

        float prod = - DAMP * deltaT;
        T curr = prev * exp(prod) + next * (1 - exp(prod));
        prev = curr;

        return curr;
    }

    void reset() {
        prev = start;
    }
};

#endif //DRONE_DELIVERY_DAMPER_HPP

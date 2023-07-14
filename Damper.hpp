//
// Created by Carlo Ronconi on 30/06/23.
//

#ifndef DRONE_DELIVERY_DAMPER_HPP
#define DRONE_DELIVERY_DAMPER_HPP

#include <cmath>

/**
 * overload operator> and operator< to allow templating of damper with vec3 T
 * add overloads of other Ts if needed
**/
template<typename T, glm::qualifier Q>
GLM_FUNC_QUALIFIER GLM_CONSTEXPR bool operator>(glm::vec<3, T, Q> const& v1, glm::vec<3, T, Q> const& v2)
{
    return glm::length(v1) > glm::length(v2);
}

template<typename T, glm::qualifier Q>
GLM_FUNC_QUALIFIER GLM_CONSTEXPR bool operator<(glm::vec<3, T, Q> const& v1, glm::vec<3, T, Q> const& v2)
{
    return glm::length(v1) < glm::length(v2);
}

/**
 * When passed to Damper allows to set limits to the value returned by .damp() to avoid spikes
 */
template<class T>
struct Constraint {
    bool active = false;
    T value = T();
};

template<class T>
class Damper {
private:
    const float DAMP;
    T prev;
    T start;
    Constraint<T> upper;
    Constraint<T> lower;

public:
    Damper(float damp = 50, T start = T(), Constraint<T> upperConstraint = {false, T()}, Constraint<T> lowerConstraint = {false, T()}):
    DAMP(damp),
    prev(start),
    start(start),
    upper(upperConstraint),
    lower(lowerConstraint){}

    /**
    * Returns smoothed-out version of next over time, optionally constrained by the lower and upper constraints
    */
    T damp(T next, float deltaT) {

        float prod = - DAMP * deltaT;
        T curr = prev * exp(prod) + next * (1 - exp(prod));

        if (upper.active && curr > upper.value) curr = upper.value;
        else if (lower.active && curr < lower.value) curr = lower.value;

        prev = curr;

        return curr;
    }

    void reset() {
        prev = start;
    }
};

#endif //DRONE_DELIVERY_DAMPER_HPP

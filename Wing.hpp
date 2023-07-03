//
// Created by Carlo Ronconi on 03/07/23.
//

#ifndef DRONE_DELIVERY_WING_HPP
#define DRONE_DELIVERY_WING_HPP

#include <cmath>

class Wing {
public:
    /**
     * computes the signed module of the lift acceleration produced by a wing
     * @param orthogonalSpeed signed module of the plane speed orthogonal to the wing surface
     * @return lift produced by the wing
     */
    virtual float computeLift(float orthogonalSpeed) const = 0;
};

class ParabolicWing : public Wing {
    const float MAX_SPEED;
    const float MAX_WING_LIFT;

public:
    ParabolicWing(const float maxSpeed, const float maxWingLift) : MAX_SPEED(maxSpeed), MAX_WING_LIFT(maxWingLift) {}

    float computeLift(float orthogonalSpeed) const override {
        if (orthogonalSpeed < - MAX_SPEED) return - MAX_WING_LIFT; // capped negative lift
        if (orthogonalSpeed < 0) return orthogonalSpeed; // linear (negative lift) for negative speeds
        if (orthogonalSpeed < MAX_SPEED) { // parabolic curve passing from origin and with maximum in (MAX_SPEED, MAX_WING_LIFT)
            return (- (MAX_WING_LIFT / (MAX_SPEED * MAX_SPEED)) * orthogonalSpeed + 2 * MAX_WING_LIFT / MAX_SPEED) * orthogonalSpeed;
        }
        return MAX_WING_LIFT;
    }
};

class LogarithmicWing : public Wing {
    const float MAX_WING_LIFT;
    const float MAX_SPEED;
    const float BASE;

public:
    /**
     * Logarithmic implementation of wing lift function
     * @param base changes how fast the lift increases with the speed
     */
    LogarithmicWing(const float maxWingLift, const float maxSpeed, const float base) : MAX_WING_LIFT(maxWingLift),
                                                                                       MAX_SPEED(maxSpeed),
                                                                                       BASE(base) {}

    float computeLift(float orthogonalSpeed) const override {
        if (orthogonalSpeed < - MAX_SPEED) return - MAX_WING_LIFT; // capped negative lift
        if (orthogonalSpeed < 0) return orthogonalSpeed; // linear (negative lift) for negative speeds
        return - pow(BASE, - orthogonalSpeed + log(MAX_WING_LIFT) / log(BASE)) + MAX_WING_LIFT;
        // no capping on positive lift is required thanks to the nature of power function
    }
};

#endif //DRONE_DELIVERY_WING_HPP

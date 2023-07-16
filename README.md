# Drone delivery
Fly your drone around the city and deliver all the packages! Beware: crashing into the buildings is easier than you think!

Project for [Computer Graphics
course](https://www11.ceda.polimi.it/schedaincarico/schedaincarico/controller/scheda_pubblica/SchedaPublic.do?&evn_default=evento&c_classe=789226&polij_device_category=DESKTOP&__pj0=0&__pj1=d5ba826011a30aecef5f9cd5ea045a7d) - MSc in Computer Science and Engineering @ Polimi.

## Features
### Simple physics engine
Plane flight is simulated using a simple physics engine implementing: 
- [kinematic particle equations](https://en.wikipedia.org/wiki/Equations_of_motion#Constant_linear_acceleration_in_any_direction) for linear quantities;
- quaternion-based rotation (without storing rotation speed) + [damped](https://github.com/carloronconi/drone-delivery/blob/78f03a27c694011560871ec6629f8b08b1ececde/Damper.hpp) Roll-Yaw-Pitch (RYP) inputs;
- [parabolic and logarithmic](https://github.com/carloronconi/drone-delivery/blob/78f03a27c694011560871ec6629f8b08b1ececde/Wing.hpp) implementations of aerodynamic wing to simulate lift.

Package-dropping is implemented in a similar way, but disregards rotations and lift.
### Simple collision detection & reaction
Detect
React
### Dynamic camera view
Damped
### Custom pipelines with dynamic day/night modes
List all the different pipelines and the dynamic aspect.
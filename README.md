# Drone delivery
Fly your drone around the city and deliver all the packages! Beware: crashing into the buildings is easier than you think!

<p align="center" width="100%">
    <img width="49%" src="https://github.com/carloronconi/drone-delivery/blob/main/readme-assets/start.gif">
    <img width="49%" src="https://github.com/carloronconi/drone-delivery/blob/main/readme-assets/drop.gif">
</p>

[Computer Graphics
course](https://www11.ceda.polimi.it/schedaincarico/schedaincarico/controller/scheda_pubblica/SchedaPublic.do?&evn_default=evento&c_classe=789226&polij_device_category=DESKTOP&__pj0=0&__pj1=d5ba826011a30aecef5f9cd5ea045a7d) - MSc Computer Science and Engineering @ Polimi.

## Usage

[Vulkan](https://www.vulkan.org) and CMake are required. 
1. Clone the repository wherever you prefer;
2. Open a terminal and run the following commands:
3. ```
   cd drone-delivery 
   ```
4. ```
   cmake .
   ```
5. ```
   make 
   ```
6. ```
   ./drone-delivery
   ```
   
If you're using a modern code editor, when opening the project folder it should be automatically recognised as a CMake project. In that case, just press the "run" button and you're good to go! 

## Features
### Physics engine
Plane flight is simulated using a simple physics engine. The plane is controlled using WASD and arrow keys to control throttle, roll, pitch and yaw. The physics engine implements: 
- [kinematic particle equations](https://en.wikipedia.org/wiki/Equations_of_motion#Constant_linear_acceleration_in_any_direction) for linear quantities;
- quaternion-based rotation (without storing rotation speed) + [damped](https://github.com/carloronconi/drone-delivery/blob/78f03a27c694011560871ec6629f8b08b1ececde/Damper.hpp) Roll-Yaw-Pitch (RYP) inputs;
- [parabolic and logarithmic](https://github.com/carloronconi/drone-delivery/blob/78f03a27c694011560871ec6629f8b08b1ececde/Wing.hpp) implementations of aerodynamic wing to simulate lift.

Package-dropping is implemented in a similar way, but disregards rotations and lift.
### Collision detection & reaction
[Collision detection](https://github.com/carloronconi/drone-delivery/blob/980084454ca97ce89378cf34ca8b3030833ac88b/Plane.hpp#L112C1-L112C1) is vertex & height based. First, if the plane's height is below 0, a GROUND collision is detected. Otherwise, we find the vertex mesh with the highest y among those within a certain circular x, z radius from the plane. If its height is greater than the plane's, a MESH collision is detected. 

[Collision reaction](https://github.com/carloronconi/drone-delivery/blob/980084454ca97ce89378cf34ca8b3030833ac88b/Plane.hpp#L154) depends on the type of collision that was detected. In case of GROUND collision, the plane is simply kept above ground and its yaw is preserved, while all other rotations are zeroed. In case of MESH collision, the plane is "bounced" back in the opposite direction of its speed.

<p align="center" width="100%">
    <img width="49%" src="https://github.com/carloronconi/drone-delivery/blob/main/readme-assets/crash.gif">
</p>

### Dynamic camera view
The camera allows for two modes: the classic follow camera and a stationary camera on the landing strip, pointed to the plane. The camera is [damped](https://github.com/carloronconi/drone-delivery/blob/78f03a27c694011560871ec6629f8b08b1ececde/Damper.hpp) during each of the two states and during the switch, creating a smooth transition between the two.

<p align="center" width="100%">
    <img width="49%" src="https://github.com/carloronconi/drone-delivery/blob/main/readme-assets/camera.gif">
</p>

### Custom pipelines with dynamic day/night modes
A total of 5 custom pipelines exist, each implementing a different vertex-fragment shader couple. Complete list of the pipelines:
- Overlay: splash screens and text/symbol overlay during game;
- Metallic: plane and arrow;
- Opaque: city blocks;
- Emit: roads with lampposts;
- Animation: propellers.

Some pipelines implement instanced-rendering to draw more instances of the same model in different positions without needing to store a different model for each. This is how the road tiles or the packages on the overlay are rendered.

<p align="center" width="100%">
    <img width="49%" src="https://github.com/carloronconi/drone-delivery/blob/main/readme-assets/night.gif">
</p>

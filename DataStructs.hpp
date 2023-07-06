//
// Created by Carlo Ronconi on 27/06/23.
//

#ifndef DRONE_DELIVERY_DATASTRUCTS_HPP
#define DRONE_DELIVERY_DATASTRUCTS_HPP


// The uniform buffer objects data structures
// Remember to use the correct alignas(...) value
//        float : alignas(4)
//        vec2  : alignas(8)
//        vec3  : alignas(16)
//        vec4  : alignas(16)
//        mat3  : alignas(16)
//        mat4  : alignas(16)

struct MeshUniformBlock {
    alignas(4) float amb;
    alignas(4) float gamma;
    alignas(16) glm::vec3 sColor;
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

struct OpaqueUniformBlock {
    alignas(4) float amb;
    alignas(4) float sigma;
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

struct OverlayUniformBlock {
    alignas(4) float visible; // applies to all instances
    alignas(16) glm::mat4 mvpMat;
    alignas(8) glm::vec2 offset;
    alignas(4) float instancesToDraw; // applies only if visible == 1
};

struct GlobalUniformBlock {
    alignas(16) glm::vec3 DlightDir;
    alignas(16) glm::vec3 DlightColor;
    alignas(16) glm::vec3 AmbLightColor;
    alignas(16) glm::vec3 eyePos;
    alignas(4) float usePointLight;
};

// The vertices data structures
struct VertexClassic {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 UV;
};

struct VertexOverlay {
    glm::vec2 pos;
    glm::vec2 UV;
};

enum GameState {SPLASH, PLAYING, WON, LOST};

#endif //DRONE_DELIVERY_DATASTRUCTS_HPP

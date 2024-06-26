//
// Created by Dark Matter on 5/21/24.
//

#ifndef LEARNOPENGL_LIGHT_H
#define LEARNOPENGL_LIGHT_H

#include "vec4.hpp"
#include "core/Component.h"
#include "shader/Shader.h"
#include "core/Behaviour.h"

enum LightType {
    BASE, DIRECTIONAL
};

class Light : public Behaviour {
public:
    Light();
    glm::vec4 color = {1.0, 1.0, 1.0, 1.0};
    float ambientIntensity = 1.0;
    float diffuseIntensity = 1.0;

    void bind(Shader *shader, const glm::vec3 & cameraLocalPos) override;
};


#endif //LEARNOPENGL_LIGHT_H

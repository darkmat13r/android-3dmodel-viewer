//
// Created by Dark Matter on 5/21/24.
//

#include "Light.h"
#include "Utility.h"

void Light::bind(Shader *shader, const glm::vec3& cameraLocalPos) {

    if (shader->getLightColorLocation() != -1)
        glUniform3f(shader->getLightColorLocation(), color.r, color.g, color.b);

    if (shader->getAmbientIntensityLocation() != -1)
        glUniform1f(shader->getAmbientIntensityLocation(), ambientIntensity);
    glUniform1f(shader->lightTypeLocation, 0);
    CHECK_GL_ERROR();
    glUniform3f(shader->getCameraLocalPosLocation(), cameraLocalPos.x, cameraLocalPos.y, cameraLocalPos.z);

    CHECK_GL_ERROR();
}

Light::Light() {

}


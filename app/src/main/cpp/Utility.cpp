#include "Utility.h"
#include "AndroidOut.h"
#include "math/mat4f.h"
#include "math/math.h"

#include <GLES3/gl3.h>

#define CHECK_ERROR(e) case e: aout << "GL Error: "#e << std::endl; break;

bool Utility::checkAndLogGlError(bool alwaysLog) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR) {
        if (alwaysLog) {
            aout << "No GL error" << std::endl;
        }
        return true;
    } else {
        switch (error) {
            CHECK_ERROR(GL_INVALID_ENUM);
            CHECK_ERROR(GL_INVALID_VALUE);
            CHECK_ERROR(GL_INVALID_OPERATION);
            CHECK_ERROR(GL_INVALID_FRAMEBUFFER_OPERATION);
            CHECK_ERROR(GL_OUT_OF_MEMORY);
            default:
                aout << "Unknown GL error: " << error << std::endl;
        }
        return false;
    }
}
glm::mat4 *
Utility::buildOrthographicMatrix(glm::mat4 *outMatrix, float halfHeight, float aspect, float near,
                                 float far) {
    float halfWidth = halfHeight * aspect;

    float A = -2.f / (far - near);
    float B = -(far + near) / (far - near);
    (*outMatrix)[0][0] = 1/aspect;
    (*outMatrix)[1][1] = 1/aspect;
    (*outMatrix)[2][2] = A;
    (*outMatrix)[2][3] = B;
    (*outMatrix)[3][3] = 0;
    (*outMatrix)[3][2] = 1;

    return outMatrix;
}
glm::mat4 *
Utility::buildPerspectiveMat(glm::mat4 *outMatrix, float halfHeight, float aspect, float near,
                             float far) {
    // Initialize matrix to zero before setting perspective values
    if(outMatrix == nullptr){
        *outMatrix = glm::mat4(1.0f);
    }
    float pov = 90;
    float tan = tanf(ToRadian(pov / 2));
    float halfWidth = halfHeight * aspect;
    float d = 1/tan;
    float zRange = (near - far);
    float A = (-far - near) / zRange;
    float B = 2 * far * near / zRange;
    (*outMatrix)[0][0] = d/aspect;
    (*outMatrix)[1][1] = d;
    (*outMatrix)[2][2] = A;
    (*outMatrix)[2][3] = B;
    (*outMatrix)[3][3] = 0;
    (*outMatrix)[3][2] = 1;

    return outMatrix;
}

float *Utility::buildIdentityMatrix(float *outMatrix) {
    // column 1
    outMatrix[0] = 1.f;
    outMatrix[1] = 0.f;
    outMatrix[2] = 0.f;
    outMatrix[3] = 0.f;

    // column 2
    outMatrix[4] = 0.f;
    outMatrix[5] = 1.f;
    outMatrix[6] = 0.f;
    outMatrix[7] = 0.f;

    // column 3
    outMatrix[8] = 0.f;
    outMatrix[9] = 0.f;
    outMatrix[10] = 1.f;
    outMatrix[11] = 0.f;

    // column 4
    outMatrix[12] = 0.f;
    outMatrix[13] = 0.f;
    outMatrix[14] = 0.f;
    outMatrix[15] = 1.f;

    return outMatrix;
}
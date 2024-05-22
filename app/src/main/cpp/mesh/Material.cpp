//
// Created by Dark Matter on 5/18/24.
//

#include "Material.h"
#include "string"
#include "AndroidOut.h"

Shader *Material::getShader() const {
    return shader_.get();
}

Material::Material(std::shared_ptr<TextureAsset> textureAsset) : diffuseTexture_(
        std::move(textureAsset)), diffuseColor_(0, 0, 0, 1) {
    loadShader();

}

void Material::loadShader() {
    std::string vertexShaderPath = "shaders/base_frag.vert";
    std::string fragmentShaderPath = "shaders/base_frag.frag";

    shader_ = std::__ndk1::make_shared<Shader>(vertexShaderPath, fragmentShaderPath);
}


Material::Material(glm::vec4 diffuseColor) : diffuseColor_(diffuseColor) {
    loadShader();
}


Material::~Material() {
    aout << "Material::destroy" << std::endl;
}

void Material::bindTexture() const {
    if (diffuseTexture_) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture_->getTextureID());
        aout << "Bind Text : " << diffuseTexture_->getTextureID() << std::endl;
        glUniform1i(shader_->getUseDiffTextureLocation(), GL_TRUE);
    }else {
        glUniform1i(shader_->getUseDiffTextureLocation(), GL_FALSE);
        //Push Color to fragment shader
        glUniform4fv(shader_->getDiffColorLocation(), 1, (const GLfloat *) &diffuseColor_.x);
    }
}


void Material::unbindTexture() const {
    aout << "Unbind Text : " << diffuseTexture_->getTextureID() << std::endl;
    glBindTexture(GL_TEXTURE_2D, 0);
}

Material::Material() {
    diffuseColor_ = glm::vec4 (0, 0, 0, 1);
    loadShader();
}



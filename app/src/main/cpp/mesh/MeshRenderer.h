//
// Created by Dark Matter on 5/18/24.
//

#ifndef LEARNOPENGL_MESHRENDERER_H
#define LEARNOPENGL_MESHRENDERER_H


#include "Mesh.h"
#include "core/Component.h"
#include "light/Light.h"
#include "camera/Camera.h"

class MeshRenderer : public Component {
public :
    MeshRenderer();

    ~MeshRenderer();

    void onAttach() override;

    void onCreate() override;

    void render(Mat4f *projectionMatrix,Camera* camera, Light *light);

    void update() override;

    void onDestroy() override;

    void addMesh(const std::shared_ptr<Mesh> &mesh);


private :
    std::vector<std::shared_ptr<Mesh>> meshes_;
    float rotation;

    void initMesh(Mesh *mesh) const;
};


#endif //LEARNOPENGL_MESHRENDERER_H

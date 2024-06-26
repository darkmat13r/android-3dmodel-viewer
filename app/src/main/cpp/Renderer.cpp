#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <android/imagedecoder.h>

#include "AndroidOut.h"
#include "unused/ShaderBase.h"
#include "Utility.h"
#include "TextureAsset.h"
#include "core/Scene.h"
#include "core/Component.h"
#include "mesh/MeshRenderer.h"
#include "assimp/Importer.hpp"
#include "importer/ModelImporter.h"
#include "assimp/port/AndroidJNI/AndroidJNIIOSystem.h"
#include "light/DirectionalLight.h"
#include "mesh/primitives/Sphere.h"
#include "light/PointLight.h"
#include "light/SpotLight.h"

//! executes glGetString and outputs the result to logcat
#define PRINT_GL_STRING(s) {aout << #s": "<< glGetString(s) << std::endl;}

/*!
 * @brief if glGetString returns a space separated list of elements, prints each one on a new line
 *
 * This works by creating an istringstream of the input c-style string. Then that is used to create
 * a vector -- each element of the vector is a new element in the input string. Finally a foreach
 * loop consumes this and outputs it to logcat using @a aout
 */
#define PRINT_GL_STRING_AS_LIST(s) { \
std::istringstream extensionStream((const char *) glGetString(s));\
std::vector<std::string> extensionList(\
        std::istream_iterator<std::string>{extensionStream},\
        std::istream_iterator<std::string>());\
aout << #s":\n";\
for (auto& extension: extensionList) {\
    aout << extension << "\n";\
}\
aout << std::endl;\
}

//! Color for cornflower blue. Can be sent directly to glClearColor
#define CORNFLOWER_BLUE 100 / 255.f, 149 / 255.f, 237 / 255.f, 1
#define DARK_GRAY 20 / 255.f, 20 / 255.f, 20 / 255.f, 1


#define WINDOW_WIDTH  2560
#define WINDOW_HEIGHT 1440


Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
    scene_->onDestroy();

}

void Renderer::render() {
    // Check to see if the surface has changed size. This is _necessary_ to do every frame when
    // using immersive mode as you'll get no other notification that your renderable area has
    // changed.
    updateRenderArea();

    // Clear the screen and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene_->render();

    GLenum err;
    CHECK_GL_ERROR();

    scene_->update();

    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}

void Renderer::initRenderer() {

    // Choose your render attributes
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    // The default display is probably what you want on Android
    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    // figure out how many configs there are
    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

    // get the list of configurations
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    // Find a config we like.
    // Could likely just grab the first if we don't care about anything else in the config.
    // Otherwise hook in your own heuristic
    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {

                    aout << "Found config with " << red << ", " << green << ", " << blue << ", "
                         << depth << std::endl;
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    aout << "Found " << numConfigs << " configs" << std::endl;
    aout << "Chose " << config << std::endl;

    // create the proper window surface
    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);


    // Create a GLES 3 context
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);
    auto versionDetails = glGetString(GL_SHADING_LANGUAGE_VERSION);
    // aout << "VERSION " << *versionDetails << std::endl;
    // get some window metrics
    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    display_ = display;
    surface_ = surface;
    context_ = context;

    // make width and height invalid so it gets updated the first frame in @a updateRenderArea()
    width_ = -1;
    height_ = -1;

    PRINT_GL_STRING(GL_VENDOR);
    PRINT_GL_STRING(GL_RENDERER);
    PRINT_GL_STRING(GL_VERSION);
    PRINT_GL_STRING(GL_SHADING_LANGUAGE_VERSION);
    PRINT_GL_STRING_AS_LIST(GL_EXTENSIONS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);

    // setup any other gl related global states
    glClearColor(DARK_GRAY);

    // enable alpha globally for now, you probably don't want to do this in a game
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    shaderLoader_ = std::make_shared<ShaderLoader>();
}

void Renderer::updateRenderArea() {
    EGLint width;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);

    EGLint height;
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;


        initScene();

        glViewport(0, 0, width, height);

        // make sure that we lazily recreate the projection matrix before we render
        shaderNeedsNewProjectionMatrix_ = true;
    }
}

/**
 * @brief Create any demo models we want for this demo.
 */
void Renderer::createModels() {

    auto assetManager = app_->activity->assetManager;

    auto *importer = new Assimp::Importer();
    auto *ioSystem = new Assimp::AndroidJNIIOSystem(
            reinterpret_cast<ANativeActivity *>(app_->activity));

    importer->SetIOHandler(ioSystem);

    std::shared_ptr<ModelImporter> modelImporter = std::make_shared<ModelImporter>(assetManager, shaderLoader_.get());
    //Load one model
    std::shared_ptr<MeshRenderer> environment = modelImporter->import(importer,
                                                                      "megatron__transformers_dotm/scene.gltf");
    float scale = 0.2;
    environment->transform->setPosition(0, 0, 4);
    environment->transform->setScale(scale, scale, scale);
    environment->transform->setRotation(0, 0, 0);

    scene_->addObject(environment);

    std::shared_ptr<DirectionalLight> light = std::make_shared<DirectionalLight>();
    light->ambientIntensity = 0.8f;
    light->direction  = { 4, 2, 6};
    light->diffuseIntensity = 1.0f;
    light->color = {1, 1, 1, 1};

   /* std::shared_ptr<PointLight> light = std::make_shared<PointLight>();
    light->transform->position = { 2, 0, 12};
    light->color = {0.8, 0.2, 0.2, 1.0};
    light->attenuation.constant = 0.9;
    light->attenuation.linear = 0.01;
    light->attenuation.exp = 0.0;
   // light->cutOff = 20.0;
    //light->transform->position = {0.0, 0.0, 0.0};
    //light->direction = {0.0, 2.0, 0.0};
    shaderLoader_->setNumOfLights(1);*/

    scene_->addObject(light);

    scene_->getMainCamera()->setPosition(0, 4, 0);
    glm::vec3 target = environment->transform->position;
    target.y = 3;
    scene_->getMainCamera()->setTarget(target);
}

void Renderer::handleInput() {
    // handle all queued inputs
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        // no inputs yet.
        return;
    }

    // handle motion events (motionEventsCounts can be 0).
    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;

        // Find the pointer index, mask and bitshift to turn it into a readable value.
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        aout << "Pointer(s): ";

        // get the x and y position of this event if it is not ACTION_MOVE.
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        // determine the action type and process the event accordingly.
        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:

            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                aout << "(" << pointer.id << ", " << x << ", " << y << ") "
                     << "Pointer Down";
                lastX = x;
                lastY = y;
                isMoving = true;
                break;

            case AMOTION_EVENT_ACTION_CANCEL:
                // treat the CANCEL as an UP event: doing nothing in the app, except
                // removing the pointer from the cache if pointers are locally saved.
                // code pass through on purpose.
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                aout << "(" << pointer.id << ", " << x << ", " << y << ") "
                     << "Pointer Up";
                isMoving = false;
                initialDistance = 0;
                break;

            case AMOTION_EVENT_ACTION_MOVE:
                // There is no pointer index for ACTION_MOVE, only a snapshot of
                // all active pointers; app needs to cache previous active pointers
                // to figure out which ones are actually moved.
                if (motionEvent.pointerCount == 1) {
                    pointer = motionEvent.pointers[0];
                    x = GameActivityPointerAxes_getX(&pointer);
                    y = GameActivityPointerAxes_getY(&pointer);
                    float deltaX = x - lastX;
                    float deltaY = y - lastY;
                    aout << "deltaX : " << deltaX;
                    aout << "deltaY : " << deltaY;
                    scene_->getMainCamera()->onMove(deltaX, deltaY);

                }

                if(motionEvent.pointerCount == 2){
                    pointer = motionEvent.pointers[0];
                    auto pointer2 = motionEvent.pointers[1];
                    float x1 = GameActivityPointerAxes_getX(&pointer);
                    float y1 = GameActivityPointerAxes_getY(&pointer);
                    float x2 = GameActivityPointerAxes_getX(&pointer2);
                    float y2 = GameActivityPointerAxes_getY(&pointer2);

                    float deltaX = (x2 - x1) / width_;
                    float deltaY = (y2 - y1) / height_;
                    float currentDistance = 0;
                    currentDistance = sqrtf(deltaX * deltaX + deltaY * deltaY);
                    aout << "deltaX : " << deltaX;
                    aout << "deltaY : " << deltaY;
                    float distanceMoved = currentDistance - initialDistance;

                    if(abs(distanceMoved) < 0.001 && abs(initialDistance ) > 0){
                        float movedX = (x1 - lastX) / width_;
                        float movedY = (y1 - lastY) / height_;
                        // Move the camera left or right based on the x movement
                        scene_->getMainCamera()->moveLeft(movedX); // Scale the movement for smoother panning
                        scene_->getMainCamera()->moveUp(-movedY);   // Scale the movement for smoother panning
                    }else if(abs(initialDistance) > 0){
                        scene_->getMainCamera()->moveForward(distanceMoved);
                    }

                    initialDistance = currentDistance;

                }
                lastX = x;
                lastY = y;
                for (auto index = 0; index < motionEvent.pointerCount; index++) {
                    pointer = motionEvent.pointers[index];
                    x = GameActivityPointerAxes_getX(&pointer);
                    y = GameActivityPointerAxes_getY(&pointer);
                    aout << "(" << pointer.id << ", " << x << ", " << y << ")";
                    if (index != (motionEvent.pointerCount - 1)) aout << ",";
                    aout << " ";
                }
                aout << "Pointer Move";
                break;
            default:
                aout << "Unknown MotionEvent Action: " << action;
        }
        aout << std::endl;
    }
    // clear the motion input count in this buffer for main thread to re-use.
    android_app_clear_motion_events(inputBuffer);

    // handle input key events.
    for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
        auto &keyEvent = inputBuffer->keyEvents[i];
        aout << "Key: " << keyEvent.keyCode << " ";
        switch (keyEvent.action) {
            case AKEY_EVENT_ACTION_DOWN:
                aout << "Key Down";
                break;
            case AKEY_EVENT_ACTION_UP:
                aout << "Key Up";
                break;
            case AKEY_EVENT_ACTION_MULTIPLE:
                // Deprecated since Android API level 29.
                aout << "Multiple Key Actions";
                break;
            default:
                aout << "Unknown KeyEvent Action: " << keyEvent.action;
        }
        aout << std::endl;
    }
    // clear the key input count too.
    android_app_clear_key_events(inputBuffer);
}

void Renderer::initScene() {
    if (scene_ != nullptr) {
        scene_->setSize(width_, height_);
    } else {
        scene_ = std::make_shared<Scene>(width_, height_);
        createModels();
    }
}

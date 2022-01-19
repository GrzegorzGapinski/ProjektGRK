#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_SWIZZLE
#define SHOW_RAY    
#include "stb_image.h"


#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"
#include "Physics.h"


bool DRAGING_ON = false;

Core::Shader_Loader shaderLoader;
GLuint programColor;
GLuint programTexture, programRed;

obj::Model planeModel, boxModel, sphereModel;
Core::RenderContext planeContext, boxContext, sphereContext;
Core::RayContext rayContext;
GLuint boxTexture, groundTexture;

glm::vec3 cameraPos = glm::vec3(0, 5, 20);
glm::vec3 cameraDir;
glm::vec3 cameraSide;
float cameraAngle = 0;
glm::mat4 cameraMatrix, perspectiveMatrix;

glm::vec3 lightDir = glm::normalize(glm::vec3(0.5, -1, -0.5));


// Initalization of physical scene (PhysX)
Physics pxScene(9.8 /* gravity (m/s^2) */);

// fixed timestep for stable and deterministic simulation
const double physicsStepTime = 1.f / 60.f;
double physicsTimeToProcess = 0;

// physical objects
PxRigidStatic *planeBody = nullptr;
PxMaterial *planeMaterial = nullptr;
std::vector<PxRigidDynamic*> boxBodies;
PxMaterial *boxMaterial = nullptr;
PxRigidDynamic *sphereBody = nullptr;
PxMaterial *sphereMaterial = nullptr;

struct GrabbedObject {
    PxRigidDynamic* actor;
    float distance;
    PxVec3 newPos;
    bool update = false;
} grabbedObject;

// renderable objects (description of a single renderable instance)
struct Renderable {
    Core::RenderContext *context;
    glm::mat4 modelMatrix;
    GLuint textureId;
};
std::vector<Renderable*> renderables;

PxVec3 vec3ToPxVec(glm::vec3 vector) {
    return PxVec3(vector.x, vector.y, vector.z);
}
glm::vec3 PxVecTovec3(PxVec3 vector) {
    return glm::vec3(vector.x, vector.y, vector.z);
}

// number of rows and columns of boxes the wall consists of
const int BOX_NUMBER = 10;

void initRenderables()
{
    // load models
    planeModel = obj::loadModelFromFile("models/plane.obj");
    boxModel = obj::loadModelFromFile("models/box.obj");
    sphereModel = obj::loadModelFromFile("models/sphere.obj");

    planeContext.initFromOBJ(planeModel);
    boxContext.initFromOBJ(boxModel);
    sphereContext.initFromOBJ(sphereModel);


    // load textures
    groundTexture = Core::LoadTexture("textures/sand.jpg");
    boxTexture = Core::LoadTexture("textures/a.jpg");

    // create ground
    Renderable *ground = new Renderable();
    ground->context = &planeContext;
    ground->textureId = groundTexture;
    renderables.emplace_back(ground);

    for (int i = 0; i < BOX_NUMBER; i++){
				// create box
				Renderable* box = new Renderable();
				box->context = &boxContext;
				box->textureId = boxTexture;
				renderables.emplace_back(box);
			
    }

}

void initPhysicsScene()
{
    //-----------------------------------------------------------
    // TASKS
    //-----------------------------------------------------------
    // IMPORTANT:
    //   * to create objects use:     pxScene.physics
    //   * to manage the scene use:   pxScene.scene

	planeBody = pxScene.physics->createRigidStatic(PxTransformFromPlaneEquation(PxPlane(0, 1, 0, 0)));
	planeMaterial = pxScene.physics->createMaterial(0.5, 0.3, 0.5);
	PxShape* planeShape = pxScene.physics->createShape(PxPlaneGeometry(), *planeMaterial);
	planeBody->attachShape(*planeShape);
	planeShape->release();
	planeBody->userData = (void*)renderables[0];
	pxScene.scene->addActor(*planeBody);
	boxMaterial = pxScene.physics->createMaterial(0.9, 0.5, 0.4);

	for (int i = 0; i < BOX_NUMBER; i++) {
        auto pos = glm::linearRand(glm::vec2(-8.f, -8.f), glm::vec2(8.f, 8.f));
		auto x = pxScene.physics->createRigidDynamic(PxTransform(pos.x,3, pos.y));
		boxBodies.push_back(x);
		PxShape* boxShape = pxScene.physics->createShape(PxBoxGeometry(1, 1, 1), *boxMaterial);
		x->attachShape(*boxShape);
		boxShape->release();
		x->userData = (void*)renderables[i + 1];
		pxScene.scene->addActor(*x);
	}

}

void updateTransforms()
{
    // Here we retrieve the current transforms of the objects from the physical simulation.
    auto actorFlags = PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC;
    PxU32 nbActors = pxScene.scene->getNbActors(actorFlags);
    if (nbActors)
    {
        std::vector<PxRigidActor*> actors(nbActors);
        pxScene.scene->getActors(actorFlags, (PxActor**)&actors[0], nbActors);
        for (auto actor : actors)
        {
            // We use the userData of the objects to set up the model matrices
            // of proper renderables.
            if (!actor->userData) continue;
            Renderable *renderable = (Renderable*)actor->userData;

            // get world matrix of the object (actor)
            PxMat44 transform = actor->getGlobalPose();
            auto &c0 = transform.column0;
            auto &c1 = transform.column1;
            auto &c2 = transform.column2;
            auto &c3 = transform.column3;

            // set up the model matrix used for the rendering
            renderable->modelMatrix = glm::mat4(
                c0.x, c0.y, c0.z, c0.w,
                c1.x, c1.y, c1.z, c1.w,
                c2.x, c2.y, c2.z, c2.w,
                c3.x, c3.y, c3.z, c3.w);
        }
    }
}
std::vector<glm::vec3> calculate_ray(float x, float y) {
    glm::vec2 screen_space_pos(x, y);
    std::vector<glm::vec3> result;

    //here ray should be calculated

    result.push_back(cameraPos);
    result.push_back(cameraDir);
    return result;
}

void keyboard(unsigned char key, int x, int y)
{
    float angleSpeed = 0.1f;
    float moveSpeed = 0.1f;
    switch (key)
    {
		case 'z': cameraAngle -= angleSpeed; break;
		case 'x': cameraAngle += angleSpeed; break;
		case 'w': cameraPos += cameraDir * moveSpeed; break;
		case 's': cameraPos -= cameraDir * moveSpeed; break;
		case 'd': cameraPos += cameraSide * moveSpeed; break;
		case 'a': cameraPos -= cameraSide * moveSpeed; break;
    }


    int size_x = glutGet(GLUT_WINDOW_WIDTH);
    int size_y = glutGet(GLUT_WINDOW_HEIGHT);
    std::vector<glm::vec3> ray = calculate_ray((x / float(size_x) - 0.5) * 2, -((y / float(size_y)) - 0.5) * 2);
    grabbedObject.newPos = vec3ToPxVec(ray[1]) * grabbedObject.distance + vec3ToPxVec(ray[0]);
}


void mouse(int x, int y)
{

    int size_x = glutGet(GLUT_WINDOW_WIDTH);
    int size_y = glutGet(GLUT_WINDOW_HEIGHT);
    std::vector<glm::vec3> ray = calculate_ray((x / float(size_x) - 0.5) * 2, -((y / float(size_y)) - 0.5) * 2);
    Core::updateRayPos(rayContext, ray);
    grabbedObject.newPos = vec3ToPxVec(ray[1]) * grabbedObject.distance + vec3ToPxVec(ray[0]);

}
void click_mouse(int button, int state, int x, int y) {
    if ((GLUT_LEFT_BUTTON == button && state == GLUT_DOWN)) {

        int size_x = glutGet(GLUT_WINDOW_WIDTH);
        int size_y = glutGet(GLUT_WINDOW_HEIGHT);
        std::vector<glm::vec3> ray = calculate_ray((x / float(size_x) - 0.5) * 2, -((y / float(size_y)) - 0.5) * 2);
        Core::updateRayPos(rayContext, ray);

        //here raycast should be done

        //check if there is a hit
        if (false) {

            //check if it is rigid dynamic
            if (false) {
            }
        }
        else {
            std::cout << "no hit\n";
        }
    }

    if (GLUT_LEFT_BUTTON == button && state == GLUT_UP) {
        DRAGING_ON = false;
        grabbedObject.update = false;
    }

}



glm::mat4 createCameraMatrix()
{
    cameraDir = glm::normalize(glm::vec3(cosf(cameraAngle - glm::radians(90.0f)), 0, sinf(cameraAngle - glm::radians(90.0f))));
    glm::vec3 up = glm::vec3(0, 1, 0);
    cameraSide = glm::cross(cameraDir, up);

    return Core::createViewMatrix(cameraPos, cameraDir, up);
}

void drawObjectColor(Core::RenderContext* context, glm::mat4 modelMatrix, glm::vec3 color)
{
    GLuint program = programColor;

    glUseProgram(program);

    glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);
    glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);

    glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    context->render();

    glUseProgram(0);
}

void drawRay(Core::RayContext& context) {
    glUseProgram(programRed);

    glm::mat4 transformation = perspectiveMatrix * cameraMatrix;
    glUniformMatrix4fv(glGetUniformLocation(programRed, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
    context.render();
    glUseProgram(0);
}

void drawObjectTexture(Core::RenderContext * context, glm::mat4 modelMatrix, GLuint textureId)
{
    GLuint program = programTexture;

    glUseProgram(program);

    glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
    Core::SetActiveTexture(textureId, "textureSampler", program, 0);

    glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    context->render();
    glUseProgram(0);
}

void renderScene()
{
    
    if (grabbedObject.update) {
        // Here should be grab object update
    }

    double time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    static double prevTime = time;
    double dtime = time - prevTime;
    prevTime = time;

    // Update physics
    if (dtime < 1.f) {
        physicsTimeToProcess += dtime;
        while (physicsTimeToProcess > 0) {
            // here we perform the physics simulation step
            pxScene.step(physicsStepTime);
            physicsTimeToProcess -= physicsStepTime;
        }
    }


    // Update of camera and perspective matrices
    cameraMatrix = createCameraMatrix();
    perspectiveMatrix = Core::createPerspectiveMatrix();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.1f, 0.3f, 1.0f);

    // update transforms from physics simulation
    updateTransforms();

    // render models
    for (Renderable* renderable : renderables) {
        drawObjectTexture(renderable->context, renderable->modelMatrix, renderable->textureId);
    }
    #ifdef SHOW_RAY
        drawRay(rayContext);
    #endif // SHOW_RAY


    glutSwapBuffers();
}

void init()
{
    srand(time(0));
    glEnable(GL_DEPTH_TEST);
    programColor = shaderLoader.CreateProgram("shaders/shader_color.vert", "shaders/shader_color.frag");
    programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
    programRed = shaderLoader.CreateProgram("shaders/shader_red.vert", "shaders/shader_red.frag");


    Core::initRay(rayContext);
    auto ray = calculate_ray(0.5f,0.5f);
    //Core::updateRayPos(rayContext, ray);

    initRenderables();
    initPhysicsScene();
}

void shutdown()
{
    shaderLoader.DeleteProgram(programColor);
    shaderLoader.DeleteProgram(programTexture);
}

void idle()
{
    glutPostRedisplay();
}

int main(int argc, char ** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(200, 200);
    glutInitWindowSize(600, 600);
    glutCreateWindow("OpenGL + PhysX");
    glewInit();

    init();
    glutKeyboardFunc(keyboard);
    glutMotionFunc(mouse);
    glutMouseFunc(click_mouse);
    glutDisplayFunc(renderScene);
    glutIdleFunc(idle);

    glutMainLoop();

    shutdown();

    return 0;
}

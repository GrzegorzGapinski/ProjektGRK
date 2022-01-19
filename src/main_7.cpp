#define STB_IMAGE_IMPLEMENTATION

#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "gtx/matrix_decompose.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <ctime>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"


#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
//#include <assimp/pbrmaterial.h>

#include "stb_image.h"
#include "Texture.h"

//obliczyc kwateriony nastepnie interpolowac.


std::vector<glm::vec3> keyPoints({
glm::vec3(-711.745f, 89.9272f, -626.537f),
//glm::vec3(-711.745f, 91.9272f, -606.537f),
glm::vec3(-687.635f, 100.428f, -503.943f),
//glm::vec3(-721.365f, 103.613f, -598.223f),
glm::vec3(-667.635f, 128.428f, -433.943f),
glm::vec3(-547.654f, 180.445f, -401.846f),
glm::vec3(-365.357f, 261.268f, -304.93f),
glm::vec3(-346.51f, 146.605f, -85.3702f),
glm::vec3(-461.105f, 120.275f, 115.596f),
glm::vec3(-507.395f, 76.497f, 338.408f),
glm::vec3(-181.343f, 58.7994f, 403.918f),
glm::vec3(-148.073f, 72.7797f, 522.283f),
glm::vec3(-76.8437f, 85.1488f, 524.396f),
glm::vec3(-30.0008f, 81.3007f, 367.907f),
glm::vec3(20.808f, 117.73f, 109.607f),
glm::vec3(8.72873f, 135.983f, -130.435f),
glm::vec3(8.72873f, 115.983f, -132.435f),
glm::vec3(8.72873f, 104.983f, -132.435f),
glm::vec3(8.72873f, 100.983f, -132.435f),
	});

std::vector<glm::quat> keyRotation;

int index = 0;
bool FOLLOW_CAR = false;

GLuint program;
GLuint programTextureSpecular;
GLuint programTexture;
GLuint programSun;
Core::Shader_Loader shaderLoader;



Core::RenderContext armContext;


std::vector<Core::RenderContext> armContexts;

std::vector<Core::Node> city;

std::vector<Core::Node> car;


float cameraAngle = 0;
glm::vec3 cameraSide;
glm::vec3 cameraDir;
glm::vec3 cameraPos = keyPoints[0] + glm::vec3(0, 10, -5);

glm::mat4 cameraMatrix, perspectiveMatrix;



float old_x, old_y = -1;
float delta_x, delta_y = 0;
glm::quat rotationCamera = glm::quat(1, 0, 0, 0);
//glm::quat rotation_y;
//glm::quat rotation_x;// = glm::angleAxis(glm::pi<float>() * 0.25f, glm::vec3(0, 1, 0));
glm::quat rotation_y = glm::normalize(glm::angleAxis(209 * 0.03f, glm::vec3(1, 0, 0)));
glm::quat rotation_x = glm::normalize(glm::angleAxis(307 * 0.03f, glm::vec3(0, 1, 0)));
float dy = 0;
float dx = 0;

void keyboard(unsigned char key, int x, int y)
{
	float angleSpeed = 0.5f;
	float moveSpeed = 2.f;
	switch (key)
	{
	case 'z': cameraAngle -= angleSpeed; break;
	case 'x': cameraAngle += angleSpeed; break;
	case 'w': cameraPos += cameraDir * moveSpeed; break;
	case 's': cameraPos -= cameraDir * moveSpeed; break;
	case 'd': cameraPos += cameraSide * moveSpeed; break;
	case 'a': cameraPos -= cameraSide * moveSpeed; break;
	case '0': cameraPos = keyPoints[0] + glm::vec3(0, 3, 0); index = 0; break;
	case 'e': index = (index + 1) % keyPoints.size(); cameraPos = keyPoints[index] + glm::vec3(-3, 20, -3);  break;
	case 'q': index = (keyPoints.size() + index - 1) % keyPoints.size(); cameraPos = keyPoints[index] + glm::vec3(-3, 20, -3);  break;
	case '1': FOLLOW_CAR = !FOLLOW_CAR;  break;
	case 'r': cameraPos = glm::vec3(0,0,1); break;
	}
}

glm::vec3 lightDir = glm::normalize(glm::vec3(1, 1, 1));


void mouse(int x, int y)
{
	if (old_x >= 0) {
		delta_x = x - old_x;
		delta_y = y - old_y;
	}
	old_x = x;
	old_y = y;
}


glm::mat4 createCameraMatrix()
{
	auto rot_y = glm::angleAxis(delta_y * 0.03f, glm::vec3(1, 0, 0));
	auto rot_x = glm::angleAxis(delta_x * 0.03f, glm::vec3(0, 1, 0));

	dy += delta_y;
	dx += delta_x;
	delta_x = 0;
	delta_y = 0;

	rotation_x = glm::normalize(rot_x * rotation_x);
	rotation_y = glm::normalize(rot_y * rotation_y);

	rotationCamera = glm::normalize(rotation_y * rotation_x);

	auto inverse_rot = glm::inverse(rotationCamera);

	cameraDir = inverse_rot * glm::vec3(0, 0, -1);
	glm::vec3 up = glm::vec3(0, 1, 0);
	cameraSide = inverse_rot * glm::vec3(1, 0, 0);

	glm::mat4 cameraTranslation;
	cameraTranslation[3] = glm::vec4(-cameraPos, 1.0f);

	return glm::mat4_cast(rotationCamera) * cameraTranslation;
}
glm::mat4 animationMatrix(float time) {
	float speed = 1.;
	time = time * speed;
	std::vector<float> distances;
	float timeStep = 0;
	for (int i = 0; i < keyPoints.size() - 1; i++) {
		timeStep += (keyPoints[i] - keyPoints[i + 1]).length();
		distances.push_back((keyPoints[i] - keyPoints[i + 1]).length());
	}
	time = fmod(time, timeStep);

	//index of first keyPoint
	int index = 0;

	while (distances[index] <= time) {
		time = time - distances[index];
		index += 1;
	}

	//t coefitient between 0 and 1 for interpolation
	float t = time / distances[index];

	int size = keyPoints.size()-1;
	//replace with catmul rom	
	//glm::vec3 pos = (keyPoints[std::max(0, index)] * t + keyPoints[std::min(size, index + 1)] * (1 - t));
	glm::vec3 pos = glm::catmullRom(keyPoints[std::max(0, index-1)], keyPoints[std::min(size, index)], keyPoints[std::min(size, index + 1)], keyPoints[std::min(size, index + 2)], t);

	//implement corect animation
	//auto animationRotation = glm::quat(1,0,0,0);
	//auto animationRotation = glm::slerp(keyRotation[std::max(0, index)], keyRotation[std::min(size, index + 1)], t);

	keyRotation[size] = glm::quat(1, 0, 0, 0);
	keyRotation[size - 1] = glm::quat(1, 0, 0, 0);
	keyRotation[size - 2] = glm::quat(1, 0, 0, 0);
	keyRotation[size - 3] = glm::quat(0, 0, 1, 0);

	auto q1 = keyRotation[std::max(0, index - 1)];
	auto q2 = keyRotation[std::min(size, index)];
	auto q3 = keyRotation[std::min(size, index + 1)];
	auto q4 = keyRotation[std::min(size, index + 2)];

	auto a1 = q2 * glm::exp((glm::log(glm::inverse(q2) * q1) + glm::log(glm::inverse(q2) * q3)) / (-4.f));
	auto a2 = q3 * glm::exp((glm::log(glm::inverse(q3) * q2) + glm::log(glm::inverse(q3) * q4)) / (-4.f));
	
	auto animationRotation = glm::squad(q2, q3, a1, a2, t);

	glm::mat4 result = glm::translate(pos) * glm::mat4_cast(animationRotation);


	return result;

}


void drawObject(GLuint program, Core::RenderContext context, glm::mat4 modelMatrix)
{
	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;


	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	context.render();
}

void renderRecursive(std::vector<Core::Node>& nodes) {
	for (auto node : nodes) {
		if (node.renderContexts.size() == 0) {
			continue;
		}

		glm::mat4 transformation = node.matrix;

		Core::Node currentNode = node;
		while (currentNode.parent != -1)
		{
			currentNode = nodes[currentNode.parent];
			transformation = currentNode.matrix * transformation;
		}

		// dodaj odwolania do nadrzednych zmiennych
		for (auto context : node.renderContexts) {
			auto program = context.material->program;
			glUseProgram(program);
			glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
			context.material->init_data();
			drawObject(program, context, transformation);
		}
	}

}

glm::mat4 followCarCamera(float time) {
	glm::quat rotation_y = glm::normalize(glm::angleAxis(209 * 0.03f, glm::vec3(1, 0, 0)));
	glm::quat rotation_x = glm::normalize(glm::angleAxis(3.14f, glm::vec3(0, 1, 0)));
	auto pos = glm::vec3(2, 3, 8);
	cameraPos = animationMatrix(time) * glm::vec4(pos, 1);
	auto transformation = animationMatrix(time);
	glm::vec3 scale;
	glm::quat rrotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transformation, scale, rrotation, translation, skew, perspective);

	return glm::translate(-pos) * glm::mat4_cast(rotation_y * rotation_x) * glm::inverse(transformation);

}

void renderScene()
{
	// Aktualizacja macierzy widoku i rzutowania. Macierze sa przechowywane w zmiennych globalnych, bo uzywa ich funkcja drawObject.
	// (Bardziej elegancko byloby przekazac je jako argumenty do funkcji, ale robimy tak dla uproszczenia kodu.
	//  Jest to mozliwe dzieki temu, ze macierze widoku i rzutowania sa takie same dla wszystkich obiektow!)
	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix(0.1, 2000);
	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.f;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.03f, 0.1f, 1.0f);





	if (FOLLOW_CAR) {
		cameraMatrix = followCarCamera(time);
	}

	renderRecursive(city);
	for (int i = 0; i < 30; i++) {
		if (time > -10) {
			car[0].matrix = animationMatrix(time + 15);;
			renderRecursive(car);
			time -= 3;
		}
	}
	glUseProgram(0);
	glutSwapBuffers();
}




Core::Material* loadDiffuseMaterial(aiMaterial* material) {
	aiString colorPath;
	// use for loading textures

	material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), colorPath);
	if (colorPath == aiString("")) {
		return nullptr;
	}
	Core::DiffuseMaterial* result = new Core::DiffuseMaterial();
	result->texture = Core::LoadTexture(colorPath.C_Str());
	result->program = programTexture;
	result->lightDir = lightDir;

	return result;
}


Core::Material* loadDiffuseSpecularMaterial(aiMaterial* material) {
	aiString colorPath;
	// use for loading textures

	material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), colorPath);
	Core::DiffuseSpecularMaterial* result = new Core::DiffuseSpecularMaterial();
	result->texture = Core::LoadTexture(colorPath.C_Str());


	aiString specularPath;
	material->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), specularPath);
	result->textureSpecular = Core::LoadTexture(specularPath.C_Str());
	result->lightDir = lightDir;
	result->program = programTextureSpecular;

	return result;
}


void loadRecusive(const aiScene* scene, aiNode* node, std::vector<Core::Node>& nodes, std::vector<Core::Material*> materialsVector, int parentIndex) {
	int index = nodes.size();
	nodes.push_back(Core::Node());
	nodes[index].parent = parentIndex;
	nodes[index].matrix = Core::mat4_cast(node->mTransformation);
	for (int i = 0; i < node->mNumMeshes; i++) {
		Core::RenderContext context;
		context.initFromAssimpMesh(scene->mMeshes[node->mMeshes[i]]);
		context.material = materialsVector[scene->mMeshes[node->mMeshes[i]]->mMaterialIndex];
		nodes[index].renderContexts.push_back(context);
	}
	for (int i = 0; i < node->mNumChildren; i++) {
		loadRecusive(scene, node->mChildren[i], nodes, materialsVector, index);
	}
}
void loadRecusive(const aiScene* scene, std::vector<Core::Node>& nodes, std::vector<Core::Material*> materialsVector) {

	loadRecusive(scene, scene->mRootNode, nodes, materialsVector, -1);
}

void initModels() {
	Assimp::Importer importer;
	//replace to get more buildings, unrecomdnded
	//const aiScene* scene = importer.ReadFile("models/blade-runner-style-cityscapes.fbx", aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
	const aiScene* scene = importer.ReadFile("models/city_small.fbx", aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
	// check for errors
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}



	std::vector<Core::Material*> materialsVector;

	for (int i = 0; i < scene->mNumMaterials; i++) {
		materialsVector.push_back(loadDiffuseSpecularMaterial(scene->mMaterials[i]));
	}
	loadRecusive(scene, city, materialsVector);


	scene = importer.ReadFile("models/flying_car.fbx", aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);


	// check for errors
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}
	materialsVector.clear();

	for (int i = 0; i < scene->mNumMaterials; i++) {
		materialsVector.push_back(loadDiffuseMaterial(scene->mMaterials[i]));
	}
	loadRecusive(scene, car, materialsVector);


	//Recovering points from fbx
	//const aiScene* points = importer.ReadFile("models/path.fbx", aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

	//auto root = points->mRootNode;

	//for (int i = 0; i < root->mNumChildren; i++) {
	//	auto node = root->mChildren[i];
	//	std::cout << "glm::vec3(" << node->mTransformation.a4 << "f, " << node->mTransformation.b4 << "f, " << node->mTransformation.c4 << "f), " << std::endl;
	//	//std::cout << node->mName.C_Str() << std::endl;
	//}
}

void initKeyRoation() {
	
	glm::vec3 oldDirection = glm::vec3(0, 0, 1);
	glm::quat oldRotationCamera = glm::quat(1, 0, 0, 0);

	for (int i = 0; i < keyPoints.size() - 1; i++) {
		glm::vec3 newDir = keyPoints[i + 1] - keyPoints[i];
		rotationCamera = glm::normalize(glm::rotationCamera(glm::normalize(oldDirection), glm::normalize(newDir)) * oldRotationCamera); 
		keyRotation.push_back(rotationCamera);
		oldDirection = newDir;
		oldRotationCamera = rotationCamera;
	}
	keyRotation.push_back(glm::quat(1, 0, 0, 0));
	
}

void init()
{

	glEnable(GL_DEPTH_TEST);
	program = shaderLoader.CreateProgram("shaders/shader_4_1.vert", "shaders/shader_4_1.frag");
	programTextureSpecular = shaderLoader.CreateProgram("shaders/shader_spec_tex.vert", "shaders/shader_spec_tex.frag");
	programTexture = shaderLoader.CreateProgram("shaders/shader_tex_2.vert", "shaders/shader_tex_2.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_4_sun.vert", "shaders/shader_4_sun.frag");

	initModels();

	initKeyRoation();

}

void shutdown()
{
	shaderLoader.DeleteProgram(program);
}

void idle()
{
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutSetOption(GLUT_MULTISAMPLE, 2);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowPosition(200, 300);
	glutInitWindowSize(800, 800);
	glutCreateWindow("OpenGL Pierwszy Program");
	glewInit();

	init();
	glutPassiveMotionFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutMainLoop();

	shutdown();

	return 0;
}

#include "SceneManager.h"
#include "quaternion.h"
#include <GLFW/glfw3.h>

#define PI_F 3.14159265358979f

SceneManager::SceneManager(int wind_width, int wind_height, GLWrapper* wrapper)
{
	this->wind_width = wind_width;
	this->wind_height = wind_height;
	this->wrapper = wrapper;
}

rt_scene& SceneManager::getScene()
{
	return scene;
}

void SceneManager::init()
{
	glfwSetWindowUserPointer(wrapper->window, this);

	auto mouseFunc = [](GLFWwindow* w, double x, double y)
	{
		static_cast<SceneManager*>(glfwGetWindowUserPointer(w))->glfw_mouse_callback(w, x, y);
	};
	auto keyFunc = [](GLFWwindow* w, int a, int b, int c, int d)
	{
		static_cast<SceneManager*>(glfwGetWindowUserPointer(w))->glfw_key_callback(w, a, b, c, d);
	};

	glfwSetInputMode(wrapper->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(wrapper->window, mouseFunc);
	glfwSetKeyCallback(wrapper->window, keyFunc);
	glfwSetFramebufferSizeCallback(wrapper->window, glfw_framebuffer_size_callback);

	initBuffers();
}

void SceneManager::update(float frameRate)
{
	UpdateScene(frameRate);
	updateBuffers();
}

void SceneManager::multiplyVector(float v[3], float s) {
	v[0] *= s;
	v[1] *= s;
	v[2] *= s;
}

void SceneManager::addVector(vec3 &v1, const float v2[3]) {
	v1.x += v2[0];
	v1.y += v2[1];
	v1.z += v2[2];
}

void SceneManager::moveCamera(Quaternion<float> &q, const float direction[3], vec3 &vector, float speed) {
	float tmp[3] = { direction[0], direction[1], direction[2] };
	q.QuatRotation(tmp);
	multiplyVector(tmp, speed);
	addVector(vector, tmp);
}

void SceneManager::moveCamera(const float direction[3], vec3 &vector, float speed) {
	float tmp[3] = { direction[0], direction[1], direction[2] };
	multiplyVector(tmp, speed);
	addVector(vector, tmp);
}

void SceneManager::ProcessRotations(float frameRate)
{
	for (int i = 0; i < rotating_primitives.size(); ++i)
	{
		auto rot = rotating_primitives.data() + i;
		switch (rot->type)
		{
		case sphere:
		{
			auto sphere = static_cast<rt_sphere*>(rot->primitive);
			rot->current += frameRate * rot->speed;

			// sphere->pos.x = rot->a * cos(rot->current);
			// sphere->pos.z = rot->b * sin(rot->current);

			break;
		}
		}
	}
}

void SceneManager::UpdateScene(float frameRate)
{
	const float xAxis[3] = { 1, 0, 0 };
	const float yAxis[3] = { 0, 1, 0 };
	const float zAxis[3] = { 0, 0, 1 };

	Quaternion<float> qX(xAxis, -pitch * PI_F / 180.0f);
	Quaternion<float> qY(yAxis, yaw * PI_F / 180.0f);
	Quaternion<float> q = qY * qX;
	scene.quat_camera_rotation = q.GetStruct();


	auto speed = frameRate;
	if (shift_pressed)
		speed *= 3;
	if (alt_pressed)
		speed /= 6;

	if (w_pressed)
		moveCamera(q, zAxis, scene.camera_pos, speed);
	if (a_pressed)
		moveCamera(q, xAxis, scene.camera_pos, -speed);
	if (s_pressed)
		moveCamera(q, zAxis, scene.camera_pos, -speed);
	if (d_pressed)
		moveCamera(q, xAxis, scene.camera_pos, speed);

	if (space_pressed)
		moveCamera(yAxis, scene.camera_pos, speed);
	if (ctrl_pressed)
		moveCamera(yAxis, scene.camera_pos, -speed);

	ProcessRotations(frameRate);
}

void SceneManager::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_RELEASE) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, GL_TRUE);

		bool pressed = action == GLFW_PRESS;

		if (key == GLFW_KEY_W)
			w_pressed = pressed;
		else if (key == GLFW_KEY_S)
			s_pressed = pressed;
		else if (key == GLFW_KEY_A)
			a_pressed = pressed;
		else if (key == GLFW_KEY_D)
			d_pressed = pressed;
		else if (key == GLFW_KEY_SPACE)
			space_pressed = pressed;
		else if (key == GLFW_KEY_LEFT_CONTROL)
			ctrl_pressed = pressed;
		else if (key == GLFW_KEY_LEFT_SHIFT)
			shift_pressed = pressed;
		else if (key == GLFW_KEY_LEFT_ALT)
			alt_pressed = pressed;
	}
}

void SceneManager::glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height)
{
	glViewport(0, 0, width, height);
}

bool firstMouse = true;

void SceneManager::glfw_mouse_callback(GLFWwindow* window, double xpos, double ypos) {

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;



	float sensitivity = 0.05f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
}

rt_material SceneManager::create_material(vec3 color, int specular, float reflect, float refract = 0.0, vec3 absorb = {}, float diffuse = 0.7, float kd = 0.8, float ks = 0.2)
{
    rt_material material = {};

    material.color = color;
    material.absorb = absorb;
    material.specular = specular;
    material.reflect = reflect;
    material.refract = refract;
    material.diffuse = diffuse;
    material.kd = kd;
    material.ks = ks;

    return material;
}

rt_sphere SceneManager::create_sphere(vec3 center, float radius, rt_material material)
{
	rt_sphere sphere = {};
	sphere.obj = { center.x, center.y, center.z, radius };

	// sphere.pos = center;
	// sphere.radius = radius;
	//
	// sphere.material = material;

	return sphere;
}

rt_plain SceneManager::create_plain(vec3 normal, vec3 pos, rt_material material)
{
    rt_plain plain = {};
    plain.normal = normal;
    plain.pos = pos;
    plain.material = material;
    return plain;
}

rt_light SceneManager::create_light(lightType type, float intensity, vec3 color, vec3 position, vec3 direction)
{
	rt_light light = {};

	light.type = type;
	light.intensity = intensity;
	light.pos = position;
	light.direction = direction;
	light.color = color;

	return light;
}

rt_scene SceneManager::create_scene(int width, int height, int spheresCount, int lightCount, int plainCount)
{
	auto min = width > height ? height : width;

	rt_scene scene = {};

	scene.camera_pos = {};
	scene.canvas_height = height;
	scene.canvas_width = width;
	scene.viewport_dist = 1;
	scene.viewport_height = height / static_cast<float>(min);
	scene.viewport_width = width / static_cast<float>(min);
	scene.bg_color = { 0,0.0,0.0 };
	scene.reflect_depth = 3;

	scene.sphere_count = spheresCount;
	scene.light_count = lightCount;
    scene.plain_count = plainCount;

	return scene;
}

void SceneManager::initBuffers()
{
    //create_sphere({2,0,4}, 1, create_material({0,1,0}, 30, 0.1));
	spheres.push_back(create_sphere({0.8,0,-1.5}, 1, create_material({0,1,0}, 30, 1.0)));
	spheres.push_back(create_sphere({ 1, 0.25, 1.5}, 0.3, create_material({ 0,1,0 }, 30, 1.0)));
	//spheres.push_back(create_spheres({ -2,0,4 }, { 0,0,1 }, 1, 500, 0.3f, 0, 0.7));

	//spheres.push_back(create_spheres({ 2,0,5 }, { 0,1,0 }, 2, 10, 0.2f, 0.2f, 5.4f));
	//spheres.push_back(create_spheres({ -2,0,4 }, { 0,0,1 }, 2, 500, 0.3f, 0, 0));
	//spheres.push_back(create_spheres({ 0,-1,3 }, { 1,0,0 }, 1, 500, 0.4f, 0, 0.7));
	//spheres.push_back(create_spheres({ 0,-1001,3 }, { 1,1,1 }, 1000, 50, 0.05f, 0, 0.7));

    //auto sp = create_spheres({ 0,0.8,1 }, { 0,0,0 }, 0.5, 500, 0.01f, 1.125, 0.7);
    //sp.material.absorb = {3, 2, 0.2};
	//spheres.push_back(sp);

	//lights.push_back(create_light(ambient, 0.2f, { 1,1,1 }, { 0 }, { 0 }));
	//lights.push_back(create_light(point, 25.0f, {1.0,1.0,1.0}, { 2, 3, 0 }, { 0 }));
    //lights.push_back(create_light(point, 25.0f, {1.0,1.0,1.0}, { 2, -5, 0 }, { 0 }));
	//lights.push_back(create_light(direct, 2.2f, { 1,1,1 }, { 0 }, { -1,-4,-4 }));

	//spheres.push_back(create_spheres({ 0,0.5,0 }, getColor(66, 247, 136), 0.5, 50, 0.05f, 1.125, 0));
	//rotating_primitives.push_back({ &spheres.back(), sphere, 9, 4, 0, 1 });

    /*rt_material plainMat;
    plainMat.color = {1,1,1};
    plainMat.diffuse = 0.7;
    plainMat.kd = 0.8;
    plainMat.ks = 0.2;*/
    /*plains.push_back(create_plain({0,1,0}, {0,-1,0}, create_material({1,1,1}, 50, 0.1)));
    plains.push_back(create_plain({0,-1,0}, {0,6,0}, create_material({1,1,1}, 50, 0.1)));
    plains.push_back(create_plain({0,0,-1}, {0,0,6}, create_material({1,1,1}, 50, 0.1)));
    plains.push_back(create_plain({0,0,1}, {0,0,-6}, create_material({1,1,1}, 50, 0.1)));
    plains.push_back(create_plain({1,0,0}, {-6,0,0}, create_material({1,1,1}, 50, 0.1)));
    plains.push_back(create_plain({-1,0,0}, {6,0,0}, create_material({1,1,1}, 50, 0.1)));*/
    spheres.push_back(create_sphere({ 0,-1006,0 }, 1000, create_material({1,1,1}, 30, 0.1)));
    spheres.push_back(create_sphere({ 0, 1006,0 }, 1000, create_material({1,1,1}, 30, 0.1)));
    spheres.push_back(create_sphere({ 1006, 0,0 }, 1000, create_material({1,1,1}, 30, 0.1)));
    spheres.push_back(create_sphere({ -1006, 0,0 }, 1000, create_material({1,1,1}, 30, 0.1)));
    spheres.push_back(create_sphere({0, 0, 1006 }, 1000, create_material({1,1,1}, 30, 0.1)));
    spheres.push_back(create_sphere({0, 0, -1006 }, 1000, create_material({1,1,1}, 30, 0.1)));

	scene = create_scene(wind_width, wind_height, spheres.size(), lights.size(), plains.size());

	glGenBuffers(1, &sceneUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, sceneUbo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_scene), NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, sceneUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// glGenBuffers(1, &sphereUbo);
	// glBindBuffer(GL_UNIFORM_BUFFER, sphereUbo);
	// glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_sphere) * spheres.size(), spheres.data(), GL_STATIC_DRAW);
	// glBindBufferBase(GL_UNIFORM_BUFFER, 1, sphereUbo);
	// glBindBuffer(GL_UNIFORM_BUFFER, 0);

	for (int i = 0; i < spheres.size(); i++)
	{
		char element[50];
		sprintf_s(element, "spheres_[%d]", i);
		GLuint loc = glGetUniformLocation(wrapper->renderHandle, element);
		glUniform4f(loc, spheres[i].obj.x, spheres[i].obj.y, spheres[i].obj.z, spheres[i].obj.w);
	}

	// glGenBuffers(1, &sphereUbo);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereUbo);
	// glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_sphere) * spheres.size(), spheres.data(), GL_STATIC_DRAW);
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sphereUbo);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
 //
 //    glGenBuffers(1, &plainSsbo);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, plainSsbo);
	// glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_plain) * plains.size(), plains.data(), GL_STATIC_DRAW);
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, plainSsbo);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
 //
	// glGenBuffers(1, &lightSsbo);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSsbo);
	// glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_light) * lights.size(), lights.data(), GL_STATIC_DRAW);
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, lightSsbo);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SceneManager::updateBuffers() const
{
	//glUniform1i(glGetUniformLocation(wrapper->computeHandle, "sphere_count"), 8);

	glBindBuffer(GL_UNIFORM_BUFFER, sceneUbo);
	GLvoid* scene_p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(scene_p, &scene, sizeof(rt_scene));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

    // if (!spheres.empty())
    // {
    //     glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereUbo);
	   //  GLvoid* spheres_p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	   //  memcpy(spheres_p, spheres.data(), sizeof(rt_sphere) * spheres.size());
	   //  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    // }
    //
    // if (!plains.empty())
    // {
    //     glBindBuffer(GL_SHADER_STORAGE_BUFFER, plainSsbo);
	   //  GLvoid* plains_p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	   //  memcpy(plains_p, plains.data(), sizeof(rt_plain) * plains.size());
	   //  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    // }
}

vec3 SceneManager::getColor(float r, float g, float b)
{
	return vec3{ r / 255, g / 255, b / 255 };
}

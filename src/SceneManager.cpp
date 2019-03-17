#include "SceneManager.h"
#include "quaternion.h"
#include <GLFW/glfw3.h>

SceneManager::SceneManager(int wind_width, int wind_height, GLFWwindow* window)
{
	this->wind_width = wind_width;
	this->wind_height = wind_height;
	this->window = window;
}

rt_scene& SceneManager::getScene()
{
	return scene;
}

void SceneManager::init()
{
	glfwSetWindowUserPointer(window, this);

	auto mouseFunc = [](GLFWwindow* w, double x, double y)
	{
		static_cast<SceneManager*>(glfwGetWindowUserPointer(w))->glfw_mouse_callback(w, x, y);
	};
	auto keyFunc = [](GLFWwindow* w, int a, int b, int c, int d)
	{
		static_cast<SceneManager*>(glfwGetWindowUserPointer(w))->glfw_key_callback(w, a, b, c, d);
	};

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouseFunc);
	glfwSetKeyCallback(window, keyFunc);
	glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

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

void SceneManager::addVector(float4 &v1, const float v2[3]) {
	v1.x += v2[0];
	v1.y += v2[1];
	v1.z += v2[2];
}

void SceneManager::moveCamera(Quaternion<float> &q, const float direction[3], float4 &vector, float speed) {
	float tmp[3] = { direction[0], direction[1], direction[2] };
	q.QuatRotation(tmp);
	multiplyVector(tmp, speed);
	addVector(vector, tmp);
}

void SceneManager::moveCamera(const float direction[3], float4 &vector, float speed) {
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

			sphere->center.x = rot->a * cos(rot->current);
			sphere->center.z = rot->b * sin(rot->current);

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
	const float PI_F = 3.14159265358979f;

	Quaternion<float> qX(xAxis, -pitch * PI_F / 180.0f);
	Quaternion<float> qY(yAxis, yaw * PI_F / 180.0f);
	Quaternion<float> q = qY * qX;
	scene.quat_camera_rotation = q.GetStruct();


	auto speed = frameRate;
	if (shift_pressed)
		speed *= 3;

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

rt_sphere SceneManager::create_spheres(float4 center, float4 color, float radius, int specular, float reflect, float refract)
{
	rt_sphere sphere = {};

	sphere.center = center;
	sphere.color = color;
	sphere.radius = radius;
	sphere.specular = specular;
	sphere.reflect = reflect;
	sphere.refract = refract;

	return sphere;
}

rt_light SceneManager::create_light(lightType type, float intensity, float4 position, float4 direction)
{
	rt_light light = {};

	light.type = type;
	light.intensity = intensity;
	light.position = position;
	light.direction = direction;

	return light;
}

rt_scene SceneManager::create_scene(int width, int height, int spheresCount, int lightCount)
{
	auto min = width > height ? height : width;

	/*std::vector<rt_sphere> spheres;
	std::vector<rt_light> lights;*/

	//spheres.push_back(create_spheres({ 2,0,4 }, { 0,1,0 }, 1, 10, 0.2f));
	//spheres.push_back(create_spheres({ -2,0,4 }, { 0,0,1 }, 1, 500, 0.3f));
	//spheres.push_back(create_spheres({ 0,-1,3 }, { 1,0,0 }, 1, 500, 0.4f));
	//spheres.push_back(create_spheres({ 0,-5001,3 }, { 1,1,0 }, 5000, 50, 0.2f));
	////for (cl_float x = 0.2; x < 8; x++)
	////for (cl_float y = 0; y < 2; y++)
	////for (cl_float z = 0; z < 3; z++)
	////{
	////	cl_float r = (cl_float)rand() / (cl_float)RAND_MAX;
	////	cl_float g = (cl_float)rand() / (cl_float)RAND_MAX;
	////	cl_float b = (cl_float)rand() / (cl_float)RAND_MAX;
	////	cl_float reflect = (cl_float)rand() / (cl_float)RAND_MAX;
	////	cl_int specular = rand() % 500;
	////	//reflect = 0.02;

	////	spheres.push_back(create_spheres({ x - 3, y, z - 2.5f }, { r,g,b }, 0.4f, specular, reflect));
	////}

	//lights.push_back(create_light(ambient, 0.2f, { 0 }, { 0 }));
	//lights.push_back(create_light(point, 0.6f, { 2,1,0 }, { 0 }));
	//lights.push_back(create_light(direct, 0.2f, { 0 }, { 1,4,4 }));

	//spheres.push_back(create_spheres({ 2,1,0 }, { 1,1,1 }, 0.2, 0, 0));

	rt_scene scene = {};

	scene.camera_pos = {};
	scene.canvas_height = height;
	scene.canvas_width = width;
	scene.viewport_dist = 1;
	scene.viewport_height = height / static_cast<float>(min);
	scene.viewport_width = width / static_cast<float>(min);
	scene.bg_color = { 0,0.2,0.7 };
	scene.reflect_depth = 3;

	scene.sphere_count = spheresCount;
	scene.light_count = lightCount;

	return scene;
}

void SceneManager::initBuffers()
{
	spheres.push_back(create_spheres({ 2,0,4 }, { 0,1,0 }, 1, 10, 0.2f, 0));
	spheres.push_back(create_spheres({ -2,0,4 }, { 0,0,1 }, 1, 500, 0.3f, 0));
	spheres.push_back(create_spheres({ 0,-1,3 }, { 1,0,0 }, 1, 500, 0.4f, 0));
	spheres.push_back(create_spheres({ 0,-5001,3 }, { 1,1,0 }, 5000, 50, 0.2f, 0));
	spheres.push_back(create_spheres({ 0,0.8,1 }, { 0,0,0 }, 0.5, 500, 0.4f, 1.5));

	lights.push_back(create_light(ambient, 0.2f, { 0 }, { 0 }));
	lights.push_back(create_light(point, 0.6f, { 2,1,0 }, { 0 }));
	lights.push_back(create_light(direct, 0.2f, { 0 }, { 1,4,4 }));

	spheres.push_back(create_spheres({ 0,0.5,0 }, getColor(66, 247, 136), 0.5, 50, 0.2f, 1.7));
	rotating_primitives.push_back({ &spheres.back(), sphere, 9, 4, 0, 1 });

	scene = create_scene(wind_width, wind_height, spheres.size(), lights.size());

	glGenBuffers(1, &sceneSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sceneSsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_scene), NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sceneSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &sphereSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_sphere) * spheres.size(), spheres.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sphereSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &lightSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_light) * lights.size(), lights.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lightSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SceneManager::updateBuffers() const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sceneSsbo);
	GLvoid* scene_p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	memcpy(scene_p, &scene, sizeof(rt_scene));
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSsbo);
	GLvoid* spheres_p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	memcpy(spheres_p, spheres.data(), sizeof(rt_sphere) * spheres.size());
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	//GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	//auto spheres = (rt_sphere*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(arr), bufMask);
	//for (int i = 0; i < 2; ++i)
	//{
	//    colors[i].x[0] = 0.3;
	//    colors[i].x[1] = 0.5;
	//    colors[i].x[2] = 0.7;
	//    colors[i].x[3] = 1;
	//    colors[i].r2 = 0.1;
	//}
	////auto c2 = colors + 1;
	////memcpy(colors, arr, sizeof(arr));
	//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

float4 SceneManager::getColor(float r, float g, float b)
{
	return float4{ r / 255, g / 255, b / 255 };
}

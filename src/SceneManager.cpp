#include "SceneManager.h"
#include <GLFW/glfw3.h>
#include <glm/common.hpp>

#define PI_F 3.14159265358979f

SceneManager::SceneManager(int wind_width, int wind_height, scene_container* scene, GLWrapper* wrapper)
{
	this->wind_width = wind_width;
	this->wind_height = wind_height;
	this->wrapper = wrapper;
	this->scene = scene;
	this->position = scene->scene.camera_pos;
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

void SceneManager::UpdateScene(float frameRate)
{
	front.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);
	right = glm::normalize(glm::cross(-front, world_up));
	scene->scene.quat_camera_rotation = glm::quat(glm::vec3(glm::radians(-pitch), glm::radians(yaw), 0));

	auto speed = frameRate * 3;
	if (shift_pressed)
		speed *= 3;
	if (alt_pressed)
		speed /= 6;

	const glm::vec3 aq = front * speed*1e3f;

	if (w_pressed)
		position += front * speed;
	if (a_pressed)
		position -= right * speed;
	if (s_pressed)
		position -= front * speed;
	if (d_pressed)
		position += right * speed;
	if (space_pressed)
		position += world_up * speed;
	if (ctrl_pressed)
		position -= world_up * speed;

	scene->scene.camera_pos = position;
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

rt_material SceneManager::create_material(glm::vec3 color, int specular, float reflect, float refract, glm::vec3 absorb , float diffuse, float kd, float ks)
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

rt_sphere SceneManager::create_sphere(glm::vec3 center, float radius, rt_material material, bool hollow)
{
	rt_sphere sphere = {};
	sphere.obj = glm::vec4(center, radius);
	sphere.hollow = hollow;
	sphere.material = material;

	return sphere;
}

rt_plane SceneManager::create_plane(glm::vec3 normal, glm::vec3 pos, rt_material material)
{
    rt_plane plane = {};
    plane.normal = normal;
    plane.pos = pos;
    plane.material = material;
    return plane;
}

rt_box SceneManager::create_box(glm::vec3 pos, glm::vec3 form, rt_material material)
{
	rt_box box = {};
	box.form = form;
	box.pos = pos;
	box.mat = material;
    return box;
}

rt_torus SceneManager::create_torus(glm::vec3 pos, glm::vec2 form, rt_material material)
{
	rt_torus torus = {};
	torus.form = form;
	torus.pos = pos;
	torus.mat = material;
    return torus;
}

rt_ring SceneManager::create_ring(glm::vec3 pos, float r1, float r2, rt_material material)
{
	rt_ring ring = {};
	ring.pos = pos;
	ring.mat = material;
	ring.r1 = r1 * r1;
	ring.r2 = r2 * r2;
    return ring;
}

rt_light_point SceneManager::create_light_point(glm::vec4 position, glm::vec3 color, float intensity, float linear_k,
	float quadratic_k)
{
	rt_light_point light = {};

	light.intensity = intensity;
	light.pos = position;
	light.color = color;
	light.linear_k = linear_k;
	light.quadratic_k = quadratic_k;

	return light;
}

rt_light_direct SceneManager::create_light_direct(glm::vec3 direction, glm::vec3 color, float intensity)
{
	rt_light_direct light = {};

	light.intensity = intensity;
	light.direction = direction;
	light.color = color;

	return light;
}

rt_scene SceneManager::create_scene(int width, int height)
{
	rt_scene scene = {};

	scene.camera_pos = {};
	scene.canvas_height = height;
	scene.canvas_width = width;
	scene.bg_color = { 0,0.0,0.0 };
	scene.reflect_depth = 5;

	return scene;
}

void SceneManager::initBuffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data)
{
	glGenBuffers(1, ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, *ubo);
	glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
	GLuint blockIndex = glGetUniformBlockIndex(wrapper->getProgramId(), name);
	if (blockIndex == 0xffffffff)
	{
		fprintf(stderr, "Invalid ubo block name '%s'", name);
		exit(1);
	}
	glUniformBlockBinding(wrapper->getProgramId(), blockIndex, bindingPoint);
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, *ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

template<typename T>
void SceneManager::initBuffer(GLuint *ubo, const char* name, int bindingPoint, std::vector<T>& data)
{
	initBuffer(ubo, name, bindingPoint, sizeof(T) * data.size(), data.data());
}

void SceneManager::initBuffers()
{
	initBuffer(&sceneUbo, "scene_buf", 0, sizeof(rt_scene), nullptr);
	initBuffer(&sphereUbo, "spheres_buf", 1, scene->spheres);
	initBuffer(&planeUbo, "planes_buf", 2, scene->planes);
	initBuffer(&surfaceUbo, "surfaces_buf", 3, scene->surfaces);
	initBuffer(&boxUbo, "boxes_buf", 4, scene->boxes);
	initBuffer(&torusUbo, "toruses_buf", 5, scene->toruses);
	initBuffer(&ringUbo, "rings_buf", 6, scene->rings);
	initBuffer(&lightPointUbo, "lights_point_buf", 7, scene->lights_point);
	initBuffer(&lightDirectUbo, "lights_direct_buf", 8, scene->lights_direct);
}

template<typename T>
void updateBuffer(GLuint ubo, std::vector<T>& data)
{
	if (!data.empty())
	{
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
		memcpy(p, data.data(), sizeof(T) * data.size());
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	}
}

void SceneManager::updateBuffers() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, sceneUbo);
	GLvoid* scene_p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(scene_p, &scene->scene, sizeof(rt_scene));
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	updateBuffer(sphereUbo, scene->spheres);
	updateBuffer(planeUbo, scene->planes);
	updateBuffer(surfaceUbo, scene->surfaces);
	updateBuffer(boxUbo, scene->boxes);
	updateBuffer(torusUbo, scene->toruses);
	updateBuffer(ringUbo, scene->rings);
	updateBuffer(lightPointUbo, scene->lights_point);
}

glm::vec3 SceneManager::getColor(float r, float g, float b)
{
	return glm::vec3(r / 255, g / 255, b / 255);
}

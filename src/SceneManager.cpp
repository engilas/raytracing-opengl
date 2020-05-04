#include "SceneManager.h"
#include "quaternion.h"
#include <GLFW/glfw3.h>

#define PI_F 3.14159265358979f

SceneManager::SceneManager(int wind_width, int wind_height, scene_container* scene, GLWrapper* wrapper)
{
	this->wind_width = wind_width;
	this->wind_height = wind_height;
	this->wrapper = wrapper;
	this->scene = scene;
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

void SceneManager::UpdateScene(float frameRate)
{
	const float xAxis[3] = { 1, 0, 0 };
	const float yAxis[3] = { 0, 1, 0 };
	const float zAxis[3] = { 0, 0, 1 };
	
	Quaternion<float> qX(xAxis, -pitch * PI_F / 180.0f);
	Quaternion<float> qY(yAxis, yaw * PI_F / 180.0f);
	Quaternion<float> q = qY * qX;
	scene->scene.quat_camera_rotation = q.GetStruct();


	auto speed = frameRate * 3;
	if (shift_pressed)
		speed *= 3;
	if (alt_pressed)
		speed /= 6;

	if (w_pressed)
		moveCamera(q, zAxis, scene->scene.camera_pos, speed);
	if (a_pressed)
		moveCamera(q, xAxis, scene->scene.camera_pos, -speed);
	if (s_pressed)
		moveCamera(q, zAxis, scene->scene.camera_pos, -speed);
	if (d_pressed)
		moveCamera(q, xAxis, scene->scene.camera_pos, speed);

	if (space_pressed)
		moveCamera(yAxis, scene->scene.camera_pos, speed);
	if (ctrl_pressed)
		moveCamera(yAxis, scene->scene.camera_pos, -speed);
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

rt_material SceneManager::create_material(vec3 color, int specular, float reflect, float refract, vec3 absorb , float diffuse, float kd, float ks)
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

rt_sphere SceneManager::create_sphere(vec3 center, float radius, rt_material material, bool hollow)
{
	rt_sphere sphere = {};
	sphere.obj = { center.x, center.y, center.z, radius };
	sphere.hollow = hollow;

	// sphere.pos = center;
	// sphere.radius = radius;
	//
	sphere.material = material;

	return sphere;
}

rt_plane SceneManager::create_plane(vec3 normal, vec3 pos, rt_material material)
{
    rt_plane plane = {};
    plane.normal = normal;
    plane.pos = pos;
    plane.material = material;
    return plane;
}

rt_box SceneManager::create_box(vec3 pos, vec3 form, rt_material material)
{
	rt_box box = {};
	box.form = form;
	box.pos = pos;
	box.mat = material;
    return box;
}

rt_torus SceneManager::create_torus(vec3 pos, vec2 form, rt_material material)
{
	rt_torus torus = {};
	torus.form = form;
	torus.pos = pos;
	torus.mat = material;
    return torus;
}

rt_light_point SceneManager::create_light_point(vec4 position, vec3 color, float intensity)
{
	rt_light_point light = {};

	light.intensity = intensity;
	light.pos = position;
	light.color = color;

	return light;
}

rt_light_direct SceneManager::create_light_direct(vec3 direction, vec3 color, float intensity)
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

void SceneManager::initBuffers()
{
	glGenBuffers(1, &sceneUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, sceneUbo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_scene), NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, sceneUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &sphereUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, sphereUbo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_sphere) * scene->spheres.size(), scene->spheres.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, sphereUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &planeUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, planeUbo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_plane) * scene->planes.size(), scene->planes.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, planeUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &surfaceUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, surfaceUbo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_surface) * scene->surfaces.size(), scene->surfaces.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 5, surfaceUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &boxUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, boxUbo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_box) * scene->boxes.size(), scene->boxes.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 6, boxUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &torusUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, torusUbo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_torus) * scene->toruses.size(), scene->toruses.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 7, torusUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &lightPointUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, lightPointUbo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_light_point) * scene->lights_point.size(), scene->lights_point.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 3, lightPointUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &lightDirectUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, lightDirectUbo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(rt_light_direct) * scene->lights_direct.size(), scene->lights_direct.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 4, lightDirectUbo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SceneManager::updateBuffers() const
{
	glBindBuffer(GL_UNIFORM_BUFFER, sceneUbo);
	GLvoid* scene_p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	memcpy(scene_p, scene, sizeof(rt_scene));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

    if (!scene->spheres.empty())
    {
        glBindBuffer(GL_UNIFORM_BUFFER, sphereUbo);
	    GLvoid* spheres_p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	    memcpy(spheres_p, scene->spheres.data(), sizeof(rt_sphere) * scene->spheres.size());
	    glUnmapBuffer(GL_UNIFORM_BUFFER);
    }
    
    if (!scene->planes.empty())
    {
        glBindBuffer(GL_UNIFORM_BUFFER, planeUbo);
	    GLvoid* planes_p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	    memcpy(planes_p, scene->planes.data(), sizeof(rt_plane) * scene->planes.size());
	    glUnmapBuffer(GL_UNIFORM_BUFFER);
    }
	if (!scene->surfaces.empty())
    {
        glBindBuffer(GL_UNIFORM_BUFFER, surfaceUbo);
	    GLvoid* surfaces_p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	    memcpy(surfaces_p, scene->surfaces.data(), sizeof(rt_surface) * scene->surfaces.size());
	    glUnmapBuffer(GL_UNIFORM_BUFFER);
    }
	if (!scene->boxes.empty())
    {
        glBindBuffer(GL_UNIFORM_BUFFER, boxUbo);
	    GLvoid* boxes_p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	    memcpy(boxes_p, scene->boxes.data(), sizeof(rt_box) * scene->boxes.size());
	    glUnmapBuffer(GL_UNIFORM_BUFFER);
    }
	if (!scene->toruses.empty())
    {
        glBindBuffer(GL_UNIFORM_BUFFER, torusUbo);
	    GLvoid* toruses_p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	    memcpy(toruses_p, scene->toruses.data(), sizeof(rt_torus) * scene->toruses.size());
	    glUnmapBuffer(GL_UNIFORM_BUFFER);
    }
	if (!scene->lights_point.empty())
	{
		glBindBuffer(GL_UNIFORM_BUFFER, lightPointUbo);
	    GLvoid* ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	    memcpy(ptr, scene->lights_point.data(), sizeof(rt_light_point) * scene->lights_point.size());
	    glUnmapBuffer(GL_UNIFORM_BUFFER);
	}
}

vec3 SceneManager::getColor(float r, float g, float b)
{
	return vec3{ r / 255, g / 255, b / 255 };
}

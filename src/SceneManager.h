#pragma once

#include "GLWrapper.h"
#include "scene.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class SceneManager 
{
public:
	SceneManager(int wind_width, int wind_height, scene_container* scene, GLWrapper* wrapper);

	void init();
	void update(float frameRate);

	static rt_material create_material(glm::vec3 color, int specular, float reflect, float refract = 0.0, glm::vec3 absorb = {}, float diffuse = 0.7, float kd = 0.8, float ks = 0.2);
	static rt_sphere create_sphere(glm::vec3 center, float radius, rt_material material, bool hollow = false);
	static rt_plane create_plane(glm::vec3 normal, glm::vec3 pos, rt_material material);
	static rt_box create_box(glm::vec3 pos, glm::vec3 form, rt_material material);
	static rt_torus create_torus(glm::vec3 pos, glm::vec2 form, rt_material material);
	static rt_ring create_ring(glm::vec3 pos, float r1, float r2, rt_material material);
	static rt_light_point create_light_point(glm::vec4 position, glm::vec3 color, float intensity, float linear_k = 0.22f, float quadratic_k = 0.2f);
	static rt_light_direct create_light_direct(glm::vec3 direction, glm::vec3 color, float intensity);
	static rt_scene create_scene(int width, int height);

private:
	scene_container* scene;

	int wind_width;
	int wind_height;
	GLWrapper* wrapper;

	bool w_pressed = false;
	bool a_pressed = false;
	bool s_pressed = false;
	bool d_pressed = false;
	bool ctrl_pressed = false;
	bool shift_pressed = false;
	bool space_pressed = false;
	bool alt_pressed = false;

	float lastX = 0;
	float lastY = 0;
	
	// Camera Attributes
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 world_up = glm::vec3(0, 1, 0);
	// Euler Angles
	float yaw = 0;
	float pitch = 0;

	GLuint sceneUbo = 0;
	GLuint sphereUbo = 0;
    GLuint planeUbo = 0;
    GLuint surfaceUbo = 0;
    GLuint boxUbo = 0;
    GLuint torusUbo = 0;
    GLuint ringUbo = 0;
	GLuint lightPointUbo = 0;
	GLuint lightDirectUbo = 0;

	void multiplyVector(float v[3], float s);
	void addVector(glm::vec3& v1, const float v2[3]);
	void moveCamera(glm::quat& q, const float direction[3], glm::vec3 & vector, float speed);
	void moveCamera(const float direction[3], glm::vec3 & vector, float speed);
	void UpdateScene(float frameRate);
	void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods);
	static void glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height);
	void glfw_mouse_callback(GLFWwindow * window, double xpos, double ypos);
	void initBuffers();
	void updateBuffers() const;
	glm::vec3 getColor(float r, float g, float b);
	
	template<typename T>
	void initBuffer(GLuint* ubo, const char* name, int bindingPoint, std::vector<T>& data);
	void initBuffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data);
};

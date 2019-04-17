#pragma once

#include "GLWrapper.h"
#include "scene.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "quaternion.h"

class SceneManager 
{
public:
	SceneManager(int wind_width, int wind_height, scene_container &scene, GLWrapper* wrapper);

	void init();
	void update(float frameRate);

	static rt_material create_material(vec3 color, int specular, float reflect, float refract = 0.0, vec3 absorb = {}, float diffuse = 0.7, float kd = 0.8, float ks = 0.2);
	static rt_sphere create_sphere(vec3 center, float radius, rt_material material);
	static rt_plain create_plain(vec3 normal, vec3 pos, rt_material material);
	static rt_light_point create_light_point(vec4 position, vec3 color, float intensity);
	static rt_light_direct create_light_direct(vec3 direction, vec3 color, float intensity);
	static rt_scene create_scene(int width, int height);

private:
	scene_container scene;

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
	float pitch = 0;
	float yaw = 0;

	GLuint sceneUbo = 0;
	GLuint sphereUbo = 0;
    GLuint plainUbo = 0;
	GLuint lightPointUbo = 0;
	GLuint lightDirectUbo = 0;

	void multiplyVector(float v[3], float s);
	void addVector(vec3 & v1, const float v2[3]);
	void moveCamera(Quaternion<float>& q, const float direction[3], vec3 & vector, float speed);
	void moveCamera(const float direction[3], vec3 & vector, float speed);
	void ProcessRotations(float frameRate);
	void UpdateScene(float frameRate);
	void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods);
	static void glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height);
	void glfw_mouse_callback(GLFWwindow * window, double xpos, double ypos);
	void initBuffers();
	void updateBuffers() const;
	vec3 getColor(float r, float g, float b);
};

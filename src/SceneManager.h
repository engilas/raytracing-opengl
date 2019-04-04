#pragma once

#include "GLWrapper.h"
#include "scene.h"
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "quaternion.h"

class SceneManager 
{
public:
	SceneManager(int wind_width, int wind_height, GLWrapper* wrapper);
	rt_scene& getScene();

	void init();
	void update(float frameRate);

private:
	std::vector<rt_sphere> spheres;
	std::vector<rt_light> lights;
    std::vector<rt_plain> plains;
	std::vector<rotating_primitive> rotating_primitives;
	rt_scene scene;

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

	GLuint sceneSsbo = 0;
	GLuint sphereSsbo = 0;
    GLuint plainSsbo = 0;
	GLuint lightSsbo = 0;

	void multiplyVector(float v[3], float s);
	void addVector(vec3 & v1, const float v2[3]);
	void moveCamera(Quaternion<float>& q, const float direction[3], vec3 & vector, float speed);
	void moveCamera(const float direction[3], vec3 & vector, float speed);
	void ProcessRotations(float frameRate);
	void UpdateScene(float frameRate);
	void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods);
	static void glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height);
	void glfw_mouse_callback(GLFWwindow * window, double xpos, double ypos);
    rt_material create_material(vec3 color, int specular, float reflect, float refract, vec3 absorb, float diffuse, float kd, float ks);
    rt_sphere create_sphere(vec3 center, float radius, rt_material material);
    rt_plain create_plain(vec3 normal,vec3 pos,rt_material material);
    rt_light create_light(lightType type, float intensity, vec3 color, vec3 position, vec3 direction);
	rt_scene create_scene(int width, int height, int spheresCount, int lightCount, int plainCount);
	void initBuffers();
	void updateBuffers() const;
	vec3 getColor(float r, float g, float b);
};

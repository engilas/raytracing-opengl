#include "scene.h"
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "quaternion.h"

class SceneManager 
{
public:
	SceneManager(int wind_width, int wind_height, GLFWwindow * window);
	rt_scene& getScene();

	void init();
	void update(float frameRate);

private:
	std::vector<rt_sphere> spheres;
	std::vector<rt_light> lights;
	std::vector<rotating_primitive> rotating_primitives;
	rt_scene scene;

	int wind_width;
	int wind_height;
	GLFWwindow * window;

	bool w_pressed = false;
	bool a_pressed = false;
	bool s_pressed = false;
	bool d_pressed = false;
	bool ctrl_pressed = false;
	bool shift_pressed = false;
	bool space_pressed = false;

	float lastX = 0;
	float lastY = 0;
	float pitch = 0;
	float yaw = 0;

	GLuint sceneSsbo = 0;
	GLuint sphereSsbo = 0;
	GLuint lightSsbo = 0;

	void multiplyVector(float v[3], float s);
	void addVector(float4 & v1, const float v2[3]);
	void moveCamera(Quaternion<float>& q, const float direction[3], float4 & vector, float speed);
	void moveCamera(const float direction[3], float4 & vector, float speed);
	void ProcessRotations(float frameRate);
	void UpdateScene(float frameRate);
	void glfw_key_callback(GLFWwindow * window, int key, int scancode, int action, int mods);
	static void glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height);
	void glfw_mouse_callback(GLFWwindow * window, double xpos, double ypos);
	rt_sphere create_spheres(float4 center, float4 color, float radius, int specular, float reflect, float refract);
	rt_light create_light(lightType type, float intensity, float4 position, float4 direction);
	rt_scene create_scene(int width, int height, int spheresCount, int lightCount);
	void initBuffers();
	void updateBuffers() const;
	float4 getColor(float r, float g, float b);
};

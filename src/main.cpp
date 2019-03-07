#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <cstdlib>
#include <chrono>
#include "GLWrapper.h"

#include "scene.h"
#include "quaternion.h"
#include <vector>

static int wind_width = 1024;
static int wind_height= 1024;

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

std::vector<rt_sphere> spheres;
std::vector<rt_light> lights;
rt_scene scene;

void multiplyVector(float v[3], float s) {
	v[0] *= s;
	v[1] *= s;
	v[2] *= s;
}

void addVector(float4 &v1, const float v2[3]) {
	v1.x += v2[0];
	v1.y += v2[1];
	v1.z += v2[2];
}

void moveCamera(Quaternion<float> &q, const float direction[3], float4 &vector, float speed) {
    float tmp[3] = {direction[0], direction[1], direction[2]};
	q.QuatRotation(tmp);
	multiplyVector(tmp, speed);
	addVector(vector, tmp);
}

void moveCamera(const float direction[3], float4 &vector, float speed) {
	float tmp[3] = {direction[0], direction[1], direction[2]};
	multiplyVector(tmp, speed);
	addVector(vector, tmp);
}

void UpdateScene(rt_scene &scene, float frameRate) 
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
}

static void glfw_key_callback(GLFWwindow* wind, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(wind, GL_TRUE);

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

bool firstMouse = true;

static void glfw_mouse_callback(GLFWwindow* window, double xpos, double ypos) {

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

rt_sphere create_spheres(float4 center, float4 color, float radius, int specular, float reflect)
{
	rt_sphere sphere = {};

	sphere.center = center;
	sphere.color = color;
	sphere.radius = radius;
	sphere.specular = specular;
	sphere.reflect = reflect;

	return sphere;
}

rt_light create_light(lightType type, float intensity, float4 position, float4 direction)
{
	rt_light light = {};

	light.type = type;
	light.intensity = intensity;
	light.position = position;
	light.direction = direction;

	return light;
}

rt_scene create_scene(int width, int height, int spheresCount, int lightCount)
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
	scene.bg_color = {0,0,0};
	scene.reflect_depth = 3;

    scene.sphere_count = spheresCount;
	scene.light_count = lightCount;

	//std::copy(spheres.begin(), spheres.end(), scene.spheres);
	//std::copy(lights.begin(), lights.end(), scene.lights);

    return scene;
}

static void glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height)
{
    glViewport(0,0,width,height);
}

void initBuffers()
{
    

    spheres.push_back(create_spheres({ 2,0,4 }, { 0,1,0 }, 1, 10, 0.2f));
	spheres.push_back(create_spheres({ -2,0,4 }, { 0,0,1 }, 1, 500, 0.3f));
	spheres.push_back(create_spheres({ 0,-1,3 }, { 1,0,0 }, 1, 500, 0.4f));
	spheres.push_back(create_spheres({ 0,-5001,3 }, { 1,1,0 }, 5000, 50, 0.2f));

    lights.push_back(create_light(ambient, 0.2f, { 0 }, { 0 }));
	lights.push_back(create_light(point, 0.6f, { 2,1,0 }, { 0 }));
	lights.push_back(create_light(direct, 0.2f, { 0 }, { 1,4,4 }));

    scene = create_scene(wind_width, wind_height, spheres.size(), lights.size());

	glGenBuffers(1, &sceneSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sceneSsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_scene) , NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sceneSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &sphereSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_sphere) * spheres.size() , spheres.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sphereSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &lightSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_light) * lights.size() , lights.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lightSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


    //GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    //auto colors = (color*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(arr), bufMask);
    //for (int i = 0; i < 2; ++i)
    //{
    //    colors[i].x[0] = 0.3;
	   // colors[i].x[1] = 0.5;
	   // colors[i].x[2] = 0.7;
	   // colors[i].x[3] = 1;
    //    colors[i].r2 = 0.1;
    //}
    ////auto c2 = colors + 1;
    ////memcpy(colors, arr, sizeof(arr));
    //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    
}

void updateBuffers()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sceneSsbo);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &scene, sizeof(rt_scene));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

int main()
{
#define FULLSCREEN

#ifdef FULLSCREEN
    GLWrapper glWrapper(true);
#else
    GLWrapper glWrapper(wind_width, wind_height, false);
#endif

	glWrapper.init();

    glfwSetInputMode(glWrapper.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(glWrapper.window, glfw_mouse_callback);
    glfwSetKeyCallback(glWrapper.window,glfw_key_callback);
    glfwSetFramebufferSizeCallback(glWrapper.window,glfw_framebuffer_size_callback);

	wind_width = glWrapper.getWidth();
	wind_height = glWrapper.getHeight();
    initBuffers();

	glfwSwapInterval(0);

    const auto start = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    int frames_count = 0;

    while (!glfwWindowShouldClose(glWrapper.window))
    {
        ++frames_count;
        auto newTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> frameTime = (newTime - currentTime);
		currentTime = newTime;

        UpdateScene(scene, frameTime.count());
        updateBuffers();
        glWrapper.draw();
		glfwSwapBuffers(glWrapper.window);
        glfwPollEvents();
    }

	glWrapper.stop(); // stop glfw, close window
	return 0;
}
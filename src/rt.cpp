#include <glad/glad.h>

//#include "OpenCLUtil.h"
//#define __CL_ENABLE_EXCEPTIONS
//#include "cl.hpp"

//#ifdef OS_WIN
//#define GLFW_EXPOSE_NATIVE_WIN32
//#define GLFW_EXPOSE_NATIVE_WGL
//#endif
//
//#ifdef OS_LNX
//#define GLFW_EXPOSE_NATIVE_X11
//#define GLFW_EXPOSE_NATIVE_GLX
//#endif

#include <GLFW/glfw3.h>

#include "OpenGLUtil.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include "GLWrapper.h"

#include "scene.h"
#include "quaternion.h"

using namespace std;
//using namespace cl;

typedef unsigned int uint;

static const float matrix[16] =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

static const float vertices[12] =
{
    -1.0f,-1.0f, 0.0,
     1.0f,-1.0f, 0.0,
     1.0f, 1.0f, 0.0,
    -1.0f, 1.0f, 0.0
};

static const float texcords[8] =
{
    0.0, 1.0,
    1.0, 1.0,
    1.0, 0.0,
    0.0, 0.0
};

GLuint genTexture();
GLuint genRenderProg(GLuint texHandle);
GLuint genComputeProg(GLuint texHandle);

static const uint indices[6] = {0,1,2,0,2,3};

static int wind_width = 720;
static int wind_height= 720;

//typedef struct {
//    Device d;
//    CommandQueue q;
//    Program p;
//    Kernel k;
//    ImageGL tex;
//
//	rt_scene scene;
//	
//	Buffer sceneMem;
//} process_params;

typedef struct {
    GLuint texPrg;
    //GLuint vao;
    GLuint tex;
	GLuint computePrg;

} render_params;

//process_params params;
render_params rparams;

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

//rt_sphere create_spheres(cl_float4 center, cl_float4 color, cl_float radius, cl_int specular, cl_float reflect)
//{
//	rt_sphere sphere;
//	memset(&sphere, 0, sizeof(rt_sphere));
//	sphere.center = center;
//	sphere.color = color;
//	sphere.radius = radius;
//	sphere.specular = specular;
//	sphere.reflect = reflect;
//
//	return sphere;
//}
//
//rt_light create_light(lightType type, cl_float intensity, cl_float4 position, cl_float4 direction)
//{
//	rt_light light;
//	memset(&light, 0, sizeof(rt_light));
//	light.type = type;
//	light.intensity = intensity;
//	light.position = position;
//	light.direction = direction;
//
//	return light;
//}
//
//
//
//rt_scene create_scene(int width, int height)
//{
//	auto min = width > height ? height : width;
//
//	int spheres_count = 3;
//	std::vector<rt_sphere> spheres;
//	std::vector<rt_light> lights;
//
//	spheres.push_back(create_spheres({ 2,0,4 }, { 0,1,0 }, 1, 10, 0.2f));
//	spheres.push_back(create_spheres({ -2,0,4 }, { 0,0,1 }, 1, 500, 0.3f));
//	spheres.push_back(create_spheres({ 0,-1,3 }, { 1,0,0 }, 1, 500, 0.4f));
//	spheres.push_back(create_spheres({ 0,-5001,3 }, { 1,1,0 }, 5000, 50, 0.2f));
//	//for (cl_float x = 0.2; x < 8; x++)
//	//for (cl_float y = 0; y < 2; y++)
//	//for (cl_float z = 0; z < 3; z++)
//	//{
//	//	cl_float r = (cl_float)rand() / (cl_float)RAND_MAX;
//	//	cl_float g = (cl_float)rand() / (cl_float)RAND_MAX;
//	//	cl_float b = (cl_float)rand() / (cl_float)RAND_MAX;
//	//	cl_float reflect = (cl_float)rand() / (cl_float)RAND_MAX;
//	//	cl_int specular = rand() % 500;
//	//	//reflect = 0.02;
//
//	//	spheres.push_back(create_spheres({ x - 3, y, z - 2.5f }, { r,g,b }, 0.4f, specular, reflect));
//	//}
//
//	lights.push_back(create_light(Ambient, 0.2f, { 0 }, { 0 }));
//	lights.push_back(create_light(Point, 0.6f, { 2,1,0 }, { 0 }));
//	lights.push_back(create_light(Direct, 0.2f, { 0 }, { 1,4,4 }));
//
//	//spheres.push_back(create_spheres({ 2,1,0 }, { 1,1,1 }, 0.2, 0, 0));
//
//	rt_scene scene;
//    memset(&scene, 0, sizeof(rt_scene));
//	scene.camera_pos = { 0 };
//    scene.canvas_height = height;
//    scene.canvas_width = width;
//    scene.viewport_dist = 1;
//    scene.viewport_height = height / (cl_float) min;
//    scene.viewport_width = width / (cl_float) min;
//	scene.bg_color = { 0 };
//	scene.reflect_depth = 3;
//
//    scene.sphere_count = spheres.size();
//	scene.light_count = lights.size();
//
//	std::copy(spheres.begin(), spheres.end(), scene.spheres);
//	std::copy(lights.begin(), lights.end(), scene.lights);
//
//    return scene;
//}
//
//void multiplyVector(cl_float v[3], cl_float s) {
//	v[0] *= s;
//	v[1] *= s;
//	v[2] *= s;
//}
//
//void addVector(cl_float4 *v1, const cl_float v2[3]) {
//	v1->x += v2[0];
//	v1->y += v2[1];
//	v1->z += v2[2];
//}

//void moveCamera(Quaternion<cl_float> &q, const cl_float direction[3], cl_float4 *vector, const cl_float speed) {
//	cl_float tmp[3] = { direction[0], direction[1], direction[2] };
//	q.QuatRotation(tmp);
//	multiplyVector(tmp, speed);
//	addVector(vector, tmp);
//}
//
//void moveCamera(const cl_float direction[3], cl_float4 *vector, const cl_float speed) {
//	cl_float tmp[3] = { direction[0], direction[1], direction[2] };
//	multiplyVector(tmp, speed);
//	addVector(vector, tmp);
//}
//
//void UpdateScene(rt_scene &scene, double frameRate) 
//{
//	const cl_float xAxis[3] = { 1, 0, 0 };
//	const cl_float yAxis[3] = { 0, 1, 0 };
//	const cl_float zAxis[3] = { 0, 0, 1 };
//	const float PI_F = 3.14159265358979f;
//
//	Quaternion<cl_float> qX(xAxis, -pitch * PI_F / 180.0f);
//	Quaternion<cl_float> qY(yAxis, yaw * PI_F / 180.0f);
//	Quaternion<cl_float> q = qY * qX;
//	//params.scene.camera_rotation = q.GetStruct();
//
//
//	auto speed = (cl_float)frameRate;
//	if (shift_pressed)
//		speed *= 3;
//
//	/*if (w_pressed) 
//		moveCamera(q, zAxis, &params.scene.camera_pos, speed);
//	if (a_pressed)
//		moveCamera(q, xAxis, &params.scene.camera_pos, -speed);
//	if (s_pressed)
//		moveCamera(q, zAxis, &params.scene.camera_pos, -speed);
//	if (d_pressed)
//		moveCamera(q, xAxis, &params.scene.camera_pos, speed);
//	
//	if (space_pressed)
//		moveCamera(yAxis, &params.scene.camera_pos, speed);
//	if (ctrl_pressed)
//		moveCamera(yAxis, &params.scene.camera_pos, -speed);*/
//}

static void glfw_error_callback(int error, const char* desc)
{
    fputs(desc,stderr);
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

static void glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height)
{
    glViewport(0,0,width,height);
}

void processTimeStep(double frameRate);
void renderFrame(void);
void updateTex(int w, int h);
void draw();
void checkErrors(std::string desc);

GLuint renderHandle;
GLuint computeHandle;



int main()
{
	int w = 1024, h = 1024;

	GLWrapper glWrapper(w, h, true);
	glWrapper.init();
	w = glWrapper.getWidth();
	h = glWrapper.getHeight();

	//GLuint quad_vao = glWrapper.create_quad_vao();
	//GLuint quad_program = glWrapper.create_quad_program();

	//GLuint ray_program = glWrapper.create_compute_program();

	//GLuint tex_output = 0;
	//int tex_w = w, tex_h = h;
	//{ // create the texture
	//	glGenTextures(1, &tex_output);
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, tex_output);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//	// linear allows us to scale the window up retaining reasonable quality
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//	// same internal format as compute shader input
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT,
	//		NULL);
	//	// bind to image unit so can write to specific pixels from the shader
	//	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	//}

	//{ // query up the workgroups
	//	int work_grp_size[3], work_grp_inv;
	//	// maximum global work group (total work in a dispatch)
	//	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_size[0]);
	//	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_size[1]);
	//	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_size[2]);
	//	printf("max global (total) work group size x:%i y:%i z:%i\n", work_grp_size[0],
	//		work_grp_size[1], work_grp_size[2]);
	//	// maximum local work group (one shader's slice)
	//	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	//	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	//	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	//	printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
	//		work_grp_size[0], work_grp_size[1], work_grp_size[2]);
	//	// maximum compute shader invocations (x * y * z)
	//	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	//	printf("max computer shader invocations %i\n", work_grp_inv);
	//}

	//glfwSwapInterval(0);

	//while (!glfwWindowShouldClose(glWrapper.window)) { // drawing loop
	//	{																					 // launch compute shaders!
	//		glUseProgram(ray_program);
	//		//glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 1);
	//		glDispatchCompute(w / 16.0, h / 16.0, 1);
	//	}

	//	// prevent sampling befor all writes to image are done
	//	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//	glClear(GL_COLOR_BUFFER_BIT);
	//	glUseProgram(quad_program);
	//	glBindVertexArray(quad_vao);
	//	glActiveTexture(GL_TEXTURE0);
	//	glBindTexture(GL_TEXTURE_2D, tex_output);
	//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	//	glfwPollEvents();
	//	if (GLFW_PRESS == glfwGetKey(glWrapper.window, GLFW_KEY_ESCAPE)) {
	//		glfwSetWindowShouldClose(glWrapper.window, 1);
	//	}
	//	glfwSwapBuffers(glWrapper.window);
	//}

	

    

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	/*glfwSetCursorPosCallback(window, glfw_mouse_callback);
    glfwSetKeyCallback(window,glfw_key_callback);
    glfwSetFramebufferSizeCallback(window,glfw_framebuffer_size_callback);*/

 //   const auto start = std::chrono::steady_clock::now();
 //   int frames_count = 0;
 //   

	glfwSwapInterval(0);


	/*GLuint texHandle = genTexture();
	renderHandle = genRenderProg(texHandle);
	computeHandle = genComputeProg(rparams.tex);*/

    while (!glfwWindowShouldClose(glWrapper.window))
    {
        glWrapper.draw();
		glfwSwapBuffers(glWrapper.window);
        glfwPollEvents();
		if (GLFW_PRESS == glfwGetKey(glWrapper.window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(glWrapper.window, 1);
		}
    }

	glWrapper.stop(); // stop glfw, close window
	return 0;
}
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
#include <vector>

static int wind_width = 1024;
static int wind_height= 1024;

GLuint sceneSsbo = 0;
GLuint sphereSsbo = 0;
GLuint lightSsbo = 0;

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
	scene.bg_color = {};
	scene.reflect_depth = 3;

    scene.sphere_count = spheresCount;
	scene.light_count = lightCount;

	//std::copy(spheres.begin(), spheres.end(), scene.spheres);
	//std::copy(lights.begin(), lights.end(), scene.lights);

    return scene;
}

void initBuffers()
{
    std::vector<rt_sphere> spheres;
	std::vector<rt_light> lights;

    spheres.push_back(create_spheres({ 2,0,4 }, { 0,1,0 }, 1, 10, 0.2f));
	spheres.push_back(create_spheres({ -2,0,4 }, { 0,0,1 }, 1, 500, 0.3f));
	spheres.push_back(create_spheres({ 0,-1,3 }, { 1,0,0 }, 1, 500, 0.4f));
	spheres.push_back(create_spheres({ 0,-5001,3 }, { 1,1,0 }, 5000, 50, 0.2f));

    lights.push_back(create_light(ambient, 0.2f, { 0 }, { 0 }));
	lights.push_back(create_light(point, 0.6f, { 2,1,0 }, { 0 }));
	lights.push_back(create_light(direct, 0.2f, { 0 }, { 1,4,4 }));

    auto scene = create_scene(wind_width, wind_height, spheres.size(), lights.size());

	glGenBuffers(1, &sceneSsbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sceneSsbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rt_scene) , &scene, GL_STATIC_DRAW);
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

int main()
{
	GLWrapper glWrapper(wind_width, wind_height, false);
	glWrapper.init();
	wind_width = glWrapper.getWidth();
	wind_height = glWrapper.getHeight();

	glfwSwapInterval(0);

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
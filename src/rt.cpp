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

static int wind_width = 720;
static int wind_height= 720;

typedef struct color
{
	float r, g, b, a; // colors
};

int main()
{
	int w = 1024, h = 1024;

	GLWrapper glWrapper(w, h, false);
	glWrapper.init();
	w = glWrapper.getWidth();
	h = glWrapper.getHeight();

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
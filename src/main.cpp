#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <chrono>
#include "GLWrapper.h"
#include "SceneManager.h"

static int wind_width = 1680;
static int wind_height = 1050;




int main()
{
//#define FULLSCREEN

#ifdef FULLSCREEN
    GLWrapper glWrapper(true);
#else
    GLWrapper glWrapper(wind_width, wind_height, false);
#endif

	glWrapper.init();
	wind_width = glWrapper.getWidth();
	wind_height = glWrapper.getHeight();

	SceneManager scene_manager(wind_width, wind_height, glWrapper.window);
	scene_manager.init();

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

		scene_manager.update(frameTime.count());
        glWrapper.draw();
		glfwSwapBuffers(glWrapper.window);
        glfwPollEvents();
    }

	glWrapper.stop(); // stop glfw, close window
	return 0;
}
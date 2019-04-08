#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <chrono>
#include "GLWrapper.h"
#include "SceneManager.h"

static int wind_width = 160;
static int wind_height = 160;

int main()
{
#define FULLSCREEN 1

#if FULLSCREEN
    GLWrapper glWrapper(true);
#else
    GLWrapper glWrapper(wind_width, wind_height, false);
#endif

	scene_container scene = {};

	glWrapper.init_window();
	wind_width = glWrapper.getWidth();
	wind_height = glWrapper.getHeight();

	scene.scene = SceneManager::create_scene(wind_width, wind_height);
	scene.spheres.push_back(SceneManager::create_sphere({ 1, 0.25, 1.5 }, 0.3, SceneManager::create_material({ 0,1,0 }, 30, 0.0)));
	scene.lights.push_back(SceneManager::create_light(point, 25.0f, { 1.0,1.0,1.0 }, { 0.8,0,-1.5 }, { 0 }));
	scene.plains.push_back(SceneManager::create_plain({ 0,1,0 }, { 0,-6,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	scene.plains.push_back(SceneManager::create_plain({ 0,-1,0 }, { 0,6,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	scene.plains.push_back(SceneManager::create_plain({ 0,0,-1 }, { 0,0,6 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	scene.plains.push_back(SceneManager::create_plain({ 0,0,1 }, { 0,0,-6 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	scene.plains.push_back(SceneManager::create_plain({ 1,0,0 }, { -6,0,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	scene.plains.push_back(SceneManager::create_plain({ -1,0,0 }, { 6,0,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));

	glWrapper.init_shaders(scene.get_defines());

	SceneManager scene_manager(wind_width, wind_height, scene, &glWrapper);
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

		//glUseProgram(glWrapper.renderHandle);

		scene_manager.update(frameTime.count());
        glWrapper.draw();
		glfwSwapBuffers(glWrapper.window);
        glfwPollEvents();
    }

	glWrapper.stop(); // stop glfw, close window
	return 0;
}
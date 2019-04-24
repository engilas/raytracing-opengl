#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <chrono>
#include "GLWrapper.h"
#include "SceneManager.h"

static int wind_width = 660;
static int wind_height = 960;

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
	//scene.ambient_color = vec3{0.2, 0.2, 0.2};

	scene.lights_point.push_back(SceneManager::create_light_point({-0.8, 0, -1.5, 0.1}, {1,0.8,0}, 2.5));
	scene.lights_point.push_back(SceneManager::create_light_point({0.8, 0.25, -1.5, 0.1}, {0,0.0,1}, 2.5));

	//mirror
	scene.spheres.push_back(SceneManager::create_sphere({0, -0.7, -1.5}, 0.3, SceneManager::create_material({1,1,1}, 200, 1, 0, {}, 1)));
	//transp
	scene.spheres.push_back(SceneManager::create_sphere({0, -0.1, -1.5}, 0.3, SceneManager::create_material({1,1,1}, 200, 0.1, 1.125, {}, 1)));
	//todo transp 2
	// scene.spheres.push_back(SceneManager::create_sphere({1001, 0, 0}, 1000, SceneManager::create_material({1,1,1}, 200, 0.1, 0.8, {}, 1)));
	scene.plains.push_back(SceneManager::create_plain({1,0,0}, {-1,0,0}, SceneManager::create_material({0,1,0}, 200, 0)));
	scene.plains.push_back(SceneManager::create_plain({-1,0,0}, {1,0,0}, SceneManager::create_material({1,0,0}, 200, 0)));
	scene.plains.push_back(SceneManager::create_plain({0,1,0}, {0,-1,0}, SceneManager::create_material({1,1,1}, 200, 0.1)));
	scene.plains.push_back(SceneManager::create_plain({0,-1,0}, {0,1,0}, SceneManager::create_material({1,1,1}, 200, 0.1)));
	scene.plains.push_back(SceneManager::create_plain({0,0,1}, {0,0,-2}, SceneManager::create_material({1,1,1}, 200, 0.1)));




	// scene.spheres.push_back(SceneManager::create_sphere({ 1, 0.25, 3.5 }, 0.3, SceneManager::create_material({ 0,1,0 }, 30, 0.1)));
	// scene.spheres.push_back(SceneManager::create_sphere({ 1, 0.25, -4 }, 0.3, SceneManager::create_material({ 0,1,0 }, 30, 0.05, 1.125, {4,2, 0.5})));
	// scene.lights.push_back(SceneManager::create_light(point, 25.0f, { 1.0,1.0,1.0 }, { 0.8,4,0 }, { 0 }));
	// scene.lights.push_back(SceneManager::create_light(direct, 0.2f, { 1.0,1.0,1.0 }, {}, { 0, -1, 0.2 }));
	// scene.plains.push_back(SceneManager::create_plain({ 0,1,0 }, { 0,-6,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	// //scene.plains.push_back(SceneManager::create_plain({ 0,-1,0 }, { 0,6,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1, 0, {}, 0.7, 0.2)));
	// scene.plains.push_back(SceneManager::create_plain({ 0,0,-1 }, { 0,0,6 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	// scene.plains.push_back(SceneManager::create_plain({ 0,0,1 }, { 0,0,-6 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	// scene.plains.push_back(SceneManager::create_plain({ 1,0,0 }, { -6,0,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	// scene.plains.push_back(SceneManager::create_plain({ -1,0,0 }, { 6,0,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));

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

		scene_manager.update(frameTime.count());
        glWrapper.draw();
		glfwSwapBuffers(glWrapper.window);
        glfwPollEvents();
    }

	glWrapper.stop(); // stop glfw, close window
	return 0;
}
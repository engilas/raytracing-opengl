#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <chrono>
#include "GLWrapper.h"
#include "SceneManager.h"
#include "Surface.h"

static int wind_width = 660;
static int wind_height = 960;

/*
 * todo
 * cube map
 * box
 * torus
 * plane/box/sphere textures
 */

int main()
{
	GLWrapper glWrapper(false);

	scene_container scene = {};

	glWrapper.init_window();
	wind_width = glWrapper.getWidth();
	wind_height = glWrapper.getHeight();

	scene.scene = SceneManager::create_scene(wind_width, wind_height);
	//scene.scene.bg_color = vec3{ 15 / 255.0f, 150 / 255.0f, 180 / 255.0f };
	//scene.scene.bg_color = vec3{ 1, 1, 1 };
	scene.shadow_ambient = vec3{0.7, 0.7, 0.7};
	scene.ambient_color = vec3{0.2, 0.2, 0.2};

	scene.spheres.push_back(SceneManager::create_sphere({ 2, 0, 6 }, 1, 
		SceneManager::create_material({ 0, 0, 1 }, 50, 0.01)));
	
	scene.lights_point.push_back(SceneManager::create_light_point({ 3, 5, 0, 0.1 }, { 1, 1, 1 }, 55.5));
	scene.lights_direct.push_back(SceneManager::create_light_direct({ 3, -1, 1 }, { 1, 1, 1 }, 1.5));

	scene.planes.push_back(SceneManager::create_plane({ 0, 0, -1 }, { 0, 0, 15 }, 
		SceneManager::create_material({ 100/255.0f, 240/255.0f, 120/255.0f }, 50, 0.3)));
	scene.planes.push_back(SceneManager::create_plane({ 0, 1, 0 }, { 0, -1, 0 }, 
		SceneManager::create_material({ 1, 1, 0 }, 100, 0.3)));

	rt_material coneMaterial = SceneManager::create_material({ 234 / 255.0f, 17 / 255.0f, 82 / 255.0f }, 200, 0.2);
	rt_surface cone = SurfaceFactory::GetEllipticCone(1 / 3.0f, 1 / 3.0f, 1, coneMaterial);
	cone.pos = { -5, 4, 6 };
	cone.rotate({ 1, 0, 0 }, 90);
	cone.yMin = -1;
	cone.yMax = 4;
	scene.surfaces.push_back(cone);
	

	/*scene.lights_point.push_back(SceneManager::create_light_point({0, 0, -1.5, 0.05}, {1,0.8,0}, 2.5));
	scene.lights_point.push_back(SceneManager::create_light_point({0, 0.25, -1.5, 0.05}, {0,0.0,1}, 2.5));*/

	//mirror
	/*scene.spheres.push_back(SceneManager::create_sphere({0, -0.7, -1.5}, 0.29, SceneManager::create_material({1,1,1}, 200, 1, 0, {}, 1)));*/
	//transp
	//scene.spheres.push_back(SceneManager::create_sphere({0, -0.1, -1.5}, 1.3, SceneManager::create_material({1,1,1}, 200, 0.1, 1.125, {10, 10, 10}, 1)));
	//transp 2
	/*scene.spheres.push_back(SceneManager::create_sphere({0, -0.1, -1.5}, 0.15, SceneManager::create_material({1,1,1}, 200, 0.1, 1.125, {0, 10, 0}, 1)));*/
	// scene.spheres.push_back(SceneManager::create_sphere({1001, 0, 0}, 1000, SceneManager::create_material({1,1,1}, 200, 0.1, 0.8, {}, 1)));
	/*scene.planes.push_back(SceneManager::create_plane({1,0,0}, {-1,0,0}, SceneManager::create_material({0,1,0}, 200, 0)));
	scene.planes.push_back(SceneManager::create_plane({-1,0,0}, {1,0,0}, SceneManager::create_material({1,0,0}, 200, 0)));
	scene.planes.push_back(SceneManager::create_plane({0,1,0}, {0,-1,0}, SceneManager::create_material({1,1,1}, 200, 0.1)));
	scene.planes.push_back(SceneManager::create_plane({0,-1,0}, {0,1,0}, SceneManager::create_material({1,1,1}, 200, 0.1)));
	scene.planes.push_back(SceneManager::create_plane({0,0,1}, {0,0,-2}, SceneManager::create_material({1,1,1}, 200, 0.1)));*/

	//rt_material coneMaterial = SceneManager::create_material({ 1, 0, 1 }, 200, 0.1);
	//rt_surface cone = SurfaceFactory::GetEllipticCone(1 / 3.0f, 1 / 3.0f, 1, coneMaterial);
	//cone.pos = { 0, 0, -5 };
	//cone.rotate({ 1, 0, 0 }, 90);
	//cone.xEdge = { -1, 1 };
	////cone.yEdge = { -1, 1 };
	////cone.zEdge = { -1, 1 };
	//scene.surfaces.push_back(cone);

	//scene.rotating_primitives.push_back({0, light, scene.lights_point[0].pos, 0.4, 0.4, 0, 1.1});
	//scene.rotating_primitives.push_back({1, light, scene.lights_point[1].pos, 0.4, 0.4, 2, 1.4});




	// scene.spheres.push_back(SceneManager::create_sphere({ 1, 0.25, 3.5 }, 0.3, SceneManager::create_material({ 0,1,0 }, 30, 0.1)));
	// scene.spheres.push_back(SceneManager::create_sphere({ 1, 0.25, -4 }, 0.3, SceneManager::create_material({ 0,1,0 }, 30, 0.05, 1.125, {4,2, 0.5})));
	// scene.lights.push_back(SceneManager::create_light(point, 25.0f, { 1.0,1.0,1.0 }, { 0.8,4,0 }, { 0 }));
	// scene.lights.push_back(SceneManager::create_light(direct, 0.2f, { 1.0,1.0,1.0 }, {}, { 0, -1, 0.2 }));
	// scene.planes.push_back(SceneManager::create_plane({ 0,1,0 }, { 0,-6,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	// //scene.planes.push_back(SceneManager::create_plane({ 0,-1,0 }, { 0,6,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1, 0, {}, 0.7, 0.2)));
	// scene.planes.push_back(SceneManager::create_plane({ 0,0,-1 }, { 0,0,6 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	// scene.planes.push_back(SceneManager::create_plane({ 0,0,1 }, { 0,0,-6 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	// scene.planes.push_back(SceneManager::create_plane({ 1,0,0 }, { -6,0,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));
	// scene.planes.push_back(SceneManager::create_plane({ -1,0,0 }, { 6,0,0 }, SceneManager::create_material({ 1,1,1 }, 50, 0.1)));

	glWrapper.init_shaders(scene.get_defines());

	SceneManager scene_manager(wind_width, wind_height, scene, &glWrapper);
	scene_manager.init();

	glfwSwapInterval(1);

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
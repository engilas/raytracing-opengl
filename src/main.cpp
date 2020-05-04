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
 * plane/box/sphere textures
 * refactoring (proj structure, use glm, formatting)
 * AA
 */

vec4 getQuaternion(vec3 axis, float angle);
void updateScene(scene_container& scene, float time);

int main()
{
	GLWrapper glWrapper(false);

	scene_container scene = {};

	glWrapper.init_window();
	wind_width = glWrapper.getWidth();
	wind_height = glWrapper.getHeight();

	scene.scene = SceneManager::create_scene(wind_width, wind_height);
	scene.scene.camera_pos = {0, 0, -5};
	//scene.scene.bg_color = vec3{ 15 / 255.0f, 150 / 255.0f, 180 / 255.0f };
	//scene.scene.bg_color = vec3{ 1, 1, 1 };
	//scene.shadow_ambient = vec3{0.7, 0.7, 0.7};
	//scene.ambient_color = vec3{0.2, 0.2, 0.2};

	// blue
	scene.spheres.push_back(SceneManager::create_sphere({ 2, 0, 6 }, 1, 
		SceneManager::create_material({ 0, 0, 1 }, 50, 0.4)));
	// transparent
	scene.spheres.push_back(SceneManager::create_sphere({ -1, 0, 6 }, 1,
		SceneManager::create_material({ 1, 1, 1 }, 200, 0.1, 1.125, {1, 0, 2}, 1), true));
	// planet
	scene.spheres.push_back(SceneManager::create_sphere({ 7000, 7000, 7000 }, 5000, 
		SceneManager::create_material({ 0.1, 0.5, 0.7 }, 1, 0.0f)));
	
	scene.lights_point.push_back(SceneManager::create_light_point({ 3, 5, 0, 0.1 }, { 1, 1, 1 }, 55.5));
	scene.lights_direct.push_back(SceneManager::create_light_direct({ 3, -1, 1 }, { 1, 1, 1 }, 1.5));

	// floor
	scene.boxes.push_back(SceneManager::create_box({ 0, -1.2, 6 }, { 10, 0.2, 5 },
		SceneManager::create_material({ 1, 0.6, 0 }, 100, 0.05)));
	// box
	scene.boxes.push_back(SceneManager::create_box({ 8, 1, 6 }, { 1, 1, 1 }, 
		SceneManager::create_material({ 0.8,0.7,0 }, 50, 0.1)));
	// torus
	scene.toruses.push_back(SceneManager::create_torus({ -9, 0.5, 6 }, { 1.0, 0.5 },
		SceneManager::create_material({ 0.5, 0.4, 1 }, 200, 0.2)));

	// cone
	rt_material coneMaterial = SceneManager::create_material({ 234 / 255.0f, 17 / 255.0f, 82 / 255.0f }, 200, 0.2);
	rt_surface cone = SurfaceFactory::GetEllipticCone(1 / 3.0f, 1 / 3.0f, 1, coneMaterial);
	cone.pos = { -5, 4, 6 };
	cone.quat_rotation = getQuaternion({ 1, 0, 0 }, 90);
	cone.yMin = -1;
	cone.yMax = 4;
	scene.surfaces.push_back(cone);

	// cylinder
	rt_material cylinderMaterial = SceneManager::create_material({ 200 / 255.0f, 255 / 255.0f, 0 / 255.0f }, 200, 0.2);
	rt_surface cylinder = SurfaceFactory::GetEllipticCylinder(1 / 2.0f, 1 / 2.0f, cylinderMaterial);
	cylinder.pos = { 5, 0, 6 };
	cylinder.quat_rotation = getQuaternion({ 1, 0, 0 }, 90);
	cylinder.yMin = -1;
	cylinder.yMax = 1;
	scene.surfaces.push_back(cylinder);

	glWrapper.init_shaders(scene.get_defines());

	std::vector<std::string> faces =
	{
		"../assets/textures/skybox/right.png",
		"../assets/textures/skybox/left.png",
		"../assets/textures/skybox/top.png",
		"../assets/textures/skybox/bottom.png",
		"../assets/textures/skybox/front.png",
		"../assets/textures/skybox/back.png"
	};
	
	glWrapper.setSkybox(GLWrapper::loadCubemap(faces));

	SceneManager scene_manager(wind_width, wind_height, &scene, &glWrapper);
	scene_manager.init();

	glfwSwapInterval(1);

	auto start = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    int frames_count = 0;

    while (!glfwWindowShouldClose(glWrapper.window))
    {
		
        ++frames_count;
        auto newTime = std::chrono::steady_clock::now();
		std::chrono::duration<float> elapsed = (newTime - start);
		std::chrono::duration<float> frameTime = (newTime - currentTime);
		currentTime = newTime;

		updateScene(scene, elapsed.count());
		scene_manager.update(frameTime.count());
        glWrapper.draw();
		glfwSwapBuffers(glWrapper.window);
        glfwPollEvents();
    }

	glWrapper.stop(); // stop glfw, close window
	return 0;
}

void updateScene(scene_container& scene, float time)
{
	rt_sphere* planet = &scene.spheres[2];
	planet->obj.x = cos(time / 50) * 7000;
	planet->obj.z = sin(time / 50) * 7000;

	rt_box* box = &scene.boxes[1];
	box->quat_rotation = getQuaternion({ 0.5774,0.5774,0.5774 }, time * 20);

	rt_torus* torus = &scene.toruses[0];
	torus->quat_rotation = getQuaternion({ 0,1,0 }, time * 30);
}

vec4 getQuaternion(vec3 axis, float angle)
{
	float rad = angle * PI_F / 180.0f;
	float rotationAxis[] = { axis.x, axis.y, axis.z };
	Quaternion<float> quat(rotationAxis, rad);
	return quat.GetStruct();
}
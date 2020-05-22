#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include "GLWrapper.h"
#include "SceneManager.h"
#include "Surface.h"

static int wind_width = 1280;
static int wind_height = 720;

void update_scene(scene_container& scene, float delta, float time);

namespace update {
	int jupiter = -1,
		saturn = -1,
		saturn_rings = -1,
		mars = -1,
		box = -1,
		torus = -1;
}

const glm::quat saturn_pitch = glm::quat(glm::vec3(glm::radians(15.f), 0, 0));

int main()
{
	GLWrapper glWrapper(wind_width, wind_height, false);
	// fullscreen
	//GLWrapper glWrapper(true);
	// window with monitor resolution
	//GLWrapper glWrapper(false);

	// set SMAA quality preset
	glWrapper.enable_SMAA(MEDIUM);
	
	glWrapper.init_window();
	glfwSwapInterval(1); // vsync
	wind_width = glWrapper.getWidth();
	wind_height = glWrapper.getHeight();

	// fix ray direction issues
	if (wind_width % 2 == 1) wind_width++;
	if (wind_height % 2 == 1) wind_height++;

	scene_container scene = {};

	scene.scene = SceneManager::create_scene(wind_width, wind_height);
	scene.scene.camera_pos = { 0, 0, -5 };
	scene.shadow_ambient = glm::vec3{ 0.1, 0.1, 0.1 };
	scene.ambient_color = glm::vec3{ 0.025, 0.025, 0.025 };

	// lights
	scene.lights_point.push_back(SceneManager::create_light_point({ 3, 5, 0, 0.1 }, { 1, 1, 1 }, 25.5));
	scene.lights_direct.push_back(SceneManager::create_light_direct({ 3, -1, 1 }, { 1, 1, 1 }, 1.5));

	// blue sphere
	scene.spheres.push_back(SceneManager::create_sphere({ 2, 0, 6 }, 1,
		SceneManager::create_material({ 0, 0, 1 }, 50, 0.35)));
	// red sphere
	scene.spheres.push_back(SceneManager::create_sphere({ -1, 0, 6 }, 1,
		SceneManager::create_material({ 1, 0, 0 }, 100, 0.1), true));
	// transparent sphere
	scene.spheres.push_back(SceneManager::create_sphere({ 0.5, 2, 6 }, 1,
		SceneManager::create_material({ 1, 1, 1 }, 200, 0.1, 1.125, { 1, 0, 2 }, 1), true));

	// jupiter
	rt_sphere jupiter = SceneManager::create_sphere({}, 5000,
		SceneManager::create_material({}, 0, 0.0f));
	jupiter.textureNum = 1;
	scene.spheres.push_back(jupiter);
	update::jupiter = scene.spheres.size() - 1;

	// saturn
	const int saturnRadius = 4150;
	rt_sphere saturn = SceneManager::create_sphere({}, saturnRadius,
		SceneManager::create_material({}, 0, 0.0f));
	saturn.textureNum = 2;
	saturn.quat_rotation = saturn_pitch;
	scene.spheres.push_back(saturn);
	update::saturn = scene.spheres.size() - 1;

	// mars
	rt_sphere mars = SceneManager::create_sphere({}, 500,
		SceneManager::create_material({}, 0, 0.0f));
	mars.textureNum = 3;
	scene.spheres.push_back(mars);
	update::mars = scene.spheres.size() - 1;

	// ring
	{
		rt_ring ring = SceneManager::create_ring({}, saturnRadius * 1.1166, saturnRadius * 2.35,
			SceneManager::create_material({}, 0, 0));
		ring.textureNum = 4;
		ring.quat_rotation = glm::angleAxis(glm::radians(90.f), glm::vec3(1, 0, 0)) * saturn_pitch;
		scene.rings.push_back(ring);
		update::saturn_rings = scene.rings.size() - 1;
	}

	// floor
	scene.boxes.push_back(SceneManager::create_box({ 0, -1.2, 6 }, { 10, 0.2, 5 },
		SceneManager::create_material({ 1, 0.6, 0 }, 100, 0.05)));
	// box
	rt_box box = SceneManager::create_box({ 8, 1, 6 }, { 1, 1, 1 },
		SceneManager::create_material({ 0.8,0.7,0 }, 50, 0.0));
	box.textureNum = 5;
	scene.boxes.push_back(box);
	update::box = scene.boxes.size() - 1;

	// *** beware! torus calculations is the most heavy part of rendering
	// *** comment next line if you have performance issues
	// torus
	rt_torus torus = SceneManager::create_torus({ -9, 0.5, 6 }, { 1.0, 0.5 },
		SceneManager::create_material({ 0.5, 0.4, 1 }, 200, 0.2));
	torus.quat_rotation = glm::quat(glm::vec3(glm::radians(45.f), 0, 0));
	scene.toruses.push_back(torus);
	update::torus = scene.toruses.size() - 1;

	// cone
	rt_material coneMaterial = SceneManager::create_material({ 234 / 255.0f, 17 / 255.0f, 82 / 255.0f }, 200, 0.2);
	rt_surface cone = SurfaceFactory::GetEllipticCone(1 / 3.0f, 1 / 3.0f, 1, coneMaterial);
	cone.pos = { -5, 4, 6 };
	cone.quat_rotation = glm::quat(glm::vec3(glm::radians(90.f), 0, 0));
	cone.yMin = -1;
	cone.yMax = 4;
	scene.surfaces.push_back(cone);

	// cylinder
	rt_material cylinderMaterial = SceneManager::create_material({ 200 / 255.0f, 255 / 255.0f, 0 / 255.0f }, 200, 0.2);
	rt_surface cylinder = SurfaceFactory::GetEllipticCylinder(1 / 2.0f, 1 / 2.0f, cylinderMaterial);
	cylinder.pos = { 5, 0, 6 };
	cylinder.quat_rotation = glm::quat(glm::vec3(glm::radians(90.f), 0, 0));
	cylinder.yMin = -1;
	cylinder.yMax = 1;
	scene.surfaces.push_back(cylinder);

	rt_defines defines = scene.get_defines();
	glWrapper.init_shaders(defines);

	std::vector<std::string> faces =
	{
		ASSETS_DIR "/textures/sb_nebula/GalaxyTex_PositiveX.jpg",
		ASSETS_DIR "/textures/sb_nebula/GalaxyTex_NegativeX.jpg",
		ASSETS_DIR "/textures/sb_nebula/GalaxyTex_PositiveY.jpg",
		ASSETS_DIR "/textures/sb_nebula/GalaxyTex_NegativeY.jpg",
		ASSETS_DIR "/textures/sb_nebula/GalaxyTex_PositiveZ.jpg",
		ASSETS_DIR "/textures/sb_nebula/GalaxyTex_NegativeZ.jpg"
	};

	glWrapper.set_skybox(GLWrapper::load_cubemap(faces, false));

	auto jupiterTex = glWrapper.load_texture(1, "8k_jupiter.jpg", "texture_sphere_1");
	auto saturnTex = glWrapper.load_texture(2, "8k_saturn.jpg", "texture_sphere_2");
	auto marsTex = glWrapper.load_texture(3, "2k_mars.jpg", "texture_sphere_3");
	auto ringTex = glWrapper.load_texture(4, "8k_saturn_ring_alpha.png", "texture_ring");
	auto boxTex = glWrapper.load_texture(5, "container.png", "texture_box");

	SceneManager scene_manager(wind_width, wind_height, &scene, &glWrapper);
	scene_manager.init();

	auto start = std::chrono::steady_clock::now();
	auto currentTime = std::chrono::steady_clock::now();

	while (!glfwWindowShouldClose(glWrapper.window))
	{
		auto newTime = std::chrono::steady_clock::now();
		std::chrono::duration<float> elapsed = (newTime - start);
		std::chrono::duration<float> frameTime = (newTime - currentTime);
		currentTime = newTime;

		update_scene(scene, frameTime.count(), elapsed.count());
		scene_manager.update(frameTime.count());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, jupiterTex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, saturnTex);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, marsTex);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, ringTex);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, boxTex);
		glWrapper.draw();
		glfwSwapBuffers(glWrapper.window);
		glfwPollEvents();
	}

	glWrapper.stop(); // stop glfw, close window
	return 0;
}

void update_scene(scene_container& scene, float delta, float time)
{
	if (update::jupiter != -1) {
		rt_sphere* jupiter = &scene.spheres[update::jupiter];
		const float jupiterSpeed = 0.02;
		jupiter->obj.x = cos(time * jupiterSpeed) * 20000;
		jupiter->obj.z = sin(time * jupiterSpeed) * 20000;

		jupiter->quat_rotation *= glm::angleAxis(delta / 15, glm::vec3(0, 1, 0));
	}

	if (update::saturn != -1 && update::saturn_rings != -1) {
		rt_sphere* saturn = &scene.spheres[update::saturn];
		rt_ring* ring = &scene.rings[update::saturn_rings];
		const float speed = 0.0082;
		const float dist = 35000;
		const float offset = 1;

		saturn->obj.x = cos(time * speed + offset) * dist;
		saturn->obj.z = sin(time * speed + offset) * dist;

		glm::vec3 axis = glm::vec3(0, 1, 0) * saturn_pitch;
		saturn->quat_rotation *= glm::angleAxis(delta / 10, axis);

		ring->pos.x = cos(time * speed + offset) * dist;
		ring->pos.z = sin(time * speed + offset) * dist;
	}

	if (update::mars != -1) {
		rt_sphere* mars = &scene.spheres[update::mars];
		const float marsSpeed = 0.05;
		mars->obj.x = cos(time * marsSpeed + 0.5f) * 10000;
		mars->obj.z = sin(time * marsSpeed + 0.5f) * 10000;
		mars->obj.y = -cos(time * marsSpeed) * 3000;
		mars->quat_rotation *= glm::angleAxis(delta / 5, glm::vec3(0, 1, 0));
	}

	if (update::box != -1)
	{
		rt_box* box = &scene.boxes[update::box];
		glm::quat q = glm::angleAxis(delta, glm::vec3(0.5774, 0.5774, 0.5774));
		box->quat_rotation *= q;
	}

	if (update::torus != -1)
	{
		rt_torus* torus = &scene.toruses[update::torus];
		torus->quat_rotation *= glm::angleAxis(delta, glm::vec3(0, 1, 0));
	}
}
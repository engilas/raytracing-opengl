#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct rt_defines
{
	int sphere_size;
	int plane_size;
	int surface_size;
	int box_size;
	int torus_size;
	int ring_size;
	int light_point_size;
	int light_direct_size;
	int iterations;
	glm::vec3 ambient_color;
	glm::vec3 shadow_ambient;
};

typedef struct {
	glm::vec3 color; float __p1;

	glm::vec3 absorb;
	float diffuse;

	float reflect;
	float refract;
	int specular;
	float kd;

	float ks;
	float __padding[3];
} rt_material;

typedef struct {
	rt_material material;
	glm::vec4 obj; // pos + radius
	glm::quat quat_rotation = glm::quat(1, 0, 0, 0);
	int textureNum;
	bool hollow;
	float __padding[2];
} rt_sphere;

typedef struct {
	rt_material material;
	glm::vec3 pos; float __p1;
	glm::vec3 normal; float __p2;
} rt_plane;

typedef struct {
	rt_material mat;
	glm::quat quat_rotation = glm::quat(1, 0, 0, 0);
	glm::vec3 pos; float __p1;
	glm::vec3 form;
	int textureNum;
} rt_box;

typedef struct {
	rt_material mat;
	glm::quat quat_rotation = glm::quat(1, 0, 0, 0);
	glm::vec3 pos; float __p1;
	glm::vec2 form; float __p2[2];
} rt_torus;

typedef struct {
	rt_material mat;
	glm::quat quat_rotation = glm::quat(1, 0, 0, 0);
	glm::vec3 pos; int textureNum;
	float r1, r2;
	float __p2[2];
} rt_ring;

typedef struct {
	rt_material mat;
	glm::quat quat_rotation = glm::quat(1, 0, 0, 0);
	float xMin = -FLT_MAX;
	float yMin = -FLT_MAX;
	float zMin = -FLT_MAX;
	float __p0;
	float xMax = FLT_MAX;
	float yMax = FLT_MAX;
	float zMax = FLT_MAX;
	float __p1;
	glm::vec3 pos;
	float a; // x2
	float b; // y2
	float c; // z2
	float d; // z
	float e; // y
	float f; // const

	float __padding[3];
} rt_surface;

typedef enum { sphere, light } primitiveType;

struct rt_light_direct {
	glm::vec3 direction; float __p1;
	glm::vec3 color;

	float intensity;
};

struct rt_light_point {
	glm::vec4 pos; //pos + radius
	glm::vec3 color;
	float intensity;

	float linear_k;
	float quadratic_k;
	float __padding[2];
};

typedef struct {
	glm::quat quat_camera_rotation;
	glm::vec3 camera_pos; float __p1;

	glm::vec3 bg_color;
	int canvas_width;

	int canvas_height;
	int reflect_depth;
	float __padding[2];
} rt_scene;

struct scene_container
{
	rt_scene scene;
	glm::vec3 ambient_color;
	glm::vec3 shadow_ambient;
	std::vector<rt_sphere> spheres;
	std::vector<rt_plane> planes;
	std::vector<rt_surface> surfaces;
	std::vector<rt_box> boxes;
	std::vector<rt_torus> toruses;
	std::vector<rt_ring> rings;
	std::vector<rt_light_point> lights_point;
	std::vector<rt_light_direct> lights_direct;

	rt_defines get_defines()
	{
		return { static_cast<int>(spheres.size()),
			static_cast<int>(planes.size()),
			static_cast<int>(surfaces.size()),
			static_cast<int>(boxes.size()),
			static_cast<int>(toruses.size()),
			static_cast<int>(rings.size()),
			static_cast<int>(lights_point.size()),
			static_cast<int>(lights_direct.size()),
			scene.reflect_depth, ambient_color, shadow_ambient };
	}
};
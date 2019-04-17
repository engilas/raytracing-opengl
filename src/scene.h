#pragma once

#include "primitives.h"
#include <vector>

struct rt_defines
{
	int sphere_size;
	int plain_size;
	int light_point_size;
	int light_direct_size;
	vec3 ambient_color;
};

typedef struct {
	vec3 color; float __p1;

	vec3 absorb;
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
	vec4 obj;
	//vec3 pos;
	//float radius;
} rt_sphere;

struct rt_plain {
	rt_material material;
    vec3 pos; float __p1;
	vec3 normal; float __p2;
};

typedef enum { sphere } primitiveType;

// typedef struct {
// 	vec3 pos; float __p1;
// 	vec3 direction; float __p2;
//
//     vec3 color;
//     lightType type;
//
// 	float intensity;
//     float radius;
//
//     float __padding[2];
// } rt_light;

struct rt_light_direct {
	vec3 direction; float __p1;
	vec3 color;

	float intensity;
};

struct rt_light_point {
	vec4 pos; //pos + radius
	vec3 color;

	float intensity;
};

typedef struct {
    vec4 quat_camera_rotation;
	vec3 camera_pos; float __p1;

	vec3 bg_color;
	int canvas_width;

	int canvas_height;
	float viewport_width;
	float viewport_height;
	float viewport_dist;

	int reflect_depth;
	float __padding[3];

    //float __padding[1];
} rt_scene;

typedef struct
{
	void* primitive;
	primitiveType type;
	float a;
	float b;
	float current;
	float speed;
} rotating_primitive;

struct scene_container
{
	rt_scene scene;
	vec3 ambient_color;
	std::vector<rt_sphere> spheres;
	std::vector<rt_plain> plains;
	std::vector<rt_light_point> lights_point;
	std::vector<rt_light_direct> lights_direct;
	std::vector<rotating_primitive> rotating_primitives;

	rt_defines get_defines()
	{
		return {static_cast<int>(spheres.size()), static_cast<int>(plains.size()), static_cast<int>(lights_point.size()), static_cast<int>(lights_direct.size()), ambient_color};
	}
};
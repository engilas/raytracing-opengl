#pragma once

#include "primitives.h"
#include <vector>
#include "quaternion.h"
#define PI_F 3.14159265358979f

struct rt_defines
{
	int sphere_size;
	int plane_size;
	int surface_size;
	int light_point_size;
	int light_direct_size;
	int iterations;
	vec3 ambient_color;
	vec3 shadow_ambient;
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
	// pos + radius
	vec4 obj;
} rt_sphere;

struct rt_plane {
	rt_material material;
    vec3 pos; float __p1;
	vec3 normal; float __p2;
};

typedef struct {
	rt_material mat;
	vec4 quat_rotation = Quaternion<float>::Identity().GetStruct();
	float xMin = -FLT_MAX;
	float yMin = -FLT_MAX;
	float zMin = -FLT_MAX;
	float __p0;
	float xMax = FLT_MAX;
	float yMax = FLT_MAX;
	float zMax = FLT_MAX;
	float __p1;
	vec3 pos;
	float a; // x2
	float b; // y2
	float c; // z2
	float d; // z
	float e; // y
	float f; // const
	
	float __padding[1];

	void rotate(vec3 axis, float angle)
	{
		float rad = angle * PI_F / 180.0f;
		float rotationAxis[] = { axis.x, axis.y, axis.z };
		Quaternion<float> quat(rotationAxis, rad);
		quat_rotation = quat.GetStruct();
	}
} rt_surface;

typedef enum { sphere, light } primitiveType;

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
	int reflect_depth;
	float __padding[2];

    //float __padding[1];
} rt_scene;

struct rotating_primitive
{
	int index;
	primitiveType type;
	vec4 pos;
	float a;
	float b;
	float current;
	float speed;
};

struct scene_container
{
	rt_scene scene;
	vec3 ambient_color;
	vec3 shadow_ambient;
	std::vector<rt_sphere> spheres;
	std::vector<rt_plane> planes;
	std::vector<rt_surface> surfaces;
	std::vector<rt_light_point> lights_point;
	std::vector<rt_light_direct> lights_direct;
	std::vector<rotating_primitive> rotating_primitives;

	rt_defines get_defines()
	{
		return {static_cast<int>(spheres.size()),
			static_cast<int>(planes.size()),
			static_cast<int>(surfaces.size()),
			static_cast<int>(lights_point.size()),
			static_cast<int>(lights_direct.size()),
			scene.reflect_depth, ambient_color, shadow_ambient};
	}
};
#pragma once

#include "primitives.h"

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
	vec4 obj;
    //rt_material material; 
	//vec3 pos;
	//float radius;
} rt_sphere;

struct rt_plain {
	rt_material material;
    vec3 pos; float __p1;
	vec3 normal; float __p2;
};

typedef enum { ambient, point, direct } lightType;
typedef enum { sphere } primitiveType;

typedef struct {
	vec3 pos; float __p1;
	vec3 direction; float __p2;

    vec3 color;
    lightType type;

	float intensity;
    float radius;

    float __padding[2];
} rt_light;

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
	int sphere_count;
	int light_count;
    int plain_count;

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
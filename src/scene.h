#pragma once

#include "primitives.h"

typedef struct {
	float4 center;
	float4 color;
	float radius;
	float reflect;
	int specular;

    float __padding[1];
} rt_sphere;

typedef enum { ambient, point, direct } lightType;

typedef struct {
	float4 position;
	float4 direction;

    lightType type;
	float intensity;

    float __padding[2];
} rt_light;

typedef struct {
	float4 camera_pos;
    quaternion camera_rotation;
	float4 bg_color;

	int canvas_width;
	int canvas_height;
	float viewport_width;
	float viewport_height;

	float viewport_dist;
	int reflect_depth;
	int sphere_count;
	int light_count;
} rt_scene;
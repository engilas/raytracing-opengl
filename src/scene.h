#include "primitives.h"

#ifndef SCENE_H
#define SCENE_H

typedef struct {
	cl_float4 center;
	cl_float4 color;
	cl_float radius;
	cl_float reflect;
	cl_int specular;
} rt_sphere;

typedef enum { Ambient, Point, Direct } lightType;

typedef struct {
	lightType type;
	cl_float intensity;
	cl_float4 position;
	cl_float4 direction;
} rt_light;

typedef struct {
	cl_float4 camera_pos;
	cl_float4 bg_color;
	cl_float canvas_width;
	cl_float canvas_height;
	cl_float viewport_width;
	cl_float viewport_height;
	cl_float viewport_dist;
	cl_int reflect_depth;

	cl_int sphere_count;
	cl_int light_count;

	quaternion camera_rotation;

	rt_sphere spheres[64];
	rt_light lights[16];
} rt_scene;

#endif
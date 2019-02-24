#include <CL/cl.h>

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

typedef struct {
	cl_float w;
	cl_float4 v;
} quaternion;

#endif
#pragma once
#include "scene.h"

class SurfaceFactory {
public:

	static rt_surface GetEllipsoid(float a, float b, float c, rt_material material)
	{
		rt_surface surface = {};
		surface.a = powf(a, -2);
		surface.b = powf(b, -2);
		surface.c = powf(c, -2);
		surface.f = -1;
		surface.mat = material;
		return surface;
	}

	static rt_surface GetEllipticParaboloid(float a, float b, rt_material material)
	{
		rt_surface surface = {};
		surface.a = powf(a, -2);
		surface.b = powf(b, -2);
		surface.d = -1;
		surface.mat = material;
		return surface;
	}

	static rt_surface GetHyperbolicParaboloid(float a, float b, rt_material material)
	{
		rt_surface surface = {};
		surface.a = powf(a, -2);
		surface.b = -powf(b, -2);
		surface.d = -1;
		surface.mat = material;
		return surface;
	}

	static rt_surface GetEllipticHyperboloidOneSheet(float a, float b, float c, rt_material material)
	{
		rt_surface surface = {};
		surface.a = powf(a, -2);
		surface.b = powf(b, -2);
		surface.c = -powf(c, -2);
		surface.f = -1;
		surface.mat = material;
		return surface;
	}

	static rt_surface GetEllipticHyperboloidTwoSheets(float a, float b, float c, rt_material material)
	{
		rt_surface surface = {};
		surface.a = powf(a, -2);
		surface.b = powf(b, -2);
		surface.c = -powf(c, -2);
		surface.f = 1;
		surface.mat = material;
		return surface;
	}

	static rt_surface GetEllipticCone(float a, float b, float c, rt_material material)
	{
		rt_surface surface = {};
		surface.a = powf(a, -2);
		surface.b = powf(b, -2);
		surface.c = -powf(c, -2);
		surface.mat = material;
		return surface;
	}

	static rt_surface GetEllipticCylinder(float a, float b, rt_material material)
	{
		rt_surface surface = {};
		surface.a = powf(a, -2);
		surface.b = powf(b, -2);
		surface.f = -1;
		surface.mat = material;
		return surface;
	}

	static rt_surface GetHyperbolicCylinder(float a, float b, rt_material material)
	{
		rt_surface surface = {};
		surface.a = powf(a, -2);
		surface.b = -powf(b, -2);
		surface.f = -1;
		surface.mat = material;
		return surface;
	}

	static rt_surface GetParabolicCylinder(float a, rt_material material)
	{
		rt_surface surface = {};
		surface.a = 1;
		surface.e = 2 * a;
		surface.mat = material;
		return surface;
	}
};

#pragma once

struct vec4 {
	float x, y, z, w;
};

struct vec3 {
	float x, y, z;

	std::string toString() const
	{
		return std::string().append("vec3(").append(std::to_string(x)).append(",").append(std::to_string(y)).append(",").append(std::to_string(z)).append(")");
	}
};

struct vec2 {
	float x, y;
};
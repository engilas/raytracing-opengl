// Sandbox_CPP.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <cmath>
#include <algorithm>

template<typename T>
class Vec3
{
public:
	Vec3() : x(T(0)), y(T(0)), z(T(0)) {}
	Vec3(T xx) : x(xx), y(xx), z(xx) {}
	Vec3(T xx, T yy, T zz) : x(xx), y(yy), z(zz) {}
	Vec3 operator + (const Vec3 &v) const
	{
		return Vec3(x + v.x, y + v.y, z + v.z);
	}
	Vec3 operator - (const Vec3 &v) const
	{
		return Vec3(x - v.x, y - v.y, z - v.z);
	}
	Vec3 operator - () const
	{
		return Vec3(-x, -y, -z);
	}
	Vec3 operator * (const T &r) const
	{
		return Vec3(x * r, y * r, z * r);
	}
	Vec3 operator * (const Vec3 &v) const
	{
		return Vec3(x * v.x, y * v.y, z * v.z);
	}
	T dotProduct(const Vec3<T> &v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}
	Vec3& operator /= (const T &r)
	{
		x /= r, y /= r, z /= r; return *this;
	}
	Vec3& operator *= (const T &r)
	{
		x *= r, y *= r, z *= r; return *this;
	}
	Vec3 crossProduct(const Vec3<T> &v) const
	{
		return Vec3<T>(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}
	T norm() const
	{
		return x * x + y * y + z * z;
	}
	T length() const
	{
		return sqrt(norm());
	}
	const T& operator [] (uint8_t i) const { return (&x)[i]; }
	T& operator [] (uint8_t i) { return (&x)[i]; }
	Vec3& normalize()
	{
		T n = norm();
		if (n > 0) {
			T factor = 1 / sqrt(n);
			x *= factor, y *= factor, z *= factor;
		}

		return *this;
	}

	friend Vec3 operator * (const T &r, const Vec3 &v)
	{
		return Vec3<T>(v.x * r, v.y * r, v.z * r);
	}
	friend Vec3 operator / (const T &r, const Vec3 &v)
	{
		return Vec3<T>(r / v.x, r / v.y, r / v.z);
	}

	friend std::ostream& operator << (std::ostream &s, const Vec3<T> &v)
	{
		return s << '[' << v.x << ' ' << v.y << ' ' << v.z << ']';
	}

	T x, y, z, w;
};

typedef Vec3<float> vec2;
typedef Vec3<float> vec3;
typedef Vec3<float> vec4;

struct rt_scene {
	vec4 camera_pos;
	vec4 quat_camera_rotation;
	vec4 bg_color;

	int canvas_width;
	int canvas_height;
	float viewport_width;
	float viewport_height;

	float viewport_dist;
	int reflect_depth;
	int sphere_count;
	int light_count;
} scene;

float maxDist = 1000;
const vec3 amb = vec3(1.0);
const float eps = 0.001;

#define sp_size 11

vec4 spheres[sp_size];
vec4 colors[sp_size];
vec2 materials[sp_size];

vec2 _vec2(float x, float y)
{
	return vec2(x, y, 0);
}

vec3 _vec3(float x, float y, float z)
{
	return vec3(x, y, z);
}

vec4 _vec4(float x, float y, float z, float w)
{
	auto res = vec4(x, y, z);
	res.w = w;
	return res;
}

vec3 reflect(vec3 i, vec3 n)
{
	return i - 2.0 * n.dotProduct(i) * n;
}

inline
float clamp(const float &v, const float &lo, const float &hi)
{
	return std::max(lo, std::min(hi, v));
}

vec3 refract(vec3 I, vec3 N, float ior)
{
	float cosi = clamp(I.dotProduct(N) , -1, 1);
	float etai = 1, etat = ior;
	vec3 n = N;
	if (cosi < 0) { cosi = -cosi; }
	else { std::swap(etai, etat); n = -N; }
	float eta = etai / etat;
	float k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;

	//float k = 1.0 - eta * eta * (1.0 - N.dotProduct(I) * N.dotProduct(I));
	//if (k < 0.0)
	//	return vec3(0.0);       // or genDType(0.0)
	//else
	//	return  eta * I - (eta * N.dotProduct(I) + sqrt(k)) * N;
}



vec4 multiplyQuaternion(vec4 q1, vec4 q2) {
	vec4 result;

	result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
	result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	result.y = q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z;
	result.z = q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x;

	return result;
}

vec3 Rotate(vec4 q, vec3 v)
{
	vec4 qv = _vec4(v.x, v.y, v.z, 0);

	vec4 mult = multiplyQuaternion(q, qv);
	float scale = 1 / (q.w * q.w + q.dotProduct(q));
	vec4 inverse = -scale * q;
	inverse.w = scale * q.w;
	vec3 result = vec3(multiplyQuaternion(mult, inverse));

	return result;
}

vec3 getRayDir(vec2 pixel_coords)
{
	//vec2 uv = (pixel_coords.xy / vec2(scene.canvas_width, scene.canvas_height)) * 2.0 - 1.0;


	int x = int(pixel_coords.x - scene.canvas_width / 2.0);
	int y = int(pixel_coords.y - scene.canvas_height / 2.0);
	//vec2 uv = vec2(x,y); //* (scene.canvas_width / scene.canvas_height);
	//uv.x *= scene.canvas_width / scene.canvas_height;
	//return normalize(uv.x * cam.right + uv.y * cam.up + cam.forward);

	vec3 result = vec3(
		x * scene.viewport_width / scene.canvas_width,
		y * scene.viewport_height / scene.canvas_height,
		scene.viewport_dist);
	// return result;
	return Rotate(scene.quat_camera_rotation, result).normalize();
}


void init()
{
	// X Y Z Radius
	spheres[0] = _vec4(1.5, 0, -1.5, 0.1);
	spheres[1] = _vec4(-1, 0.25, -1.5, 0.1);
	spheres[2] = _vec4(0, -0.7, -1.5, 0.3);
	spheres[3] = _vec4(0, -0.1, -1.5, 0.3);
	spheres[4] = _vec4(0, -0.1, -1.5, 0.15);
	spheres[5] = _vec4(1001.0, 0, 0, 1000.0);
	spheres[6] = _vec4(-1001.0, 0, 0, 1000.0);
	spheres[7] = _vec4(0, 1001.0, 0, 1000.0);
	spheres[8] = _vec4(0, -1001.0, 0, 1000.0);
	spheres[9] = _vec4(0, 0, -1002.0, 1000.0);
	spheres[10] = _vec4(0, -0.1, -1, 0.15);

	//R G B Diffuse
	colors[0] = _vec4(1.0, 0.8, 0.0, -1.0);
	colors[1] = _vec4(0.0, 0.0, 1.0, -1.0);
	colors[2] = _vec4(1.0, 1.0, 1.0, 1.0);
	colors[3] = _vec4(1.0, 1.0, 1.0, 1.0);
	colors[4] = _vec4(1.0, 0.0, 0.0, 1.0);
	colors[5] = _vec4(0.0, 1.0, 0.0, 0.7);
	colors[6] = _vec4(1.0, 0.0, 0.0, 0.7);
	colors[7] = _vec4(1.0, 1.0, 1.0, 0.7);
	colors[8] = _vec4(1.0, 1.0, 1.0, 0.7);
	colors[9] = _vec4(1.0, 1.0, 1.0, 0.7);
	colors[10] = _vec4(1.0, 0.0, 0.0, 0.5);

	//Reflection Coeff, Refraction index
	materials[0] = _vec2(0.0, 0.0);
	materials[1] = _vec2(0.0, 0.0);
	materials[2] = _vec2(1.0, 0.0);
	materials[3] = _vec2(0.5, 1.25);
	materials[4] = _vec2(0.5, 1.25);
	materials[5] = _vec2(0.0, 0.0);
	materials[6] = _vec2(0.0, 0.0);
	materials[7] = _vec2(0.1, 0.0);
	materials[8] = _vec2(0.1, 0.0);
	materials[9] = _vec2(0.1, 0.0);
	materials[10] = _vec2(0.2, 0.0);

	// cam.up       = vec3(0.0, 1.0, 0.0);
	// cam.right    = vec3(1.0, 0.0, 0.0);
	// cam.forward  = vec3(0.0, 0.0,-1.0);
	// cam.position = vec3(0.0, 0.0,-0.2);
}

bool intersectSphere(vec3 ro, vec3 rd, vec4 sp, float tm, float &t)
{
	bool r = false;
	vec3 v = ro - sp;
	float b = v.dotProduct(rd);
	float c = v.dotProduct(v) - sp.w * sp.w;
	t = b * b - c;
	if (t > 0.0)
	{
		float sqrt_ = sqrt(t);
		t = -b - sqrt_;
		if (t < 0.0) t = -b + sqrt_;
		r = (t > 0.0) && (t < tm);
	}
	return r;
}

float calcInter(vec3 ro, vec3 rd, vec4 &ob, vec4 &col, vec2 &mat)
{
	float tm = maxDist;
	float t;

	if (intersectSphere(ro, rd, spheres[0], tm, t))
	{
		ob = spheres[0]; col = colors[0]; tm = t; mat = materials[0];
	}
	if (intersectSphere(ro, rd, spheres[1], tm, t))
	{
		ob = spheres[1]; col = colors[1]; tm = t; mat = materials[1];
	}
	if (intersectSphere(ro, rd, spheres[2], tm, t))
	{
		ob = spheres[2]; col = colors[2]; tm = t; mat = materials[2];
	}
	if (intersectSphere(ro, rd, spheres[3], tm, t))
	{
		ob = spheres[3]; col = colors[3]; tm = t; mat = materials[3];
	}
	//if(intersectSphere(ro,rd,spheres[4],tm,t)) { ob = spheres[4]; col = colors[4]; tm = t; mat = materials[4]; }
	if (intersectSphere(ro, rd, spheres[5], tm, t))
	{
		ob = spheres[5]; col = colors[5]; tm = t; mat = materials[5];
	}
	if (intersectSphere(ro, rd, spheres[6], tm, t))
	{
		ob = spheres[6]; col = colors[6]; tm = t; mat = materials[6];
	}
	if (intersectSphere(ro, rd, spheres[7], tm, t))
	{
		ob = spheres[7]; col = colors[7]; tm = t; mat = materials[7];
	}
	if (intersectSphere(ro, rd, spheres[8], tm, t))
	{
		ob = spheres[8]; col = colors[8]; tm = t; mat = materials[8];
	}
	if (intersectSphere(ro, rd, spheres[9], tm, t))
	{
		ob = spheres[9]; col = colors[9]; tm = t; mat = materials[9];
	}
	//if (intersectSphere(ro, rd, spheres[10], tm, t)) { ob = spheres[10]; col = colors[10]; tm = t; mat = materials[10]; }

	return tm;
}

bool inShadow(vec3 ro, vec3 rd, float d)
{
	float t;
	bool ret = false;

	if (intersectSphere(ro, rd, spheres[2], d, t))
	{
		ret = true;
	}
	if (intersectSphere(ro, rd, spheres[3], d, t))
	{
		ret = true;
	}
	//if(intersectSphere(ro,rd,spheres[4],d,t)){ ret = true; }
	if (intersectSphere(ro, rd, spheres[5], d, t))
	{
		ret = true;
	}
	if (intersectSphere(ro, rd, spheres[6], d, t))
	{
		ret = true;
	}
	if (intersectSphere(ro, rd, spheres[7], d, t))
	{
		ret = true;
	}
	if (intersectSphere(ro, rd, spheres[8], d, t))
	{
		ret = true;
	}
	if (intersectSphere(ro, rd, spheres[9], d, t))
	{
		ret = true;
	}
	//if (intersectSphere(ro, rd, spheres[10], d, t)) { ret = true; }

	return ret;
}

vec3 calcShade(vec3 pt, vec4 ob, vec4 col, vec2 mat, vec3 n)
{

	float dist, diff;
	vec3 lcol, l;

	vec3 color = vec3(0.0);
	vec3 ambcol = amb * (1.0 - col.w) * col;
	vec3 scol = col.w * col;
	vec3 c_color;

	if (col.w > 0.0) //If its not a light
	{
		l = spheres[0] - pt;
		dist = l.length();
		l = l.normalize();
		lcol = colors[0];
		diff = n.dotProduct(l);
		if (diff >= 0)
		{
			c_color = (ambcol * lcol + lcol * diff * scol) * (1 / (1.0 + dist * dist));
			if (inShadow(pt, l, dist))
				c_color *= 0.0;
			color = color + c_color;
		}

		l = spheres[1] - pt;
		dist = l.length();
		l = l.normalize();
		vec3 lcol = colors[1];
		diff = n.dotProduct(l);
		if (diff >= 0)
		{
			c_color = (ambcol * lcol + lcol * diff * scol) * (1 / (1.0 + dist * dist));
			if (inShadow(pt, l, dist))
				c_color *= 0.0;
			color = color + c_color;
		}
	}
	else
		color = col;

	return color;
}

float getFresnel(vec3 n, vec3 rd, float r0)
{
	float ndotv = clamp(n.dotProduct(-rd), 0.0, 1.0);
	return r0 + (1.0 - r0) * pow(1.0 - ndotv, 5.0);
}

vec3 getReflection(vec3 ro, vec3 rd)
{
	vec3 color = vec3(0);
	vec4 ob, col;
	vec2 mat;
	float tm = calcInter(ro, rd, ob, col, mat);
	if (tm < maxDist)
	{
		vec3 pt = ro + rd * tm;
		vec3 n = (pt - ob).normalize();
		color = calcShade(pt, ob, col, mat, n);
	}
	return color;
}


//int main()
//{
//	auto ro = _vec3(0, 0, 0);
//	auto rd = _vec3(0, 0, -1);
//	auto sp = _vec4(0, 0, 0.95, 1);
//
//	float result;
//	intersectSphere(ro, rd, sp, 1e10, result);
//
//	std::cout << "Hello World!\n";
//}

void set_scene(int width, int height, int spheresCount, int lightCount)
{
	auto min = width > height ? height : width;

	scene.camera_pos = {2.16, 0, -2.3};
	scene.canvas_height = height;
	scene.canvas_width = width;
	scene.viewport_dist = 1;
	scene.viewport_height = height / static_cast<float>(min);
	scene.viewport_width = width / static_cast<float>(min);
	scene.bg_color = { 0,0.2,0.7 };
	scene.reflect_depth = 3;

	scene.sphere_count = spheresCount;
	scene.light_count = lightCount;
	const float PI_F = 3.14159265358979f;
	scene.quat_camera_rotation = _vec4(0, 1, 0, 180 * PI_F / 180.0f);
}

void main()
{
	//scene.quat_camera_rotation = _vec4(0,0,-1,0);
	//scene.canvas_width =
	auto width = 640.0f;
	auto height = 640.0f;
	set_scene(width, height, 5, 5);

	init();
	float fresnel, tm;
	vec4 ob, col;
	vec2 mat;
	vec3 pt, refCol, n, refl;

	vec2 pixel_coords = _vec2(width / 2, height / 2);

	vec3 mask = vec3(1.0);
	vec3 color = vec3(0.0);
	vec3 ro = scene.camera_pos;
	vec3 rd = vec3(-1, 0, 0);//getRayDir(pixel_coords);

	auto iterations = 5;

	for (int i = 0; i < iterations; i++)
	{
		tm = calcInter(ro, rd, ob, col, mat);

		if (tm >= maxDist) break;

		pt = ro + rd * tm;
		n = (pt - ob).normalize();
		fresnel = getFresnel(n, rd, mat.x);
		mask *= fresnel;

		bool outside = rd.dotProduct(n) < 0;
		if (mat.y > 0.0) // Refractive
		{
			ro = outside ? pt - n * eps : pt + n * eps;
			if (outside)
			{
				refl = reflect(rd, n);
				refCol = getReflection(outside ? pt + n * eps : pt - n * eps, refl);
				color = color + refCol * mask;
				mask = col * (1.0 - fresnel) * (mask * (1 / fresnel));
			}
			rd = refract(rd, outside ? n : -n, mat.y);
		}
		else if (mat.x > 0.0) // Reflective
		{
			ro = outside ? pt + n * eps : pt - n * eps;
			color = color + calcShade(ro, ob, col, mat, n) * (1.0 - fresnel) * mask * (1 / fresnel);
			rd = reflect(rd, n);
		}
		else // Diffuse
		{
			color = color + calcShade(outside ? pt + n * eps : pt - n * eps, ob, col, mat, outside ? n : -n) * mask * (1 / fresnel);
			break;
		}

	}

	//imageStore(img_output, pixel_coords, vec4(color, 1.0));
	//imageStore(img_output,);
	//fragColor = vec4(color,1.0);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

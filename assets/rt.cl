#define MAX_RECURSION_DEPTH 5
#define SHADOW_ENABLED 1
#define T_MIN 0.05f

typedef struct {
	float w;
	float4 v;
} __attribute__((packed)) quaternion;

typedef struct {
	float4 center;
	float4 color;
	float radius;
	float reflect;
	int specular;
} __attribute__((packed)) rt_sphere;

typedef enum { Ambient, Point, Direct } lightType;

typedef struct {
	lightType type;
	float intensity;
	float4 position;
	float4 direction;
} __attribute__((packed)) rt_light;

typedef struct {
	float4 camera_pos;
	float4 bg_color;
	float canvas_width;
	float canvas_height;
	float viewport_width;
	float viewport_height;
	float viewport_dist;
	int reflect_depth;

	int sphere_count;
	int light_count;

	quaternion camera_rotation;

	rt_sphere spheres[64];
	rt_light lights[16];
} rt_scene;

quaternion multiplyQuaternion(quaternion *q1, quaternion *q2) {
	quaternion result;

	result.w =   q1->w*q2->w - q1->v.x * q2->v.x - q1->v.y * q2->v.y - q1->v.z * q2->v.z;
	result.v.x = q1->w*q2->v.x + q1->v.x * q2->w + q1->v.y * q2->v.z - q1->v.z * q2->v.y;
	result.v.y = q1->w*q2->v.y + q1->v.y * q2->w + q1->v.z * q2->v.x - q1->v.x * q2->v.z;
	result.v.z = q1->w*q2->v.z + q1->v.z * q2->w + q1->v.x * q2->v.y - q1->v.y * q2->v.x;

	return result;
}

float4 Rotate(__constant quaternion *q, float4 *v)
{
	quaternion qv;
	qv.w = 0;
	qv.v = *v;

	quaternion tmp = *q;
	quaternion mult = multiplyQuaternion(&tmp, &qv);
	quaternion inverse;
	float scale = 1 / (q->w*q->w + dot(q->v, q->v));
	inverse.w = scale * q->w;
	inverse.v = - scale * q->v;
	quaternion result = multiplyQuaternion(&mult, &inverse);

	return result.v;
}

float4 CanvasToViewport(float x, float y, __constant rt_scene* scene)
{
	float4 result = (float4) (x * scene->viewport_width / scene->canvas_width,
		y * scene->viewport_height / scene->canvas_height,
		scene->viewport_dist,
		0);
	return Rotate(&scene->camera_rotation, &result);
}

float IntersectRaySphere(float4 o, float4 d, float tMin, __constant rt_sphere* sphere)
{
	float t1, t2;

	float4 c = sphere->center;
	float r = sphere->radius;
	float4 oc = o - c;

	float k1 = dot(d, d);
	float k2 = 2 * dot(oc, d);
	float k3 = dot(oc, oc) - r * r;
	float discr = k2 * k2 - 4 * k1 * k3;

	if (discr < 0)
	{
		return INFINITY;
	}
	else
	{
		t1 = (-k2 + sqrt(discr)) / (2 * k1);
		t2 = (-k2 - sqrt(discr)) / (2 * k1);
	}

	float t = INFINITY;
	if (t1 < t && t1 >= tMin)
	{
		t = t1;
	}
	if (t2 < t && t2 >= tMin)
	{
		t = t2;
	}

	return t;
}

float4 ReflectRay(float4 r, float4 normal) {
	return 2*normal*dot(r, normal) - r;
}

void ClosestIntersection(float4 o, float4 d, float tMin, float tMax, __constant rt_scene *scene, float *t, int *sphereIndex) {
	float closest = INFINITY;
	int sphere_index = -1;

	for (int i = 0; i < scene->sphere_count; i++)
	{
		float t = IntersectRaySphere(o, d, tMin, scene->spheres + i);

		if (t >= tMin && t <= tMax && t < closest)
		{
			closest = t;
			sphere_index = i;
		}
	}

	*t = closest;
	*sphereIndex = sphere_index;
}

float ComputeLighting(float4 point, float4 normal, __constant rt_scene *scene, float4 view, int specular)
{
	float sum = 0;
	float4 L;

	for (int i = 0; i < scene->light_count; i++)
	{
		__constant rt_light *light = scene->lights + i;
		if (light->type == Ambient) {
			sum += light->intensity;
		}
		else {
			float tMax = 0;
			if (light->type == Point) {
				L = light->position - point;
				tMax = 1;
			}
			if (light->type == Direct) {
				L = light->direction;
				tMax = INFINITY;
			}

			//#ifdef SHADOW_ENABLED
			int sphereIndex;
			float t;
			ClosestIntersection(point, L, T_MIN, tMax, scene, &t, &sphereIndex);
			if (sphereIndex != -1) continue;
			//#endif

			float nDotL = dot(normal, L);
			if (nDotL > 0) {
				sum += light->intensity * nDotL / (length(normal) * length(L));
			}

			if (specular <= 0) continue;

			float4 r = ReflectRay(L, normal);
			float rDotV = dot (r, view);
			if (rDotV > 0)
			{
				sum += light->intensity * pow(rDotV / (length(r) * length(view)), specular);
			}
		}
	}
	return sum;
}

float4 TraceRay(float4 o, float4 d, float tMin, float tMax,
	__constant rt_scene *scene)
{
	if (scene->reflect_depth == 0) return (float4)(0,0,0,0);

	float closest;
	int sphere_index;

	float4 colors[MAX_RECURSION_DEPTH];
    float reflects[MAX_RECURSION_DEPTH];

	int recursionCount = 0;

	for (int i = 0; i < scene->reflect_depth; i++)
	{
		ClosestIntersection(o, d, tMin, tMax, scene, &closest, &sphere_index);
		
		if (sphere_index == -1)
		{
			colors[recursionCount] = scene->bg_color;
			reflects[recursionCount] = 0;
			++recursionCount;
			break;
		}
		__constant rt_sphere *sphere = scene->spheres+sphere_index;
		float4 p = o + (d * closest);
		float4 normal = normalize(p - sphere->center);

		//good for surfaces, bad for box, sphere
		// if (dot(normal, d) > 0)
		// {
		// 	normal = -normal;
		// }
		float4 view = -d;
		colors[recursionCount] = sphere->color * ComputeLighting(p, normal, scene, view, sphere->specular);
		reflects[recursionCount] = sphere->reflect;
		++recursionCount;
		if (sphere->reflect <= 0 || scene->reflect_depth == 1)
			break;

		if (i < scene->reflect_depth - 1) {
			//setup for next iteration
			o = p;
			d = ReflectRay(view, normal);
		}
	}

	if (recursionCount <= 1)
	 	return colors[0];

	float4 totalColor = colors[recursionCount - 1];

	for(int i = recursionCount - 2; i >= 0; i--)
	{
		float reflect = reflects[i];
		float4 prevColor = colors[i];
		totalColor = prevColor * (1 - reflect) + totalColor * reflect;
	}

	return totalColor;
}

__kernel void rt(
	__constant __read_only rt_scene *scene,
	__write_only image2d_t output)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int width = scene->canvas_width;
	const int height = scene->canvas_height;

	if (x >= width || y >= height) return;
	int xCartesian = x - width / 2.0f;
	int yCartesian = height / 2.0f - y;

	float4 d = CanvasToViewport(xCartesian, yCartesian, scene);
	float4 color = TraceRay(scene->camera_pos, d, T_MIN, INFINITY, scene);

	write_imagef(output, (int2)(x, y), color);
}
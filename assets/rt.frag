#version 430

#define MAX_RECURSION_DEPTH 5

#define T_MIN 0.001f
#define REFLECT_BIAS 0.001f
#define SHADOW_BIAS 0.0001f

#define FLT_MIN 1.175494351e-38
#define FLT_MAX 3.402823466e+38
#define PI_F 3.14159265358979f

#define LIGHT_AMBIENT 0
#define LIGHT_POINT 1
#define LIGHT_DIRECT 2

#define SHADOW_ENABLED 1
#define DBG 0

#define MULTI_INTERSECTION 1
#define CUSTOM_REFRACT 1
#define TOTAL_INTERNAL_REFLECTION 1
#define REFRACTIVE_INDEX_AIR 1.00029
#define DO_FRESNEL 1
// #define LIGHT_AMBIENT2 vec3(0.00)
// #define OBJECT_ABSORB  vec3(0.0, 5.0, 5.0) // for beers law
#define AIM_ENABLED 1
#define PLANE_ONESIDE 1

struct rt_material {
	vec3 color;
	vec3 absorb;

	float diffuse;
	float x;
	float y;
	int specular;
	float kd;
	float ks;
};

struct rt_sphere {
	rt_material mat;
	vec4 obj;
};

struct rt_plain {
	rt_material mat;
	vec3 pos;
	vec3 normal;
};

struct rt_light_direct {
	vec3 direction;
	vec3 color;

	float intensity;
};

struct rt_light_point {
	vec4 pos; //pos + radius
	vec3 color;

	float intensity;
};

struct rt_scene {
	vec4 quat_camera_rotation;//uniform
	vec3 camera_pos;//uniform
	vec3 bg_color;//uniform

	int canvas_width;// uniform
	int canvas_height;// uniform

	int reflect_depth;//define
};

struct hit_record {
	rt_material mat;
  	vec3 n;
};

#define SPHERE_SIZE {SPHERE_SIZE}
#define PLAIN_SIZE {PLAIN_SIZE}
#define LIGHT_DIRECT_SIZE {LIGHT_DIRECT_SIZE}
#define LIGHT_POINT_SIZE {LIGHT_POINT_SIZE}
#define AMBIENT_COLOR {AMBIENT_COLOR}

layout( std140, binding=0 ) uniform scene_buf
{
    rt_scene scene;
};

layout( std140, binding=1 ) uniform sphere_buf
{
	#if SPHERE_SIZE != 0
    rt_sphere spheres[SPHERE_SIZE];
	#else
	rt_sphere spheres[1];
	#endif
};

layout( std140, binding=2 ) uniform plains_buf
{
	#if PLAIN_SIZE != 0
    rt_plain plains[PLAIN_SIZE];
	#else
	rt_plain plains[1];
	#endif
};

layout( std140, binding=3 ) uniform lights_point_buf
{
	#if LIGHT_POINT_SIZE != 0
    rt_light_point lights_point[LIGHT_POINT_SIZE];
	#else
	rt_light_point lights_point[1];
	#endif
};

layout( std140, binding=4 ) uniform lights_direct_buf
{
	#if LIGHT_DIRECT_SIZE != 0
    rt_light_direct lights_direct[LIGHT_DIRECT_SIZE];
	#else
	rt_light_direct lights_direct[1];
	#endif
};

#if DBG
bool dbgEd = false;
#endif

void _dbg()
{
	#if DBG
	ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
    gl_FragColor =vec4(1,0,0,1);
	dbgEd = true;
	#endif
}

void _dbg(float value)
{
	#if DBG
	value = clamp(value, 0, 1);
	ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
    gl_FragColor = vec4(value,value,value,1);
	dbgEd = true;
	#endif
}

void _dbg(vec3 value)
{
	#if DBG
	ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
    gl_FragColor = vec4(clamp(value, vec3(0), vec3(1)),1);
	dbgEd = true;
	#endif
}

const int iterations = 5;
const float maxDist = 1000000.0;
const vec3 amb = vec3(1.0);
const float eps = 0.001;

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
	vec4 qv = vec4(v, 0);

	vec4 mult = multiplyQuaternion(q, qv);
	float scale = 1 / (q.w * q.w + dot(q, q));
	vec4 inverse = - scale * q;
	inverse.w = scale * q.w;
	vec3 result = vec3(multiplyQuaternion(mult, inverse));

	return result;
}

vec3 getRayDir(vec2 pixel_coords)
{
	vec3 result = vec3((pixel_coords - vec2(scene.canvas_width, scene.canvas_height) / 2) / scene.canvas_height, 1);
	return normalize(Rotate(scene.quat_camera_rotation, result));
}

bool intersectSphere(vec3 ro, vec3 rd, vec4 sp, float tm, out float t)
{
    bool r = false;
	vec3 v = ro - sp.xyz;
	float b = dot(v,rd);
	float c = dot(v,v) - sp.w*sp.w;
	t = b*b-c;
    if( t > 0.0 )
    {
		float sqrt_ = sqrt(t);
		t = -b - sqrt_;
		#if MULTI_INTERSECTION
		if (t < 0.0) t = - b + sqrt_;
		#endif
		r = (t > 0.0) && (t < tm);
    } 
    return r;
}

bool intersectPlain(vec3 ro, vec3 rd, vec3 n, vec3 p, float tm, out float t) {
	float denom = clamp(dot(n, rd), -1, 1); 
	#ifdef PLANE_ONESIDE
		if (denom < -1e-6) 
	#else
    	if (abs(denom) > 1e-6) 
	#endif
	{
        vec3 p_ro = p - ro; 
        t = dot(p_ro, n) / denom; 
        return (t > 0) && (t < tm);
    } 
 
    return false; 
}


float calcInter(vec3 ro, vec3 rd, out int num, out int type)
{
	float tm = maxDist;
	float t;
	for (int i = 0; i < PLAIN_SIZE; ++i) {
		if (intersectPlain(ro,rd, plains[i].normal,plains[i].pos,tm,t)) {
			num = i; tm = t; type = 2;
		}
	}
	for (int i = 0; i < SPHERE_SIZE; ++i) {
		if (intersectSphere(ro,rd, spheres[i].obj,tm,t)) {
			num = i; tm = t; type = 1;
		}
	}
	
 	return tm;
}

bool inShadow(vec3 ro,vec3 rd,float d)
{
	bool ret = false;
	float t;
	
	for (int i = 0; i < SPHERE_SIZE; ++i)
		if(intersectSphere(ro,rd,spheres[i].obj,d,t)) {ret = true;}
	#if PLANE_ONESIDE == 0
	for (int i = 0; i < PLAIN_SIZE; ++i)
		if(intersectPlain(ro,rd, plains[i].normal,plains[i].pos,d,t)) {ret = true;}
	#endif


	return ret;
}

void calcShade2(vec3 l, vec3 lcol, float intensity, vec3 pt, vec3 rd, vec3 col, float albedo, vec3 n, float specPower, bool doShadow, float dist, float distDiv, inout vec3 diffuse, inout vec3 specular) {
	l = normalize(l);
	// diffuse
	float dp = clamp(dot(n, l), 0.0, 1.0);
	lcol *= dp;
	#if SHADOW_ENABLED
	if (doShadow) 
		lcol *= inShadow(pt, l, dist) ? 0 : 1;
	#endif
	diffuse += lcol * col * albedo * intensity / distDiv;
	
	//specular
	if (specPower > 0) {
		vec3 reflection = reflect(l, n);
		float specDp = clamp(dot(rd, reflection), 0.0, 1.0);
		specular += lcol * pow(specDp, specPower) * intensity / distDiv;
	}
}

vec3 calcShade (vec3 pt, vec3 rd, vec3 col, float albedo, vec3 n, float specPower, bool doShadow, float kd, float ks)
{
	float dist, distDiv;
	vec3 lcol,l;
	vec3 diffuse = vec3(0);
	vec3 specular = vec3(0);

	vec3 pixelColor = AMBIENT_COLOR * col;

	//TODO:  разделить свет на 3 типа a, p, d  - каждый в своем буффере, итерировать по буфферу

	for (int i = 0; i < LIGHT_POINT_SIZE; i++) {
		lcol = lights_point[i].color;
		l = lights_point[i].pos.xyz - pt;
		dist = length(l);
		distDiv = 1 + dist*dist;

		calcShade2(l, lcol, lights_point[i].intensity, pt, rd, col, albedo, n, specPower, doShadow, dist, distDiv, diffuse, specular);
	}
	for (int i = 0; i < LIGHT_DIRECT_SIZE; i++) {
		lcol = lights_direct[i].color;
		l = - lights_direct[i].direction;
		dist = maxDist;
		distDiv = 1;

		calcShade2(l, lcol, lights_direct[i].intensity, pt, rd, col, albedo, n, specPower, doShadow, dist, distDiv, diffuse, specular);
	}
	pixelColor += diffuse * kd + specular * ks;
	return pixelColor;
}

float getFresnel(vec3 n,vec3 rd,float r0)
{
    float ndotv = clamp(dot(n, -rd), 0.0, 1.0);
	return r0 + (1.0 - r0) * pow(1.0 - ndotv, 5.0);
}

float FresnelReflectAmount (float n1, float n2, vec3 normal, vec3 incident, float refl)
{
    #if DO_FRESNEL
        // Schlick aproximation
        float r0 = (n1-n2) / (n1+n2);
        r0 *= r0;
        float cosX = -dot(normal, incident);
        if (n1 > n2)
        {
            float n = n1/n2;
            float sinT2 = n*n*(1.0-cosX*cosX);
            // Total internal reflection
            if (sinT2 > 1.0)
                return 1.0;
            cosX = sqrt(1.0-sinT2);
        }
        float x = 1.0-cosX;
        float ret = r0+(1.0-r0)*x*x*x*x*x;

        // adjust reflect multiplier for object reflectivity
        ret = (refl + (1.0-refl) * ret);
        return ret;
    #else
    	return refl;
    #endif
}

hit_record get_hit_info(vec3 pt, int num, int type) {
	hit_record hr;
	if (type == 1) {
		hr = hit_record(spheres[num].mat, normalize(pt - spheres[num].obj.xyz));
	}
	if (type == 2) {
		hr = hit_record(plains[num].mat, normalize(plains[num].normal));
	}
	return hr;
}

vec3 getReflection(vec3 ro,vec3 rd)
{
	vec3 color = vec3(0);
	vec3 pt;
	int num, type;
	float tm = calcInter(ro,rd,num,type);
	hit_record hr;
	if(tm < maxDist) {
		pt = ro + rd * tm;
		hr = get_hit_info(pt, num, type);
		color = calcShade(dot(rd, hr.n) < 0 ? pt + hr.n * eps : pt - hr.n * eps, rd, hr.mat.color, hr.mat.diffuse, hr.n, hr.mat.specular, true, hr.mat.kd, hr.mat.ks);
	}
	return color;
}

void swap(inout float a, inout float b)
{
	float tmp = a;
	a = b;
	b = tmp;
}

vec3 refract_c(vec3 I, vec3 N, float ior)
{
	float cosi = clamp(dot(I, N), -1, 1);
	float etai = 1, etat = ior;
	vec3 n = N;
	if (cosi < 0) { cosi = -cosi; }
	else { swap(etai, etat); n = -N; }
	float eta = etai / etat;
	float k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0.0 ? vec3(0.0) : eta * I + (eta * cosi - sqrt(k)) * n;
}

// vec3 getNormal(int type, vec4 ob, vec3 pt) {
// 	vec3 result;
// 	if (type == 1) {
// 		result = normalize(pt - ob.xyz);
// 	}
// 	return result;
// }



void main()
{
	vec2 pixel_coords = gl_FragCoord.xy;
	if (pixel_coords.x >= scene.canvas_width || pixel_coords.y >= scene.canvas_height){
		return;
	}
	#if AIM_ENABLED
	if (pixel_coords == vec2(scene.canvas_width / 2, scene.canvas_height / 2))
	{
		gl_FragColor = vec4(1);
		return;
	}
	#endif

	float reflectMultiplier,refractMultiplier,tm;
	vec3 col;
    rt_material mat;
	vec3 pt,refCol,n,refl;
	vec4 ob;

	vec3 mask = vec3(1.0);
	vec3 color = vec3(0.0);
	vec3 ro = vec3(scene.camera_pos);
	vec3 rd = getRayDir(pixel_coords);
	float absorbDistance = 0.0;
	int type = 0;
	int num;
	hit_record hr;
	
	for(int i = 0; i < iterations; i++)
	{
		tm = calcInter(ro,rd,num,type);
		if(tm < maxDist)
		{
			pt = ro + rd*tm;
			hr = get_hit_info(pt, num, type);
			col = hr.mat.color;
			mat= hr.mat;
			n = hr.n;

			bool outside = dot(rd, n) < 0;
			n = outside ? n : -n;

			#if TOTAL_INTERNAL_REFLECTION
			//float reflIdx = mat.y > 0 ? mat.y : REFRACTIVE_INDEX_AIR;
			if (mat.y > 0) 
				reflectMultiplier = FresnelReflectAmount( outside ? 1 : mat.y,
													  	  outside ? mat.y : 1,
											 		      rd, n, mat.x);
			else reflectMultiplier = getFresnel(n,rd,mat.x);
			#else
			reflectMultiplier = getFresnel(n,rd,mat.x);
			#endif
			refractMultiplier = 1 - reflectMultiplier;

			if(mat.y > 0.0) // Refractive
			{
				if (outside && mat.x > 0)
				{
					//todo: если прозрачный объект будет внутри другого прозр.объекта, такое не прокатит (absorb = 0)
					//сделать многоуровневый absorbDistance
					absorbDistance = 0.0;
					refl = reflect(rd, n);
					refCol = getReflection(pt + n * eps, refl);
					color += refCol * reflectMultiplier * mask;
					mask *= refractMultiplier;
				}
				else if (!outside) {
					absorbDistance += tm;    
        			vec3 absorb = exp(-mat.absorb * absorbDistance);
					mask *= absorb;
				}
				#if TOTAL_INTERNAL_REFLECTION
				//todo: не делать break, вместо этого сделать rd = reflect(..)
				if (reflectMultiplier >= 1)
					break;
				#endif
				ro = pt - n * eps;
				#if CUSTOM_REFRACT
				rd = refract_c(rd, outside ? n : -n, mat.y);
				#else
				rd = refract(rd, n, outside ? 1 / mat.y : mat.y);
				#endif
			}
			else 
			if(mat.x > 0.0) // Reflective
			{
				ro = pt + n * eps;
				color += calcShade(ro, rd, col, mat.diffuse, n, mat.specular, true, mat.kd, mat.ks)* refractMultiplier * mask;
				rd = reflect(rd, n);
				mask *= reflectMultiplier;
			}
			else // Diffuse
            {
				color += calcShade(pt + n * eps, rd, col, mat.diffuse, n, mat.specular, true, mat.kd, mat.ks) * mask;
                break;
            }
			
		} 
		else {
			color += scene.bg_color * mask;
			break;
		}
	}
	#if DBG == 0
	gl_FragColor = vec4(color,1);
	#else
	if (!dbgEd) gl_FragColor = vec4(0);
	#endif
}
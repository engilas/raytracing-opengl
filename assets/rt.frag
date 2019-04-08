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

struct rt_light {
	vec3 pos;
	vec3 direction;
	vec3 color;

	int type;
	float intensity;
	float radius;
};

struct rt_scene {
	vec4 quat_camera_rotation;//uniform
	vec3 camera_pos;//uniform
	vec3 bg_color;//uniform

	int canvas_width;// uniform
	int canvas_height;// uniform

	float viewport_width;//remove
	float viewport_height;//remove
	float viewport_dist;//remove

	int reflect_depth;//define
};

struct hit_record {
	rt_material mat;
  	//vec3 pt;
  	vec3 n;
  	//float t;
};

// #define SPHERE_SIZE {SPHERE_SIZE}
// #define PLAIN_SIZE {PLAIN_SIZE}
// #define LIGHT_SIZE {LIGHT_SIZE}
#define SPHERE_SIZE 1
#define PLAIN_SIZE 6
#define LIGHT_SIZE 1

layout( std140, binding=0 ) uniform scene_buf
{
    rt_scene scene;
};

// layout( std140, binding=1 ) uniform sphere_buf
// {
//     rt_sphere spheres[SPHERE_SIZE];
// };
layout( std140, binding=1 ) uniform sphere_buf
{
    rt_sphere spheres[SPHERE_SIZE];
};

//uniform rt_sphere[SPHERE_SIZE] spheres;
//uniform vec4[SPHERE_SIZE] spheres_;

// layout( std430, binding=2 ) readonly buffer spheres_buf
// {
//     rt_sphere spheress[ ];
// };

layout( std140, binding=2 ) uniform plains_buf
{
    rt_plain plains[PLAIN_SIZE];
};

layout( std140, binding=3 ) uniform lights_buf
{
    rt_light lights[LIGHT_SIZE];
};

//uniform readonly int sphere_count;

// layout(binding=5) uniform UBO {
// 	int sphere_count;
// };

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

vec3 getRayDir(ivec2 pixel_coords)
{
	int x = int (pixel_coords.x - scene.canvas_width / 2.0);
	int y = int (pixel_coords.y - scene.canvas_height / 2.0);

	vec3 result = vec3(
		x * scene.viewport_width / scene.canvas_width,
	 	y * scene.viewport_height / scene.canvas_height,
	 	scene.viewport_dist);
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

#define light_counts 1

vec3 LightPixel2 (vec3 pt, vec3 rd, vec3 col, float albedo, vec3 n, float specPower, bool doShadow, float kd, float ks)
{
	float dist, distDiv;
	vec3 lcol,l;
	vec3 diffuse = vec3(0);
	vec3 specular = vec3(0);

	vec3 pixelColor = vec3(0);
	//return vec3(1);
	//if(diffuse > 0.0) //If its not a light

	//TODO:  разделить свет на 3 типа a, p, d  - каждый в своем буффере, итерировать по буфферу
	{
		for (int i = 0; i < LIGHT_SIZE; i++) {
			lcol = lights[i].color;

			if (lights[i].type == LIGHT_AMBIENT) {
				pixelColor += lights[i].intensity * col * lcol;
				continue;
			}
			if (lights[i].type == LIGHT_POINT) {
				l = lights[i].pos - pt;
				dist = length(l);
				distDiv = 1 + dist*dist;
			} 
		 	if (lights[i].type == LIGHT_DIRECT) {
			 	l = - lights[i].direction;
			  	dist = maxDist;
			  	distDiv = 1;
			}

			l = normalize(l);
			
			// diffuse
			float dp = clamp(dot(n, l), 0.0, 1.0);
			lcol *= dp;
			#if SHADOW_ENABLED
			if (doShadow) 
				lcol *= inShadow(pt, l, dist) ? 0 : 1;
			#endif
			diffuse += lcol * col * albedo * 35 / distDiv;
			
			//specular
			if (specPower > 0) {
				vec3 reflection = reflect(l, n);
				float specDp = clamp(dot(rd, reflection), 0.0, 1.0);
				specular += lcol * pow(specDp, specPower) * 15 / distDiv;
			}
		}
		pixelColor = diffuse * kd + specular * ks;
	} //else return col;
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

vec3 getReflection(vec3 ro,vec3 rd)
{
	vec3 color = vec3(0);
	vec3 col,pt,n;
    vec2 mat;
	vec4 ob;
	// if(calcInter(ro,rd,ob,col,mat))
	// {
	// 	bool outside = dot(rd, n) < 0;
	// 	color = LightPixel2(outside ? pt + n * eps : pt - n * eps,rd,col,0.7,outside?n:-n,30, true, 0.8, 0.2);
	// }
	// return color;

	// float tm = calcInter(ro,rd,ob,col,mat);
	// if(tm < maxDist)
	// {
	// 	vec3 pt = ro + rd*tm;
	// 	vec3 n = normalize(pt - ob.xyz);
	// 	bool outside = dot(rd, n) < 0;
	// 	color = calcShade(outside ? pt + n * eps : pt - n * eps,ob,col,mat,n);
	// }
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

void main()
{
	ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
	if (pixel_coords.x >= scene.canvas_width || pixel_coords.y >= scene.canvas_height){
		return;
	}
	#if AIM_ENABLED
	if (pixel_coords == ivec2(scene.canvas_width / 2, scene.canvas_height / 2))
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
	
	for(int i = 0; i < iterations; i++)
	{
		hit_record hr;
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
				reflectMultiplier = FresnelReflectAmount( outside ? REFRACTIVE_INDEX_AIR : mat.y,
													  	  outside ? mat.y : REFRACTIVE_INDEX_AIR,
											 		      rd, n, mat.x);
			else reflectMultiplier = getFresnel(n,rd,mat.x);
			#else
			reflectMultiplier = getFresnel(n,rd,mat.x);
			#endif
			refractMultiplier = 1 - reflectMultiplier;

			if(mat.y > 0.0) // Refractive
			{
				if (outside)
				{
					//todo: если прозрачный объект будет внутри другого прозр.объекта, такое не прокатит (absorb = 0)
					//сделать многоуровневый absorbDistance
					absorbDistance = 0.0;
					refl = reflect(rd, n);
					refCol = getReflection(outside ? pt + n * eps : pt - n * eps, refl);
					color += refCol * reflectMultiplier * mask;
					mask *= refractMultiplier;
				}
				#define absorba vec3(5,4,3)
				#define mat_diff 0.7
				else {
					absorbDistance += tm;    
        			vec3 absorb = exp(-absorba * absorbDistance);
					mask *= absorb;
				}
				#if TOTAL_INTERNAL_REFLECTION
				//todo: не делать break, вместо этого сделать rd = reflect(..)
				if (reflectMultiplier >= 1)
				 	break;
				#endif
				ro = outside ? pt - n * eps : pt + n * eps;
				#if CUSTOM_REFRACT
				rd = refract_c(rd, n, mat.y);
				#else
				float refractCoeff = outside ? REFRACTIVE_INDEX_AIR / mat.y : mat.y / REFRACTIVE_INDEX_AIR;
				rd = refract(rd, outside ? n : -n, refractCoeff);
				#endif
			}
			else 
			if(mat.x > 0.0) // Reflective
			{
				ro = pt + n * eps;
				color += LightPixel2(ro, rd, col, mat.diffuse, n, mat.specular, true, mat.kd, mat.ks)* refractMultiplier * mask;
				rd = reflect(rd, n);
				mask *= reflectMultiplier;
			}
			else // Diffuse
            {
				color += LightPixel2(pt + n * eps, rd, col, mat.diffuse, n, mat.specular, true, mat.kd, mat.ks) * mask;
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
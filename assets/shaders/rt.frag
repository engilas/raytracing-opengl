#version 330 core

#define FLT_MIN 1.175494351e-38
#define FLT_MAX 3.402823466e+38
#define PI_F 3.14159265358979f

#define TYPE_SPHERE 0
#define TYPE_PLANE 1
#define TYPE_SURFACE 2
#define TYPE_BOX 3
#define TYPE_TORUS 4
#define TYPE_RING 5
#define TYPE_POINT_LIGHT 6

#define SHADOW_ENABLED 1
#define DBG 0
#define DBG_FIRST_VALUE 1

#define TOTAL_INTERNAL_REFLECTION 1
#define DO_FRESNEL 1
#define PLANE_ONESIDE 1
#define REFLECT_REDUCE_ITERATION 1

struct rt_material {
	vec3 color;
	vec3 absorb;

	float diffuse;
	float reflection;
	float refraction;
	int specular;
	float kd;
	float ks;
};

struct rt_sphere {
	rt_material mat;
	vec4 obj;
	vec4 quat_rotation; // rotate normal
	int textureNum;
	bool hollow;
};

struct rt_plane {
	rt_material mat;
	vec3 pos;
	vec3 normal;
};

struct rt_box {
	rt_material mat;
	vec4 quat_rotation;
	vec3 pos;
	vec3 form;
	int textureNum;
};

struct rt_ring {
	rt_material mat;
	vec4 quat_rotation;
	vec3 pos;
	int textureNum;
	float r1; // square of min radius
	float r2; // square of max radius
};

struct rt_surface {
	rt_material mat;
	vec4 quat_rotation;
	vec3 v_min;
	vec3 v_max;
	vec3 pos;
	float a; // x2
	float b; // y2
	float c; // z2
	float d; // z
	float e; // y
	float f; // const	
};

struct rt_torus {
	rt_material mat;
	vec4 quat_rotation;
	vec3 pos;
	vec2 form; // x - radius, y - ring thickness
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

	float linear_k;
	float quadratic_k;
};

struct rt_scene {
	vec4 quat_camera_rotation;
	vec3 camera_pos;
	vec3 bg_color;

	int canvas_width;
	int canvas_height;

	int reflect_depth;
};

struct hit_record {
	rt_material mat;
  	vec3 normal;
	float bias_mult;
	float alpha;
};

#define SPHERE_SIZE {SPHERE_SIZE}
#define PLANE_SIZE {PLANE_SIZE}
#define SURFACE_SIZE {SURFACE_SIZE}
#define BOX_SIZE {BOX_SIZE}
#define TORUS_SIZE {TORUS_SIZE}
#define RING_SIZE {RING_SIZE}
#define LIGHT_DIRECT_SIZE {LIGHT_DIRECT_SIZE}
#define LIGHT_POINT_SIZE {LIGHT_POINT_SIZE}
#define AMBIENT_COLOR {AMBIENT_COLOR}
#define SHADOW_AMBIENT {SHADOW_AMBIENT}
#define ITERATIONS {ITERATIONS}

out vec4 FragColor;

uniform samplerCube skybox;

uniform sampler2D texture_sphere_1;
uniform sampler2D texture_sphere_2;
uniform sampler2D texture_sphere_3;
uniform sampler2D texture_sphere_4;
uniform sampler2D texture_ring;
uniform sampler2D texture_box;

const float maxDist = 1000000.0;

// if attributes calculations can be processed with intersection search (box, disk)
vec3 opt_normal;
vec2 opt_uv;

#if DBG
bool dbgEd = false;
#endif

layout( std140 ) uniform scene_buf
{
    rt_scene scene;
};

layout( std140 ) uniform spheres_buf
{
	#if SPHERE_SIZE != 0
    rt_sphere spheres[SPHERE_SIZE];
	#else
	rt_sphere spheres[1];
	#endif
};

layout( std140 ) uniform planes_buf
{
	#if PLANE_SIZE != 0
    rt_plane planes[PLANE_SIZE];
	#else
	rt_plane planes[1];
	#endif
};

layout( std140 ) uniform surfaces_buf
{
	#if SURFACE_SIZE != 0
    rt_surface surfaces[SURFACE_SIZE];
	#else
	rt_surface surfaces[1];
	#endif
};

layout( std140 ) uniform boxes_buf
{
	#if BOX_SIZE != 0
    rt_box boxes[BOX_SIZE];
	#else
	rt_box boxes[1];
	#endif
};

layout( std140 ) uniform toruses_buf
{
	#if TORUS_SIZE != 0
    rt_torus toruses[TORUS_SIZE];
	#else
	rt_torus toruses[1];
	#endif
};

layout( std140 ) uniform rings_buf
{
	#if RING_SIZE != 0
    rt_ring rings[RING_SIZE];
	#else
	rt_ring rings[1];
	#endif
};

layout( std140 ) uniform lights_point_buf
{
	#if LIGHT_POINT_SIZE != 0
    rt_light_point lights_point[LIGHT_POINT_SIZE];
	#else
	rt_light_point lights_point[1];
	#endif
};

layout( std140 ) uniform lights_direct_buf
{
	#if LIGHT_DIRECT_SIZE != 0
    rt_light_direct lights_direct[LIGHT_DIRECT_SIZE];
	#else
	rt_light_direct lights_direct[1];
	#endif
};

void _dbg()
{
	#if DBG
	#if DBG_FIRST_VALUE 
	if (dbgEd)
		return;
	#endif
	
	ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
    FragColor =vec4(1,0,0,1);
	dbgEd = true;
	#endif
}

void _dbg(float value)
{
	#if DBG
	#if DBG_FIRST_VALUE 
	if (dbgEd)
		return;
	#endif
	value = clamp(value, 0, 1);
	ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
    FragColor = vec4(value,value,value,1);
	dbgEd = true;
	#endif
}

void _dbg(vec3 value)
{
	#if DBG
	#if DBG_FIRST_VALUE 
	if (dbgEd)
		return;
	#endif
	ivec2 pixel_coords = ivec2 (gl_FragCoord.xy);
    FragColor = vec4(clamp(value, vec3(0), vec3(1)),1);
	dbgEd = true;
	#endif
}

void swap(inout float a, inout float b)
{
	float tmp = a;
	a = b;
	b = tmp;
}

bool isBetween(vec3 value, vec3 min, vec3 max) 
{
	return greaterThan(value, min) == bvec3(true) && lessThan(value, max) == bvec3(true);
}

vec4 quat_conj(vec4 q)
{ 
  	return vec4(-q.x, -q.y, -q.z, q.w); 
}
  
vec4 quat_inv(vec4 q)
{ 
  	return quat_conj(q) * (1 / dot(q, q));
}

vec4 quat_mult(vec4 q1, vec4 q2)
{ 
	vec4 qr;
	qr.x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y);
	qr.y = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x);
	qr.z = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w);
	qr.w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z);
	return qr;
}

vec3 rotate(vec4 qr, vec3 v)
{ 
	vec4 qr_conj = quat_conj(qr);
	vec4 q_pos = vec4(v.xyz, 0);
	vec4 q_tmp = quat_mult(qr, q_pos);
	return quat_mult(q_tmp, qr_conj).xyz;
}

vec3 getRayDir()
{
	vec3 result = vec3((gl_FragCoord.xy - vec2(scene.canvas_width, scene.canvas_height) / 2) / scene.canvas_height, 1);
	return normalize(rotate(scene.quat_camera_rotation, result));
}

vec4 getSphereTexture(vec3 sphereNormal, vec4 quat, int texNum) {
	if (quat != vec4(0,0,0,1)) {
		sphereNormal = rotate(quat, sphereNormal);
	}
	float u = 0.5 + atan(sphereNormal.z, sphereNormal.x) / (2.*PI_F);
	float v = 0.5 - asin(sphereNormal.y) / PI_F;
	vec2 uv = vec2(u, v);
	vec2 df = fwidth(uv);
	if(df.x > 0.5) df.x = 0.;

	vec4 color;
	if (texNum == 1) {
		color = textureLod(texture_sphere_1, uv, log2(max(df.x, df.y)*1024.));
	}
	if (texNum == 2) {
		color = textureLod(texture_sphere_2, uv, log2(max(df.x, df.y)*1024.));
	}
	if (texNum == 3) {
		color = textureLod(texture_sphere_3, uv, log2(max(df.x, df.y)*1024.));
	}
	return color;
}

bool intersectSphere(vec3 ro, vec3 rd, vec4 object, bool hollow, float tmin, out float t)
{
    vec3 oc = ro - object.xyz;
	float b = dot( oc, rd );
	float c = dot( oc, oc ) - object.w*object.w;
	float h = b*b - c;
	if( h<0.0 ) return false;
	float h_sqrt = sqrt(h);
	t = -b - h_sqrt;
	if (hollow && t < 0.0) 
		t = -b + h_sqrt;
	return t > 0 && t < tmin;
}

bool intersectPlane(vec3 ro, vec3 rd, vec3 n, vec3 p, float tmin, out float t) {
	float denom = clamp(dot(n, rd), -1, 1); 
	#ifdef PLANE_ONESIDE
		if (denom < -1e-6) 
	#else
    	if (abs(denom) > 1e-6) 
	#endif
	{
        vec3 p_ro = p - ro; 
        t = dot(p_ro, n) / denom; 
        return (t > 0) && (t < tmin);
    } 
 
    return false; 
}

bool intersectRing(vec3 ro, vec3 rd, int num, float tmin, out float t) {
	rt_ring ring = rings[num];
	rd = rotate(ring.quat_rotation, rd);
	ro = rotate(ring.quat_rotation, ro - ring.pos);

	t = -ro.z / rd.z;

	float x = ro.x + rd.x * t;
	float y = ro.y + rd.y * t;

	float p = x * x + y * y;

	if (t > 0 && t < tmin && p < ring.r2 && p > ring.r1) {
		float cosv = dot(normalize(vec2(x, y)), vec2(1, 0));
		opt_uv = vec2((p - ring.r1) / (ring.r2 - ring.r1), cosv);
		return true;
	}
	return false;
}
vec3 getRingNormal(int num) {
	rt_ring ring = rings[num];
	return rotate(quat_inv(ring.quat_rotation), vec3(0, 0, -1));
}
vec4 getRingTexture(int num, vec2 uv) {
	return texture(texture_ring, uv);
}

bool intersectBox(vec3 ro, vec3 rd, int num, float tmin, out float t) 
{
	rt_box box = boxes[num];
    // convert from ray to box space
	vec3 rdd = rotate(box.quat_rotation, rd);
	vec3 roo = rotate(box.quat_rotation, ro - box.pos);

	// ray-box intersection in box space
    vec3 m = 1.0/rdd;
    vec3 n = m*roo;
    vec3 k = abs(m)*box.form;
	
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;

	float tN = max( max( t1.x, t1.y ), t1.z );
	float tF = min( min( t2.x, t2.y ), t2.z );
	
	if( tN > tF || tF < 0.0) return false;
    
	if (tN >= tmin)
		return false;

	vec3 nor = -sign(rdd)*step(t1.yzx,t1.xyz)*step(t1.zxy,t1.xyz);
	t = tN;
	// convert to ray space
	opt_normal = rotate(quat_inv(box.quat_rotation), nor);
	return true;
}
vec4 getBoxTexture(vec3 pt, vec3 normal, int num) {
	rt_box box = boxes[num];
	vec3 pos = rotate(box.quat_rotation, box.pos);
	pt = rotate(box.quat_rotation, pt);
	normal = rotate(box.quat_rotation, normal);
	return abs(normal.x)*texture(texture_box, 0.5*(pt.zy - pos.zy)-vec2(0.5)) + 
			abs(normal.y)*texture(texture_box, 0.5*(pt.zx - pos.zx)-vec2(0.5)) + 
			abs(normal.z)*texture(texture_box, 0.5*(pt.xy - pos.xy)-vec2(0.5));
}

// begin torus section
vec2 cmul(vec2 c1, vec2 c2){
	return vec2(c1.x*c2.x-c1.y*c2.y,c1.x*c2.y+c1.y*c2.x);
}
vec2 cinv(vec2 c){
	return vec2(c.x,-c.y)/dot(c,c);
}
vec2 cTorus(vec2 t, in vec3 ro, in vec3 rd, in vec2 torus ){
// f(t) = (tÂ²*rdÂ²+2*t*ro*rd+roÂ²+RÂ²-rÂ²)Â²-4*RÂ²*(tÂ²*rdxyÂ²+2*t*roxy*rdxy+roxyÂ²)
	float R2=torus.x*torus.x;
	float r2=torus.y*torus.y;
	vec2 t2=vec2(t.x*t.x-t.y*t.y,2.*t.x*t.y);//cmul(t,t);
	vec2 res= t2*dot(rd,rd)+2.*t*dot(ro,rd)+vec2(dot(ro,ro)+R2-r2,0.);
	res=cmul(res,res);
	vec2 res2=4.*R2*(t2*dot(rd.xy,rd.xy)+2.*t*dot(ro.xy,rd.xy)+vec2(dot(ro.xy,ro.xy),0.));
	
	return res-res2;
}
float DKstep(inout vec2 c0, vec2 c1, vec2 c2, vec2 c3, in vec3 ro, in vec3 rd, in vec2 torus){
	vec2 fc=cTorus(c0,ro,rd,torus);
	fc=cmul(fc,cinv(cmul(c0-c1,cmul(c0-c2,c0-c3))));
	c0-=fc;
	return max(abs(fc.x),abs(fc.y));
}
bool intersectTorus( in vec3 ro, in vec3 rd, int num, float tmin, out float t ){
	float eps = 0.001;
	rt_torus torus = toruses[num];
	ro = rotate(torus.quat_rotation, ro - torus.pos);
	rd = rotate(torus.quat_rotation, rd);
	vec2 c0=vec2(1.,0.);
	vec2 c1=vec2(0.4,0.9);
	vec2 c2=cmul(c1,vec2(0.4,0.9));
	vec2 c3=cmul(c2,vec2(0.4,0.9));
	for(int i=0; i<60; i++){
		float e = DKstep(c0, c1, c2, c3, ro, rd, torus.form);
		e = max(e,DKstep(c1, c2, c3, c0, ro, rd, torus.form));
		e = max(e,DKstep(c2, c3, c0, c1, ro, rd, torus.form));
		e = max(e,DKstep(c3, c0, c1, c2, ro, rd, torus.form));
		if(e<eps) break;
	}
	vec4 rs= vec4(c0.x, c1.x, c2.x, c3.x);
	vec4 ri= abs(vec4(c0.y, c1.y, c2.y, c3.y));

	if(ri.x>eps || rs.x<0.) rs.x=10000.;
	if(ri.y>eps || rs.y<0.) rs.y=10000.;
	if(ri.z>eps || rs.z<0.) rs.z=10000.;
	if(ri.w>eps || rs.w<0.) rs.w=10000.;
	t = min(min(rs.x,rs.y),min(rs.z,rs.w));
	return t > 0 && t < 100 && t < tmin;
}
vec3 getTorusNormal(vec3 ro, vec3 rd, float t, int num)
{
	rt_torus torus = toruses[num];
	ro = rotate(torus.quat_rotation, ro - torus.pos);
	rd = rotate(torus.quat_rotation, rd);
	vec3 pos = ro + rd * t;
	vec3 normal = pos*(dot(pos,pos)- torus.form.y*torus.form.y - torus.form.x*torus.form.x*vec3(1.0,1.0,-1.0));
	return normalize(rotate(quat_inv(torus.quat_rotation), normal));
}
// end torus section

// begin surface section
bool checkSurfaceEdges(vec3 o, vec3 d, inout float tMin, inout float tMax, vec3 v_min, vec3 v_max, float epsilon)
{
	vec3 pt = d * tMin + o;
	if (!isBetween(pt, v_min, v_max)) 
	{
		if (tMax < epsilon) return false;
		pt = d * tMax + o;
		if (!isBetween(pt, v_min, v_max))
			return false;
		swap(tMin, tMax);
	}
	return true;
}
bool intersectSurface(vec3 ro, vec3 rd, int num, float tmin, out float t)
{
	vec3 orig_ro = ro;
	vec3 orig_rd = rd;
	rt_surface surface = surfaces[num];
	ro = rotate(surface.quat_rotation, ro - surface.pos);
	rd = rotate(surface.quat_rotation, rd);

	float a = surface.a;
	float b = surface.b;
	float c = surface.c;
	float d = surface.d;
	float e = surface.e;
	float f = surface.f;

	float d1 = rd.x;
	float d2 = rd.y;
	float d3 = rd.z;
	float o1 = ro.x;
	float o2 = ro.y;
	float o3 = ro.z;

	float p1 = 2 * a * d1 * o1 + 2 * b * d2 * o2 + 2 * c * d3 * o3 + d * d3 + d2 * e;
	float p2 = a * d1 * d1 + b * d2 * d2 + c * d3 * d3;
	float p3 = a * o1 * o1 + b * o2 * o2 + c * o3 * o3 + d * o3 + e * o2 + f;
	float p4 = sqrt(p1 * p1 - 4 * p2 * p3);

	//division by zero
	if (abs(p2) < 1e-6)
	{
		t = -p3 / p1;
		return t > tmin;
	}

	float min = FLT_MAX;
	float max = FLT_MAX;

	float t1 = (-p1 - p4) / (2 * p2);
	float t2 = (-p1 + p4) / (2 * p2);

	float epsilon = 1e-4;

	if (t1 > epsilon && t1 < min)
	{
		min = t1;
		max = t2;
	}

	if (t2 > epsilon && t2 < min)
	{
		min = t2;
		max = t1;
	}

	if (!checkSurfaceEdges(orig_ro, orig_rd, min, max, surface.v_min, surface.v_max, epsilon))
		return false;

	t = min;
	return t < tmin;
}
vec3 getSurfaceNormal(vec3 ro, vec3 rd, float t, int num) {
	rt_surface surface = surfaces[num];
	ro = ro - surface.pos;
	ro = rotate(surface.quat_rotation, ro);
	rd = rotate(surface.quat_rotation, rd);

	vec3 tm = rd * t + ro;

	vec3 normal = vec3(2 * surface.a * tm.x, 2 * surface.b * tm.y + surface.e, 2 * surface.c * tm.z + surface.d);
	normal = rotate(quat_inv(surface.quat_rotation), normal);
	return normalize(normal);
}
// end surface section

float calcInter(vec3 ro, vec3 rd, out int num, out int type)
{
	float tmin = maxDist;
	float t;
	for (int i = 0; i < PLANE_SIZE; i++) {
		if (intersectPlane(ro, rd, planes[i].normal, planes[i].pos, tmin, t)) {
			num = i; tmin = t; type = TYPE_PLANE;
		}
	}
	for (int i = 0; i < SPHERE_SIZE; i++) {
		if (intersectSphere(ro, rd, spheres[i].obj, spheres[i].hollow, tmin, t)) {
			num = i; tmin = t; type = TYPE_SPHERE;
		}
	}
	for (int i = 0; i < SURFACE_SIZE; i++) {
		if (intersectSurface(ro, rd, i, tmin, t)) {
			num = i; tmin = t; type = TYPE_SURFACE;
		}
	}
	for (int i = 0; i < BOX_SIZE; i++) {
		if (intersectBox(ro, rd, i, tmin, t)) {
			num = i; tmin = t; type = TYPE_BOX;
		}
	}
	for (int i = 0; i < TORUS_SIZE; i++) {
		if (intersectTorus(ro, rd, i, tmin, t)) {
			num = i; tmin = t; type = TYPE_TORUS;
		}
	}
	for (int i = 0; i < RING_SIZE; i++) {
		if (intersectRing(ro, rd, i, tmin, t)) {
			num = i; tmin = t; type = TYPE_RING;
		}
	}
	for (int i = 0; i < LIGHT_POINT_SIZE; i++) {
		if (intersectSphere(ro, rd, lights_point[i].pos, false, tmin, t)) {
			num = i; tmin = t; type = TYPE_POINT_LIGHT;
		}
	}
	
 	return tmin;
}

float inShadow(vec3 ro, vec3 rd, float dist)
{
	float t;
	float shadow = 0;
	
	for (int i = 0; i < SPHERE_SIZE; i++)
		if(intersectSphere(ro, rd, spheres[i].obj, false, dist, t)) {shadow = 1;}
	for (int i = 0; i < SURFACE_SIZE; i++)
		if(intersectSurface(ro, rd, i, dist, t)) {shadow = 1;}
	for (int i = 0; i < BOX_SIZE; i++)
		if(intersectBox(ro, rd, i, dist, t)) {shadow = 1;}
	for (int i = 0; i < TORUS_SIZE; i++)
		if(intersectTorus(ro, rd, i, dist, t)) {shadow = 1;}
	for (int i = 0; i < RING_SIZE; i++)
		if(intersectRing(ro, rd, i, dist, t)) {
			rt_ring ring = rings[i];
			if (ring.textureNum > 0) {
				shadow += getRingTexture(ring.textureNum, opt_uv).a;
			} else {
				shadow = 1;
			}
		}
	#if PLANE_ONESIDE == 0
	for (int i = 0; i < PLANE_SIZE; i++)
		if(intersectPlane(ro, rd, planes[i].normal, planes[i].pos, dist, t)) {shadow = 1;}
	#endif

	return min(shadow, 1);
}

void calcShade2(vec3 light_dir, vec3 light_color, float intensity, vec3 pt, vec3 rd, rt_material material, vec3 normal, bool doShadow, float dist, float distDiv, inout vec3 diffuse, inout vec3 specular) {
	light_dir = normalize(light_dir);
	// diffuse
	float dp = clamp(dot(normal, light_dir), 0.0, 1.0);
	light_color *= dp;
	#if SHADOW_ENABLED
	if (doShadow) {
		vec3 shadow = vec3(1 - inShadow(pt, light_dir, dist));
		light_color *= max(shadow, SHADOW_AMBIENT);
	}
	#endif
	diffuse += light_color * material.color * material.diffuse * intensity / distDiv;
	
	//specular
	if (material.specular > 0) {
		vec3 reflection = reflect(light_dir, normal);
		float specDp = clamp(dot(rd, reflection), 0.0, 1.0);
		specular += light_color * pow(specDp, material.specular) * intensity / distDiv;
	}
}

vec3 calcShade(vec3 pt, vec3 rd, rt_material material, vec3 normal, bool doShadow)
{
	float dist, distDiv;
	vec3 light_color, light_dir;
	vec3 diffuse = vec3(0);
	vec3 specular = vec3(0);

	vec3 pixelColor = AMBIENT_COLOR * material.color;

	for (int i = 0; i < LIGHT_POINT_SIZE; i++) {
		rt_light_point light = lights_point[i];
		light_color = light.color;
		light_dir = light.pos.xyz - pt;
		dist = length(light_dir);
		distDiv = 1 + light.linear_k * dist + light.quadratic_k * dist * dist;

		calcShade2(light_dir, light_color, light.intensity, pt, rd, material, normal, doShadow, dist, distDiv, diffuse, specular);
	}
	for (int i = 0; i < LIGHT_DIRECT_SIZE; i++) {
		light_color = lights_direct[i].color;
		light_dir = - lights_direct[i].direction;
		dist = maxDist;
		distDiv = 1;

		calcShade2(light_dir, light_color, lights_direct[i].intensity, pt, rd, material, normal, doShadow, dist, distDiv, diffuse, specular);
	}
	pixelColor += diffuse * material.kd + specular * material.ks;
	return pixelColor;
}

float getFresnel(vec3 normal, vec3 rd, float reflection)
{
    float ndotv = clamp(dot(normal, -rd), 0.0, 1.0);
	return reflection + (1.0 - reflection) * pow(1.0 - ndotv, 5.0);
}

float FresnelReflectAmount(float n1, float n2, vec3 normal, vec3 incident, float refl)
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

hit_record get_hit_info(vec3 ro, vec3 rd, vec3 pt, float t, int num, int type) {
	hit_record hr;
	if (type == TYPE_SPHERE) {
		rt_sphere sphere = spheres[num];
		hr = hit_record(sphere.mat, normalize(pt - sphere.obj.xyz), 0, 1);
		if (sphere.textureNum != 0) {
			vec4 texColor = getSphereTexture(hr.normal, sphere.quat_rotation, sphere.textureNum);
			hr.mat.color = texColor.rgb;
			hr.alpha = texColor.a;
		}
	}
	if (type == TYPE_PLANE) {
		hr = hit_record(planes[num].mat, normalize(planes[num].normal), 0, 1);
	}
	if (type == TYPE_SURFACE) {
		hr = hit_record(surfaces[num].mat, getSurfaceNormal(ro, rd, t, num), 0, 1);
	}
	if (type == TYPE_BOX) {
		rt_box box = boxes[num];
		hr = hit_record(box.mat, opt_normal, 0, 1);
		if (box.textureNum != 0) {
			hr.mat.color = getBoxTexture(pt, opt_normal, num).rgb;
		}
	}
	if (type == TYPE_TORUS) {
		hr = hit_record(toruses[num].mat, getTorusNormal(ro, rd, t, num), 0, 1);
	}
	if (type == TYPE_RING) {
		rt_ring ring = rings[num];
		hr = hit_record(ring.mat, getRingNormal(num), 0, 1);
		if (ring.textureNum != 0) {
			vec4 texColor = getRingTexture(ring.textureNum, opt_uv);
			hr.mat.color = texColor.rgb;
			hr.alpha = texColor.a;
		}
	}
	float distance = length(pt - ro);
	hr.bias_mult = (9e-3 * distance + 35) / 35e3;

	return hr;
}

// get one-step reflection color for refractive objects
vec3 getReflectedColor(vec3 ro, vec3 rd)
{
	vec3 color = vec3(0);
	vec3 pt;
	int num, type;
	float t = calcInter(ro, rd, num, type);
	if (type == TYPE_POINT_LIGHT) return lights_point[num].color;
	hit_record hr;
	if(t < maxDist) {
		pt = ro + rd * t;
		hr = get_hit_info(ro, rd, pt, t, num, type);
		ro = dot(rd, hr.normal) < 0 ? pt + hr.normal * hr.bias_mult : pt - hr.normal * hr.bias_mult;
		color = calcShade(ro, rd, hr.mat, hr.normal, true);
	}
	return color;
}

void main()
{
	float reflectMultiplier,refractMultiplier,tm;
	vec3 col;
    rt_material mat;
	vec3 pt,n;
	vec4 ob;

	vec3 mask = vec3(1.0);
	vec3 color = vec3(0.0);
	vec3 ro = vec3(scene.camera_pos);
	vec3 rd = getRayDir();
	float absorbDistance = 0.0;
	int type = 0;
	int num;
	hit_record hr;
	
	for(int i = 0; i < ITERATIONS; i++)
	{
		tm = calcInter(ro, rd, num, type);
		if(tm < maxDist)
		{
			pt = ro + rd*tm;
			hr = get_hit_info(ro, rd, pt, tm, num, type);

			if (type == TYPE_POINT_LIGHT) {
				color += lights_point[num].color * mask;
				break;
			}

			mat = hr.mat;
			n = hr.normal;

			bool outside = dot(rd, n) < 0;
			n = outside ? n : -n;

			#if TOTAL_INTERNAL_REFLECTION
			if (mat.refraction > 0) 
				reflectMultiplier = FresnelReflectAmount( outside ? 1 : mat.refraction,
													  	  outside ? mat.refraction : 1,
											 		      rd, n, mat.reflection);
			else reflectMultiplier = getFresnel(n,rd,mat.reflection);
			#else
			reflectMultiplier = getFresnel(n,rd,mat.reflection);
			#endif
			refractMultiplier = 1 - reflectMultiplier;

			if(mat.refraction > 0.0) // Refractive
			{
				if (outside && mat.reflection > 0)
				{
					color += getReflectedColor(pt + n * hr.bias_mult, reflect(rd, n)) * reflectMultiplier * mask;
					mask *= refractMultiplier;
				}
				else if (!outside) {
					absorbDistance += tm;    
        			vec3 absorb = exp(-mat.absorb * absorbDistance);
					mask *= absorb;
				}
				#if TOTAL_INTERNAL_REFLECTION
				//todo: rd = reflect(..) instead of break
				if (reflectMultiplier >= 1)
					break;
				#endif
				ro = pt - n * hr.bias_mult;
				rd = refract(rd, n, outside ? 1 / mat.refraction : mat.refraction);
				#ifdef REFLECT_REDUCE_ITERATION
				i--;
				#endif
			}
			else if(mat.reflection > 0.0) // Reflective
			{
				ro = pt + n * hr.bias_mult;
				color += calcShade(ro, rd, mat, n, true) * refractMultiplier * mask;
				rd = reflect(rd, n);
				mask *= reflectMultiplier;
			}
			else // Diffuse
			{
				color += calcShade(pt + n * hr.bias_mult, rd, mat, n, true) * mask * hr.alpha;
				if (hr.alpha < 1) {
					ro = pt - n * hr.bias_mult;
					mask *= 1 - hr.alpha;
				} else {
					break;
				}
			}
		} 
		else {
			color += texture(skybox, rd).rgb * mask;
			break;
		}
	}
	#if DBG == 0
	FragColor = vec4(color,1);
	#else
	if (!dbgEd) FragColor = vec4(color,1);
	#endif
}
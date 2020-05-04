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

#define TYPE_SPHERE 0
#define TYPE_PLANE 1
#define TYPE_SURFACE 2
#define TYPE_BOX 3
#define TYPE_TORUS 4
#define TYPE_POINT_LIGHT 5

#define SHADOW_ENABLED 1
#define DBG 0

#define TOTAL_INTERNAL_REFLECTION 1
#define DO_FRESNEL 1
#define AIM_ENABLED 1
#define PLANE_ONESIDE 1
#define REFLECT_REDUCE_ITERATION 1

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
	vec2 form; // x - ?, y - ?
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
#define PLANE_SIZE {PLANE_SIZE}
#define SURFACE_SIZE {SURFACE_SIZE}
#define BOX_SIZE {BOX_SIZE}
#define TORUS_SIZE {TORUS_SIZE}
#define LIGHT_DIRECT_SIZE {LIGHT_DIRECT_SIZE}
#define LIGHT_POINT_SIZE {LIGHT_POINT_SIZE}
#define AMBIENT_COLOR {AMBIENT_COLOR}
#define SHADOW_AMBIENT {SHADOW_AMBIENT}
#define ITERATIONS {ITERATIONS}

uniform samplerCube skybox;

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

layout( std140, binding=2 ) uniform planes_buf
{
	#if PLANE_SIZE != 0
    rt_plane planes[PLANE_SIZE];
	#else
	rt_plane planes[1];
	#endif
};

layout( std140, binding=5 ) uniform surfaces_buf
{
	#if SURFACE_SIZE != 0
    rt_surface surfaces[SURFACE_SIZE];
	#else
	rt_surface surfaces[1];
	#endif
};

layout( std140, binding=6 ) uniform boxes_buf
{
	#if BOX_SIZE != 0
    rt_box boxes[BOX_SIZE];
	#else
	rt_box boxes[1];
	#endif
};

layout( std140, binding=7 ) uniform toruses_buf
{
	#if TORUS_SIZE != 0
    rt_torus toruses[TORUS_SIZE];
	#else
	rt_torus toruses[1];
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

const float maxDist = 1000000.0;
const float eps = 0.001;

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

vec3 getRayDir(vec2 pixel_coords)
{
	vec3 result = vec3((pixel_coords - vec2(scene.canvas_width, scene.canvas_height) / 2) / scene.canvas_height, 1);
	return normalize(rotate(scene.quat_camera_rotation, result));
}

bool intersectSphere(vec3 ro, vec3 rd, vec4 sp, bool hollow, float tm, out float t)
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
		if (hollow && t < 0.0) 
			t = - b + sqrt_;
		r = (t > 0.0) && (t < tm);
    } 
    return r;
}

bool intersectPlane(vec3 ro, vec3 rd, vec3 n, vec3 p, float tm, out float t) {
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

bool intersectBox(vec3 ro, vec3 rd, int num, float tm, out float t, out vec3 normal) 
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
    
	if (tN >= tm)
		return false;

	vec3 nor = -sign(rdd)*step(t1.yzx,t1.xyz)*step(t1.zxy,t1.xyz);
	t = tN;
	// convert to ray space
	normal = rotate(quat_inv(box.quat_rotation), nor);
	return true;
}

// torus section
float uit=0.;
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
bool intersectTorus( in vec3 ro, in vec3 rd, int num, float tm, out float t ){
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
		uit+=1.;
	}
	vec4 rs= vec4(c0.x, c1.x, c2.x, c3.x);
	vec4 ri= abs(vec4(c0.y, c1.y, c2.y, c3.y));

	if(ri.x>eps || rs.x<0.) rs.x=10000.;
	if(ri.y>eps || rs.y<0.) rs.y=10000.;
	if(ri.z>eps || rs.z<0.) rs.z=10000.;
	if(ri.w>eps || rs.w<0.) rs.w=10000.;
	t = min(min(rs.x,rs.y),min(rs.z,rs.w));
	return t > 0 && t < 100 && t < tm;
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

bool intersectSurface(vec3 ro, vec3 rd, int num, float tm, out float t)
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
		return t > tm;
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
	return t < tm;
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

float calcInter(vec3 ro, vec3 rd, out int num, out int type, out vec3 opt_normal)
{
	float tm = maxDist;
	float t;
	for (int i = 0; i < PLANE_SIZE; i++) {
		if (intersectPlane(ro, rd, planes[i].normal, planes[i].pos, tm, t)) {
			num = i; tm = t; type = TYPE_PLANE;
		}
	}
	for (int i = 0; i < SPHERE_SIZE; i++) {
		if (intersectSphere(ro, rd, spheres[i].obj, spheres[i].hollow, tm, t)) {
			num = i; tm = t; type = TYPE_SPHERE;
		}
	}
	for (int i = 0; i < SURFACE_SIZE; i++) {
		if (intersectSurface(ro, rd, i, tm, t)) {
			num = i; tm = t; type = TYPE_SURFACE;
		}
	}
	for (int i = 0; i < BOX_SIZE; i++) {
		if (intersectBox(ro, rd, i, tm, t, opt_normal)) {
			num = i; tm = t; type = TYPE_BOX;
		}
	}
	for (int i = 0; i < TORUS_SIZE; i++) {
		if (intersectTorus(ro, rd, i, tm, t)) {
			num = i; tm = t; type = TYPE_TORUS;
		}
	}
	for (int i = 0; i < LIGHT_POINT_SIZE; i++) {
		if (intersectSphere(ro, rd, lights_point[i].pos, false, tm, t)) {
			num = i; tm = t; type = TYPE_POINT_LIGHT;
		}
	}
	
 	return tm;
}

bool inShadow(vec3 ro, vec3 rd, float d)
{
	bool ret = false;
	float t;
	vec3 opt_normal;
	
	for (int i = 0; i < SPHERE_SIZE; i++)
		if(intersectSphere(ro, rd, spheres[i].obj, false, d, t)) {ret = true;}
	for (int i = 0; i < SURFACE_SIZE; i++)
		if(intersectSurface(ro, rd, i, d, t)) {ret = true;}
	for (int i = 0; i < BOX_SIZE; i++)
		if(intersectBox(ro, rd, i, d, t, opt_normal)) {ret = true;}
	for (int i = 0; i < TORUS_SIZE; i++)
		if(intersectTorus(ro, rd, i, d, t)) {ret = true;}
	#if PLANE_ONESIDE == 0
	for (int i = 0; i < PLANE_SIZE; i++)
		if(intersectPlane(ro,rd, planes[i].normal,planes[i].pos,d,t)) {ret = true;}
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
		lcol *= inShadow(pt, l, dist) ? SHADOW_AMBIENT : vec3(1,1,1);
	#endif
	diffuse += lcol * col * albedo * intensity / distDiv;
	
	//specular
	if (specPower > 0) {
		vec3 reflection = reflect(l, n);
		float specDp = clamp(dot(rd, reflection), 0.0, 1.0);
		specular += lcol * pow(specDp, specPower) * intensity / distDiv;
	}
}

vec3 calcShade(vec3 pt, vec3 rd, vec3 col, float albedo, vec3 n, float specPower, bool doShadow, float kd, float ks)
{
	float dist, distDiv;
	vec3 lcol,l;
	vec3 diffuse = vec3(0);
	vec3 specular = vec3(0);

	vec3 pixelColor = AMBIENT_COLOR * col;

	for (int i = 0; i < LIGHT_POINT_SIZE; i++) {
		lcol = lights_point[i].color;
		l = lights_point[i].pos.xyz - pt;
		dist = length(l);
		distDiv = 1 + dist * dist;

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

hit_record get_hit_info(vec3 ro, vec3 rd, vec3 pt, float tm, int num, int type, vec3 opt_normal) {
	hit_record hr;
	if (type == TYPE_SPHERE) {
		hr = hit_record(spheres[num].mat, normalize(pt - spheres[num].obj.xyz));
	}
	if (type == TYPE_PLANE) {
		hr = hit_record(planes[num].mat, normalize(planes[num].normal));
	}
	if (type == TYPE_SURFACE) {
		hr = hit_record(surfaces[num].mat, getSurfaceNormal(ro, rd, tm, num));
	}
	if (type == TYPE_BOX) {
		hr = hit_record(boxes[num].mat, opt_normal);
	}
	if (type == TYPE_TORUS) {
		hr = hit_record(toruses[num].mat, getTorusNormal(ro, rd, tm, num));
	}
	// if (type == TYPE_POINT_LIGHT) {
	// 	hr = hit_record(empty_mat, vec3(0));
	// }
	return hr;
}

vec3 getReflection(vec3 ro, vec3 rd)
{
	vec3 color = vec3(0);
	vec3 pt, opt_normal;
	int num, type;
	float tm = calcInter(ro, rd, num, type, opt_normal);
	if (type == TYPE_POINT_LIGHT) return lights_point[num].color;
	hit_record hr;
	if(tm < maxDist) {
		pt = ro + rd * tm;
		hr = get_hit_info(ro, rd, pt, tm, num, type, opt_normal);
		color = calcShade(dot(rd, hr.n) < 0 ? pt + hr.n * eps : pt - hr.n * eps, rd, hr.mat.color, hr.mat.diffuse, hr.n, hr.mat.specular, true, hr.mat.kd, hr.mat.ks);
	}
	return color;
}

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
	vec3 opt_normal; // if normal calculations can be processed with intersection search (box)

	vec3 mask = vec3(1.0);
	vec3 color = vec3(0.0);
	vec3 ro = vec3(scene.camera_pos);
	vec3 rd = getRayDir(pixel_coords);
	float absorbDistance = 0.0;
	int type = 0;
	int num;
	hit_record hr;
	
	for(int i = 0; i < ITERATIONS; i++)
	{
		tm = calcInter(ro,rd,num,type,opt_normal);
		if(tm < maxDist)
		{
			pt = ro + rd*tm;
			hr = get_hit_info(ro, rd, pt, tm, num, type, opt_normal);

			if (type == TYPE_POINT_LIGHT) {
				color += lights_point[num].color * mask;
				break;
			}

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
				//todo: rd = reflect(..) instead of break
				if (reflectMultiplier >= 1)
					break;
				#endif
				ro = pt - n * eps;
				rd = refract(rd, n, outside ? 1 / mat.y : mat.y);
				#ifdef REFLECT_REDUCE_ITERATION
				i--;
				#endif
			}
			else if(mat.x > 0.0) // Reflective
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
			color += texture(skybox, rd).rgb * mask;
			break;
		}
	}
	#if DBG == 0
	gl_FragColor = vec4(color,1);
	#else
	if (!dbgEd) gl_FragColor = vec4(0);
	#endif
}
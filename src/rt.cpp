#include <glad/glad.h>

#include "OpenCLUtil.h"
#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"

//#ifdef OS_WIN
//#define GLFW_EXPOSE_NATIVE_WIN32
//#define GLFW_EXPOSE_NATIVE_WGL
//#endif
//
//#ifdef OS_LNX
//#define GLFW_EXPOSE_NATIVE_X11
//#define GLFW_EXPOSE_NATIVE_GLX
//#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "OpenGLUtil.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include "GLWrapper.h"

#include "scene.h"
#include "quaternion.h"

using namespace std;
//using namespace cl;

typedef unsigned int uint;

static const float matrix[16] =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

static const float vertices[12] =
{
    -1.0f,-1.0f, 0.0,
     1.0f,-1.0f, 0.0,
     1.0f, 1.0f, 0.0,
    -1.0f, 1.0f, 0.0
};

static const float texcords[8] =
{
    0.0, 1.0,
    1.0, 1.0,
    1.0, 0.0,
    0.0, 0.0
};

//GLuint genTexture();
//GLuint genRenderProg(GLuint texHandle);
//GLuint genComputeProg(GLuint texHandle);

static const uint indices[6] = {0,1,2,0,2,3};

static int wind_width = 720;
static int wind_height= 720;

//typedef struct {
//    Device d;
//    CommandQueue q;
//    Program p;
//    Kernel k;
//    ImageGL tex;
//
//	rt_scene scene;
//	
//	Buffer sceneMem;
//} process_params;

typedef struct {
    GLuint texPrg;
    //GLuint vao;
    GLuint tex;
	GLuint computePrg;

} render_params;

//process_params params;
render_params rparams;

bool w_pressed = false;
bool a_pressed = false;
bool s_pressed = false;
bool d_pressed = false;
bool ctrl_pressed = false;
bool shift_pressed = false;
bool space_pressed = false;

float lastX = 0;
float lastY = 0;
float pitch = 0;
float yaw = 0;

rt_sphere create_spheres(cl_float4 center, cl_float4 color, cl_float radius, cl_int specular, cl_float reflect)
{
	rt_sphere sphere;
	memset(&sphere, 0, sizeof(rt_sphere));
	sphere.center = center;
	sphere.color = color;
	sphere.radius = radius;
	sphere.specular = specular;
	sphere.reflect = reflect;

	return sphere;
}

rt_light create_light(lightType type, cl_float intensity, cl_float4 position, cl_float4 direction)
{
	rt_light light;
	memset(&light, 0, sizeof(rt_light));
	light.type = type;
	light.intensity = intensity;
	light.position = position;
	light.direction = direction;

	return light;
}



rt_scene create_scene(int width, int height)
{
	auto min = width > height ? height : width;

	int spheres_count = 3;
	std::vector<rt_sphere> spheres;
	std::vector<rt_light> lights;

	spheres.push_back(create_spheres({ 2,0,4 }, { 0,1,0 }, 1, 10, 0.2f));
	spheres.push_back(create_spheres({ -2,0,4 }, { 0,0,1 }, 1, 500, 0.3f));
	spheres.push_back(create_spheres({ 0,-1,3 }, { 1,0,0 }, 1, 500, 0.4f));
	spheres.push_back(create_spheres({ 0,-5001,3 }, { 1,1,0 }, 5000, 50, 0.2f));
	//for (cl_float x = 0.2; x < 8; x++)
	//for (cl_float y = 0; y < 2; y++)
	//for (cl_float z = 0; z < 3; z++)
	//{
	//	cl_float r = (cl_float)rand() / (cl_float)RAND_MAX;
	//	cl_float g = (cl_float)rand() / (cl_float)RAND_MAX;
	//	cl_float b = (cl_float)rand() / (cl_float)RAND_MAX;
	//	cl_float reflect = (cl_float)rand() / (cl_float)RAND_MAX;
	//	cl_int specular = rand() % 500;
	//	//reflect = 0.02;

	//	spheres.push_back(create_spheres({ x - 3, y, z - 2.5f }, { r,g,b }, 0.4f, specular, reflect));
	//}

	lights.push_back(create_light(Ambient, 0.2f, { 0 }, { 0 }));
	lights.push_back(create_light(Point, 0.6f, { 2,1,0 }, { 0 }));
	lights.push_back(create_light(Direct, 0.2f, { 0 }, { 1,4,4 }));

	//spheres.push_back(create_spheres({ 2,1,0 }, { 1,1,1 }, 0.2, 0, 0));

	rt_scene scene;
    memset(&scene, 0, sizeof(rt_scene));
	scene.camera_pos = { 0 };
    scene.canvas_height = height;
    scene.canvas_width = width;
    scene.viewport_dist = 1;
    scene.viewport_height = height / (cl_float) min;
    scene.viewport_width = width / (cl_float) min;
	scene.bg_color = { 0 };
	scene.reflect_depth = 3;

    scene.sphere_count = spheres.size();
	scene.light_count = lights.size();

	std::copy(spheres.begin(), spheres.end(), scene.spheres);
	std::copy(lights.begin(), lights.end(), scene.lights);

    return scene;
}

void multiplyVector(cl_float v[3], cl_float s) {
	v[0] *= s;
	v[1] *= s;
	v[2] *= s;
}

void addVector(cl_float4 *v1, const cl_float v2[3]) {
	v1->x += v2[0];
	v1->y += v2[1];
	v1->z += v2[2];
}

void moveCamera(Quaternion<cl_float> &q, const cl_float direction[3], cl_float4 *vector, const cl_float speed) {
	cl_float tmp[3] = { direction[0], direction[1], direction[2] };
	q.QuatRotation(tmp);
	multiplyVector(tmp, speed);
	addVector(vector, tmp);
}

void moveCamera(const cl_float direction[3], cl_float4 *vector, const cl_float speed) {
	cl_float tmp[3] = { direction[0], direction[1], direction[2] };
	multiplyVector(tmp, speed);
	addVector(vector, tmp);
}

void UpdateScene(rt_scene &scene, double frameRate) 
{
	const cl_float xAxis[3] = { 1, 0, 0 };
	const cl_float yAxis[3] = { 0, 1, 0 };
	const cl_float zAxis[3] = { 0, 0, 1 };
	const float PI_F = 3.14159265358979f;

	Quaternion<cl_float> qX(xAxis, -pitch * PI_F / 180.0f);
	Quaternion<cl_float> qY(yAxis, yaw * PI_F / 180.0f);
	Quaternion<cl_float> q = qY * qX;
	//params.scene.camera_rotation = q.GetStruct();


	auto speed = (cl_float)frameRate;
	if (shift_pressed)
		speed *= 3;

	/*if (w_pressed) 
		moveCamera(q, zAxis, &params.scene.camera_pos, speed);
	if (a_pressed)
		moveCamera(q, xAxis, &params.scene.camera_pos, -speed);
	if (s_pressed)
		moveCamera(q, zAxis, &params.scene.camera_pos, -speed);
	if (d_pressed)
		moveCamera(q, xAxis, &params.scene.camera_pos, speed);
	
	if (space_pressed)
		moveCamera(yAxis, &params.scene.camera_pos, speed);
	if (ctrl_pressed)
		moveCamera(yAxis, &params.scene.camera_pos, -speed);*/
}

static void glfw_error_callback(int error, const char* desc)
{
    fputs(desc,stderr);
}

static void glfw_key_callback(GLFWwindow* wind, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(wind, GL_TRUE);

		bool pressed = action == GLFW_PRESS;

		if (key == GLFW_KEY_W)
			w_pressed = pressed;
		else if (key == GLFW_KEY_S)
			s_pressed = pressed;
		else if (key == GLFW_KEY_A)
			a_pressed = pressed;
		else if (key == GLFW_KEY_D)
			d_pressed = pressed;
		else if (key == GLFW_KEY_SPACE)
			space_pressed = pressed;
		else if (key == GLFW_KEY_LEFT_CONTROL)
			ctrl_pressed = pressed;
		else if (key == GLFW_KEY_LEFT_SHIFT)
			shift_pressed = pressed;
    }
}

bool firstMouse = true;

static void glfw_mouse_callback(GLFWwindow* window, double xpos, double ypos) {

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; 
	lastX = xpos;
	lastY = ypos;



	float sensitivity = 0.05f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
}

static void glfw_framebuffer_size_callback(GLFWwindow* wind, int width, int height)
{
    glViewport(0,0,width,height);
}

void processTimeStep(double frameRate);
void renderFrame(void);
void updateTex(int frame);
void draw();
void checkErrors(std::string desc);

//GLuint renderHandle;
GLuint computeHandle;



int main()
{
	int w = 2048, h = 2048;

	GLWrapper glWrapper(true);
	glWrapper.init();
	w = glWrapper.getWidth();
	h = glWrapper.getHeight();

	GLuint quad_vao = glWrapper.create_quad_vao();
	GLuint quad_program = glWrapper.create_quad_program();

	GLuint ray_program = glWrapper.create_compute_program();

	GLuint tex_output = 0;
	int tex_w = w, tex_h = h;
	{ // create the texture
		glGenTextures(1, &tex_output);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_output);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// linear allows us to scale the window up retaining reasonable quality
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		// same internal format as compute shader input
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT,
			NULL);
		// bind to image unit so can write to specific pixels from the shader
		glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	}

	{ // query up the workgroups
		int work_grp_size[3], work_grp_inv;
		// maximum global work group (total work in a dispatch)
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_size[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_size[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_size[2]);
		printf("max global (total) work group size x:%i y:%i z:%i\n", work_grp_size[0],
			work_grp_size[1], work_grp_size[2]);
		// maximum local work group (one shader's slice)
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
		printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
			work_grp_size[0], work_grp_size[1], work_grp_size[2]);
		// maximum compute shader invocations (x * y * z)
		glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
		printf("max computer shader invocations %i\n", work_grp_inv);
	}

	glfwSwapInterval(0);

	while (!glfwWindowShouldClose(glWrapper.window)) { // drawing loop
		{																					 // launch compute shaders!
			glUseProgram(ray_program);
			glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 1);
		}

		// prevent sampling befor all writes to image are done
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(quad_program);
		glBindVertexArray(quad_vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_output);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glfwPollEvents();
		if (GLFW_PRESS == glfwGetKey(glWrapper.window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(glWrapper.window, 1);
		}
		glfwSwapBuffers(glWrapper.window);
	}

	glWrapper.stop(); // stop glfw, close window
	return 0;

    //cl_int errCode;
    //try {
        
        // create opengl stuff
        //rparams.prg = initShaders(ASSETS_DIR "/rt.vert", ASSETS_DIR "/rt.frag");
        //rparams.tex = createTexture2D(wind_width,wind_height);
        //GLuint vbo  = createBuffer(12,vertices,GL_STATIC_DRAW);
        //GLuint tbo  = createBuffer(8,texcords,GL_STATIC_DRAW);
        //GLuint ibo;
        //glGenBuffers(1,&ibo);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        //glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(uint)*6,indices,GL_STATIC_DRAW);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
        //// bind vao
        //glGenVertexArrays(1,&rparams.vao);
        //glBindVertexArray(rparams.vao);
        //// attach vbo
        //glBindBuffer(GL_ARRAY_BUFFER,vbo);
        //glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,NULL);
        //glEnableVertexAttribArray(0);
        //// attach tbo
        //glBindBuffer(GL_ARRAY_BUFFER,tbo);
        //glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,NULL);
        //glEnableVertexAttribArray(1);
        //// attach ibo
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo);
        //glBindVertexArray(0);
        // create opengl texture reference using opengl texture
        //params.tex = ImageGL(context,CL_MEM_READ_WRITE,GL_TEXTURE_2D,0,rparams.tex,&errCode);
        /*if (errCode!=CL_SUCCESS) {
            std::cout<<"Failed to create OpenGL texture refrence: "<<errCode<<std::endl;
            return 250;
        }*/


  //      params.scene = create_scene(wind_width, wind_height);
		//
		//params.sceneMem = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(rt_scene), &params.scene, &errCode);
		//if (errCode != CL_SUCCESS) {
		//	std::cout << "Failed to create scene buffer: " << errCode << std::endl;
		//	return 250;
		//}

  //      // set kernel arguments
  //      params.k.setArg(0, params.sceneMem);
  //      params.k.setArg(1, params.tex);

    /*} catch(Error error) {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
        std::string val = params.p.getBuildInfo<CL_PROGRAM_BUILD_LOG>(params.d);
        std::cout<<"Log:\n"<<val<<std::endl;
        return 249;
    }*/

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	/*glfwSetCursorPosCallback(window, glfw_mouse_callback);
    glfwSetKeyCallback(window,glfw_key_callback);
    glfwSetFramebufferSizeCallback(window,glfw_framebuffer_size_callback);*/

 //   const auto start = std::chrono::steady_clock::now();
 //   int frames_count = 0;
 //   

	//glfwSwapInterval(0);


	////GLuint texHandle = genTexture();
	////renderHandle = genRenderProg(texHandle);
	//computeHandle = genComputeProg(rparams.tex);

	//auto currentTime = std::chrono::steady_clock::now();

 //   while (!glfwWindowShouldClose(window)) 
	//{
 //       ++frames_count;

	//	auto newTime = std::chrono::steady_clock::now();
	//	std::chrono::duration<double> frameTime = (newTime - currentTime);
	//	currentTime = newTime;

 //       // process call
 //       //processTimeStep(frameTime.count());
 //       // render call
 //       renderFrame();
 //       // swap front and back buffers
 //       glfwSwapBuffers(window);
 //       // poll for events
 //       glfwPollEvents();
 //   }

	/*for (int i = 0; ; ++i) {
		updateTex(i);
		draw();
		glfwSwapBuffers(window);
	}

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now( ) - start );
    auto seconds = elapsed.count() / 1000.0;
    std::cout << "Total elapsed (sec): " << seconds << std::endl;
    std::cout << "Total frames: " << frames_count << std::endl;

    double fps = frames_count / seconds;

    std::cout << "FPS: " << fps << std::endl;

    glfwDestroyWindow(window);

    glfwTerminate();


    std::cin.get();
    return 0;*/
}


//void updateTex(int frame) {
//	glUseProgram(computeHandle);
//	glUniform1f(glGetUniformLocation(computeHandle, "roll"), (float)frame*0.01f);
//	glDispatchCompute(512 / 16, 512 / 16, 1); // 512^2 threads in blocks of 16^2
//	checkErrors("Dispatch compute shader");
//}
//
//void draw() {
//	glUseProgram(rparams.prg);
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//	//swapBuffers();
//	checkErrors("Draw screen");
//}


inline unsigned divup(unsigned a, unsigned b)
{
	return (a + b - 1) / b;
}

void processTimeStep(double frameRate)
{
  //  cl::Event ev;
  //  try {
  //      glFinish();

  //      std::vector<Memory> objs;
  //      objs.clear();
  //      objs.push_back(params.tex);
  //      // flush opengl commands and wait for object acquisition
  //      cl_int res = params.q.enqueueAcquireGLObjects(&objs,NULL,&ev);
  //      ev.wait();
  //      if (res!=CL_SUCCESS) {
  //          std::cout<<"Failed acquiring GL object: "<<res<<std::endl;
  //          exit(248);
  //      }
  //      
		//NDRange local(16,16);
		//NDRange global(local[0] * divup(wind_width, local[0]),local[1] * divup(wind_height, local[1]));

		//UpdateScene(params.scene, frameRate);

		//params.q.enqueueWriteBuffer(params.sceneMem, CL_TRUE, 0, sizeof(rt_scene), &params.scene, NULL, NULL);

  //      params.q.enqueueNDRangeKernel(params.k,cl::NullRange, global, local);
  //      // release opengl object
  //      res = params.q.enqueueReleaseGLObjects(&objs);
  //      ev.wait();
  //      if (res!=CL_SUCCESS) {
  //          std::cout<<"Failed releasing GL object: "<<res<<std::endl;
  //          exit(247);
  //      }
  //      params.q.finish();
  //  } catch(Error err) {
  //      std::cout << err.what() << "(" << err.err() << ")" << std::endl;
  //  }
}

void renderFrame()
{
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glClearColor(0.2,0.2,0.2,0.0);
    //glEnable(GL_DEPTH_TEST);
    //// bind shader
    //glUseProgram(rparams.prg);
    //// get uniform locations
    //int mat_loc = glGetUniformLocation(rparams.prg,"matrix");
    //int tex_loc = glGetUniformLocation(rparams.prg,"tex");
    //// bind texture
    //glActiveTexture(GL_TEXTURE0);
    //glUniform1i(tex_loc,0);
    //glBindTexture(GL_TEXTURE_2D,rparams.tex);
    //glGenerateMipmap(GL_TEXTURE_2D);
    //// set project matrix
    //glUniformMatrix4fv(mat_loc,1,GL_FALSE,matrix);
    //// now render stuff
    //glBindVertexArray(rparams.vao);
    //glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
    //glBindVertexArray(0);
}

//#define GL_COMPUTE_SHADER                 0x91B9

void checkErrors(std::string desc) {
	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		fprintf(stderr, "OpenGL error in \"%s\": (%d)\n", desc.c_str(), e);
		exit(20);
	}
}

GLuint genComputeProg(GLuint texHandle) {
	// Creating the compute shader, and the program object containing the shader
	GLuint progHandle = glCreateProgram();
	GLuint cs = glCreateShader(GL_COMPUTE_SHADER);

	// In order to write to a texture, we have to introduce it as image2D.
	// local_size_x/y/z layout variables define the work group size.
	// gl_GlobalInvocationID is a uvec3 variable giving the global ID of the thread,
	// gl_LocalInvocationID is the local index within the work group, and
	// gl_WorkGroupID is the work group's index
	const char *csSrc[] = {
		"#version 430\n",
		"uniform float roll;\
		 layout (rgba32f, binding = 0) uniform image2D destTex;\
         layout (local_size_x = 16, local_size_y = 16) in;\
         void main() {\
             ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);\
             float localCoef = length(vec2(ivec2(gl_LocalInvocationID.xy)-8)/8.0);\
             float globalCoef = sin(float(gl_WorkGroupID.x+gl_WorkGroupID.y)*0.1 + roll)*0.5;\
             imageStore(destTex, storePos, vec4(1.0-globalCoef*localCoef, 0.0, 0.0, 0.0));\
         }"
	};

	glShaderSource(cs, 2, csSrc, NULL);
	glCompileShader(cs);
	int rvalue;
	glGetShaderiv(cs, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in compiling the compute shader\n");
		GLchar log[10240];
		GLsizei length;
		glGetShaderInfoLog(cs, 10239, &length, log);
		fprintf(stderr, "Compiler log:\n%s\n", log);
		exit(40);
	}
	glAttachShader(progHandle, cs);

	glLinkProgram(progHandle);
	glGetProgramiv(progHandle, GL_LINK_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in linking compute shader program\n");
		GLchar log[10240];
		GLsizei length;
		glGetProgramInfoLog(progHandle, 10239, &length, log);
		fprintf(stderr, "Linker log:\n%s\n", log);
		exit(41);
	}
	glUseProgram(progHandle);

	glUniform1i(glGetUniformLocation(progHandle, "destTex"), 0);

	checkErrors("Compute shader");
	return progHandle;
}
//GLuint genRenderProg(GLuint texHandle) {
//	GLuint progHandle = glCreateProgram();
//	GLuint vp = glCreateShader(GL_VERTEX_SHADER);
//	GLuint fp = glCreateShader(GL_FRAGMENT_SHADER);
//
//	const char *vpSrc[] = {
//		"#version 430\n",
//		"in vec2 pos;\
//		 out vec2 texCoord;\
//		 void main() {\
//			 texCoord = pos*0.5f + 0.5f;\
//			 gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);\
//		 }"
//	};
//
//	const char *fpSrc[] = {
//		"#version 430\n",
//		"uniform sampler2D srcTex;\
//		 in vec2 texCoord;\
//		 out vec4 color;\
//		 void main() {\
//			 float c = texture(srcTex, texCoord).x;\
//			 color = vec4(c, 1.0, 1.0, 1.0);\
//		 }"
//	};
//
//	glShaderSource(vp, 2, vpSrc, NULL);
//	glShaderSource(fp, 2, fpSrc, NULL);
//
//	glCompileShader(vp);
//	int rvalue;
//	glGetShaderiv(vp, GL_COMPILE_STATUS, &rvalue);
//	if (!rvalue) {
//		fprintf(stderr, "Error in compiling vp\n");
//		exit(30);
//	}
//	glAttachShader(progHandle, vp);
//
//	glCompileShader(fp);
//	glGetShaderiv(fp, GL_COMPILE_STATUS, &rvalue);
//	if (!rvalue) {
//		fprintf(stderr, "Error in compiling fp\n");
//		exit(31);
//	}
//	glAttachShader(progHandle, fp);
//
//	glBindFragDataLocation(progHandle, 0, "color");
//	glLinkProgram(progHandle);
//
//	glGetProgramiv(progHandle, GL_LINK_STATUS, &rvalue);
//	if (!rvalue) {
//		fprintf(stderr, "Error in linking sp\n");
//		exit(32);
//	}
//
//	glUseProgram(progHandle);
//	glUniform1i(glGetUniformLocation(progHandle, "srcTex"), 0);
//
//	GLuint vertArray;
//	glGenVertexArrays(1, &vertArray);
//	glBindVertexArray(vertArray);
//
//	GLuint posBuf;
//	glGenBuffers(1, &posBuf);
//	glBindBuffer(GL_ARRAY_BUFFER, posBuf);
//	float data[] = {
//		-1.0f, -1.0f,
//		-1.0f, 1.0f,
//		1.0f, -1.0f,
//		1.0f, 1.0f
//	};
//	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, data, GL_STREAM_DRAW);
//	GLint posPtr = glGetAttribLocation(progHandle, "pos");
//	glVertexAttribPointer(posPtr, 2, GL_FLOAT, GL_FALSE, 0, 0);
//	glEnableVertexAttribArray(posPtr);
//
//	checkErrors("Render shaders");
//	return progHandle;
//}
//GLuint genTexture() {
//	// We create a single float channel 512^2 texture
//	GLuint texHandle;
//	glGenTextures(1, &texHandle);
//
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, texHandle);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 512, 512, 0, GL_RED, GL_FLOAT, NULL);
//
//	// Because we're also using this tex as an image (in order to write to it),
//	// we bind it to an image unit as well
//	glBindImageTexture(0, texHandle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
//	checkErrors("Gen texture");
//	return texHandle;
//}
#include "GLWrapper.h"
#include <iostream>
#include <fstream>

static void glfw_error_callback(int error, const char * desc)
{
	fputs(desc, stderr);
}

GLWrapper::GLWrapper(int width, int height, bool fullScreen)
{
	this->width = width;
	this->height = height;
	this->fullScreen = fullScreen;
	this->useCustomResolution = true;
}

GLWrapper::GLWrapper(bool fullScreen)
{
	this->fullScreen = fullScreen;
}

GLWrapper::~GLWrapper()
{
	glfwDestroyWindow(window);

	glfwTerminate();
}

int GLWrapper::getWidth()
{
	return width;
}

int GLWrapper::getHeight()
{
	return height;
}

typedef struct color
{
	float r, g, b, a; // colors
};

bool GLWrapper::init()
{
	if (!glfwInit())
		return false;

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

	if (!useCustomResolution) 
	{
		width = mode->width;
		height = mode->height;
	}

	glfwSetErrorCallback(glfw_error_callback);

	window = glfwCreateWindow(width, height, "RT", fullScreen ? monitor : NULL, NULL);

	if (!window) {
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGL()) {
		printf("gladLoadGL failed!\n");
		return false;
	}
	printf("OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

    texHandle = genTexture(width, height);
	renderHandle = genRenderProg();
	computeHandle = genComputeProg();


	color c = { 0 };
	c.r = 255;
	c.g = 0;
	c.b = 0;
	c.a = 0;

	GLuint ssbo = 0;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(c), &c, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return true;
}

void GLWrapper::stop()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void GLWrapper::draw()
{
	



    glUseProgram(computeHandle);
	//glUniform1f(glGetUniformLocation(computeHandle, "roll"), (float)frame*0.01f);
	glDispatchCompute(width / 16.0, height / 16.0, 1);
	//glDispatchCompute(w, h, 1); // 512^2 threads in blocks of 16^2
	checkErrors("Dispatch compute shader");

    glUseProgram(renderHandle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	//swapBuffers();
	checkErrors("Draw screen");
}


void GLWrapper::print_shader_info_log(GLuint shader) {
	int max_length = 4096;
	int actual_length = 0;
	char slog[4096];
	glGetShaderInfoLog(shader, max_length, &actual_length, slog);
	fprintf(stderr, "shader info log for GL index %u\n%s\n", shader, slog);
}

void GLWrapper::print_program_info_log(GLuint program) {
	int max_length = 4096;
	int actual_length = 0;
	char plog[4096];
	glGetProgramInfoLog(program, max_length, &actual_length, plog);
	fprintf(stderr, "program info log for GL index %u\n%s\n", program, plog);
}

bool GLWrapper::check_shader_errors(GLuint shader) {
	GLint params = -1;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf(stderr, "ERROR: shader %u did not compile\n", shader);
		print_shader_info_log(shader);
		return false;
	}
	return true;
}

bool GLWrapper::check_program_errors(GLuint program) {
	GLint params = -1;
	glGetProgramiv(program, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf(stderr, "ERROR: program %u did not link\n", program);
		print_program_info_log(program);
		return false;
	}
	return true;
}

void GLWrapper::checkErrors(std::string desc)
{
    GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		fprintf(stderr, "OpenGL error in \"%s\": (%d)\n", desc.c_str(), e); //todo error must be here
		exit(20);
	}
}

GLuint GLWrapper::genTexture(int width, int height)
{
    GLuint texHandle;
	glGenTextures(1, &texHandle);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

	// Because we're also using this tex as an image (in order to write to it),
	// we bind it to an image unit as well
	glBindImageTexture(0, texHandle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	checkErrors("Gen texture");
	return texHandle;
}

GLuint GLWrapper::genRenderProg()
{
    GLuint progHandle = glCreateProgram();
	GLuint vp = glCreateShader(GL_VERTEX_SHADER);
	GLuint fp = glCreateShader(GL_FRAGMENT_SHADER);

	const char *vpSrc[] = {
		"#version 430\n",
		"in vec2 pos;\
		 out vec2 texCoord;\
		 void main() {\
			 texCoord = pos*0.5f + 0.5f;\
			 gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);\
		 }"
	};

	const char *fpSrc[] = {
		"#version 430\n",
		"uniform sampler2D srcTex;\
		 in vec2 texCoord;\
		 out vec4 color;\
		 void main() {\
			 color = texture2D(srcTex,texCoord);\
		 }"
	};

	glShaderSource(vp, 2, vpSrc, NULL);
	glShaderSource(fp, 2, fpSrc, NULL);

	glCompileShader(vp);
	int rvalue;
	glGetShaderiv(vp, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in compiling vp\n");
		exit(30);
	}
	glAttachShader(progHandle, vp);

	glCompileShader(fp);
	glGetShaderiv(fp, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in compiling fp\n");
		exit(31);
	}
	glAttachShader(progHandle, fp);

	glBindFragDataLocation(progHandle, 0, "color");
	glLinkProgram(progHandle);

	glGetProgramiv(progHandle, GL_LINK_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in linking sp\n");
		exit(32);
	}

	glUseProgram(progHandle);
	glUniform1i(glGetUniformLocation(progHandle, "srcTex"), 0);

	GLuint vertArray;
	glGenVertexArrays(1, &vertArray);
	glBindVertexArray(vertArray);

	GLuint posBuf;
	glGenBuffers(1, &posBuf);
	glBindBuffer(GL_ARRAY_BUFFER, posBuf);
	float data[] = {
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, data, GL_STREAM_DRAW);
	GLint posPtr = glGetAttribLocation(progHandle, "pos");
	glVertexAttribPointer(posPtr, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posPtr);

	checkErrors("Render shaders");
	return progHandle;
}

char* loadFile(const char *fname, GLint &fSize)
{
	std::ifstream file(fname, std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		unsigned int size = (unsigned int)file.tellg();
		fSize = size;
		char *memblock = new char[size];
		file.seekg(0, std::ios::beg);
		file.read(memblock, size);
		file.close();
		std::cout << "file " << fname << " loaded" << std::endl;
		return memblock;
	}

	std::cout << "Unable to open file " << fname << std::endl;
	return NULL;
}

static const char *compute_shader_str =
"#version 430\n                                                               \
layout (local_size_x = 16, local_size_y = 16) in;\n                             \
layout (rgba32f, binding = 0) uniform image2D img_output;\n                   \
\n                                                                            \
void main () {\n                                                              \
  vec4 pixel = vec4 (0.0, 0.0, 0.0, 1.0);\n                                   \
  ivec2 pixel_coords = ivec2 (gl_GlobalInvocationID.xy);\n                    \
\n                                                                            \
float max_x = 5.0;\n                                                          \
float max_y = 5.0;\n                                                          \
ivec2 dims = imageSize (img_output);\n                                        \
float x = (float(pixel_coords.x * 2 - dims.x) / dims.x);\n                    \
float y = (float(pixel_coords.y * 2 - dims.y) / dims.y);\n                    \
vec3 ray_o = vec3 (x * max_x, y * max_y, 0.0);\n                              \
vec3 ray_d = vec3 (0.0, 0.0, -1.0); // ortho\n                                \
\n                                                                            \
vec3 sphere_c = vec3 (0.0, 0.0, -10.0);                                       \
float sphere_r = 1.0;                                                         \
\n                                                                            \
vec3 omc = ray_o - sphere_c;\n                                                \
float b = dot (ray_d, omc);\n                                                 \
float c = dot (omc, omc) - sphere_r * sphere_r;\n                             \
float bsqmc = b * b - c;\n                                                    \
float t = 10000.0;\n                                                          \
// hit one or both sides\n                                                    \
if (bsqmc >= 0.0) {\n                                                         \
  pixel = vec4 (0.4, 0.4, 1.0, 1.0);\n                                        \
}\n                                                                           \
\n                                                                            \
  imageStore (img_output, pixel_coords, pixel);\n                             \
}\n";

static const char *test =
"#version 430\n                                                               \
layout (local_size_x = 16, local_size_y = 16) in;\n                             \
layout (rgba32f, binding = 0) uniform image2D img_output;\n                   \
\n                                                                            \
void main () {\n                                                              \
  vec4 pixel = vec4 (0.0, 0.0, 0.0, 1.0);\n                                   \
  ivec2 pixel_coords = ivec2 (gl_GlobalInvocationID.xy);\n                    \
\n                                                                            \
float max_x = 5.0;\n                                                          \
float max_y = 5.0;\n                                                          \
ivec2 dims = imageSize (img_output);\n                                        \
float x = (float(pixel_coords.x * 2 - dims.x) / dims.x);\n                    \
float y = (float(pixel_coords.y * 2 - dims.y) / dims.y);\n                    \
vec3 ray_o = vec3 (x * max_x, y * max_y, 0.0);\n                              \
vec3 ray_d = vec3 (0.0, 0.0, -1.0); // ortho\n                                \
\n                                                                            \
vec3 sphere_c = vec3 (0.0, 0.0, -10.0);                                       \
float sphere_r = 1.0;                                                         \
\n                                                                            \
vec3 omc = ray_o - sphere_c;\n                                                \
float b = dot (ray_d, omc);\n                                                 \
float c = dot (omc, omc) - sphere_r * sphere_r;\n                             \
float bsqmc = b * b - c;\n                                                    \
float t = 10000.0;\n                                                          \
// hit one or both sides\n                                                    \
if (bsqmc >= 0.0) {\n                                                         \
  pixel = vec4 (0.4, 0.4, 1.0, 1.0);\n                                        \
}\n                                                                           \
\n                                                                            \
  imageStore (img_output, pixel_coords, pixel);\n                             \
}\n";

GLuint GLWrapper::genComputeProg()
{
    // Creating the compute shader, and the program object containing the shader
	GLuint progHandle = glCreateProgram();
	GLuint cs = glCreateShader(GL_COMPUTE_SHADER);

	//// In order to write to a texture, we have to introduce it as image2D.
	//// local_size_x/y/z layout variables define the work group size.
	//// gl_GlobalInvocationID is a uvec3 variable giving the global ID of the thread,
	//// gl_LocalInvocationID is the local index within the work group, and
	//// gl_WorkGroupID is the work group's index
	//const char *csSrc = 
	//	"#version 430\n \
	//	uniform float roll;\
	//	 layout (rgba32f, binding = 0) uniform image2D destTex;\
 //        layout (local_size_x = 16, local_size_y = 16) in;\
 //        void main() {\
 //            ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);\
 //            float localCoef = length(vec2(ivec2(gl_LocalInvocationID.xy)-8)/8.0);\
 //            float globalCoef = sin(float(gl_WorkGroupID.x+gl_WorkGroupID.y)*0.1 + roll)*0.5;\
 //            imageStore(destTex, storePos, vec4(1.0-globalCoef*localCoef, 0.0, 0.0, 0.0));\
 //        }"
	//;

	char *source;
	GLint source_len;
	source = loadFile(ASSETS_DIR "/rt.comp", source_len);

	glShaderSource(cs, 1, &source, &source_len);
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

	glUniform1i(glGetUniformLocation(progHandle, "img_output"), 0);

	checkErrors("Compute shader");

	delete[] source;

	return progHandle;
}

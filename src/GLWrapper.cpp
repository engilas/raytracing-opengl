#include "GLWrapper.h"

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

	return true;
}

void GLWrapper::stop()
{
	glfwDestroyWindow(window);
	glfwTerminate();
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

GLuint GLWrapper::create_quad_vao() {
	GLuint vao = 0, vbo = 0;
	float verts[] = { -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,
										1.0f,	-1.0f, 1.0f, 0.0f, 1.0f,	1.0f, 1.0f, 1.0f };
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), verts, GL_STATIC_DRAW);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	GLintptr stride = 4 * sizeof(float);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, NULL);
	glEnableVertexAttribArray(1);
	GLintptr offset = 2 * sizeof(float);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
	return vao;
}

// this is the quad's vertex shader in an ugly C string
static const char *vert_shader_str =
"#version 430\n                                                               \
layout (location = 0) in vec2 vp;\n                                           \
layout (location = 1) in vec2 vt;\n                                           \
out vec2 st;\n                                                                \
\n                                                                            \
void main () {\n                                                              \
  st = vt;\n                                                                  \
  gl_Position = vec4 (vp, 0.0, 1.0);\n                                        \
}\n";

// this is the quad's fragment shader in an ugly C string
static const char *frag_shader_str =
"#version 430\n                                                               \
in vec2 st;\n                                                                 \
uniform sampler2D img;\n                                                      \
out vec4 fc;\n                                                                \
\n                                                                            \
void main () {\n                                                              \
  fc = texture (img, st);\n                                                 \
}\n";

GLuint GLWrapper::create_quad_program() {
	GLuint program = glCreateProgram();
	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_shader, 1, &vert_shader_str, NULL);
	glCompileShader(vert_shader);
	check_shader_errors(vert_shader); // code moved to gl_utils.cpp
	glAttachShader(program, vert_shader);
	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, &frag_shader_str, NULL);
	glCompileShader(frag_shader);
	check_shader_errors(frag_shader); // code moved to gl_utils.cpp
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	check_program_errors(program); // code moved to gl_utils.cpp
	return program;
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
for (int ia = 0; ia < 500; ia++)    b = dot (ray_d, omc);\n                                         \
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

GLuint GLWrapper::create_compute_program() 
{
	GLuint ray_program = 0;
	 // create the compute shader
	GLuint ray_shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(ray_shader, 1, &compute_shader_str, NULL);
	glCompileShader(ray_shader);
	(check_shader_errors(ray_shader)); // code moved to gl_utils.cpp
	ray_program = glCreateProgram();
	glAttachShader(ray_program, ray_shader);
	glLinkProgram(ray_program);
	(check_program_errors(ray_program)); // code moved to gl_utils.cpp
	return ray_program;
}
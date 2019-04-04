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

    //texHandle = genTexture(width, height);
	//renderHandle = genRenderProg();
	renderHandle = genComputeProg();

	return true;
}

void GLWrapper::stop()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

inline unsigned divup(unsigned a, unsigned b)
{
	return (a + b - 1) / b;
}

GLvoid drawRectangle(GLfloat *v1, GLfloat *v2, GLfloat *v3, GLfloat *v4, GLfloat *col1, GLfloat *col2, GLfloat *col3, GLfloat *col4)
{
	// glBegin(GL_POYGON);
	// glColor4fv(col1);
	// glVertex4fv(v1);
	// glColor4fv(col2);
	// glVertex4fv(v2);
	// glColor4fv(col3);
	// glVertex4fv(v3);
	// glColor4fv(col4);
	// glVertex4fv(v4);
	// glEnd();
}

void GLWrapper::draw()
{
	//glDispatchCompute(divup(width, 16), divup(height, 16), 1);
	//checkErrors("Dispatch compute shader");

	//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	//glRecti(-1, -1, 1, 1); /* fragment shader is not run unless there's vertices in OpenGL 2? */

	//glVertexAttrib1f(0, 0);
	//glDrawArrays(GL_POINT, 0, 1);
	//glDrawArrays(GL_POINT, )
	//glBindVertexArray()
    //glUseProgram(computeHandle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
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

static char* load_file(const char *fname, GLint &fSize)
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

GLuint GLWrapper::genRenderProg()
{
    GLuint progHandle = glCreateProgram();
	GLuint vp = glCreateShader(GL_VERTEX_SHADER);
	GLuint fp = glCreateShader(GL_FRAGMENT_SHADER);

	const char *vpSrc[] = {
		"#version 430\n",
		"layout(location = 0) out vec2 uv;\
		void main()\
		{\
			float x = float(((uint(gl_VertexID) + 2u) / 3u) % 2u);\
			float y = float(((uint(gl_VertexID) + 1u) / 3u) % 2u);\
			gl_Position = vec4(-1.0f + x * 2.0f, -1.0f + y * 2.0f, 0.0f, 1.0f);\
			uv = vec2(x, y);\
		}"
	};

	GLint source_len;
	auto fpSrc = load_file(ASSETS_DIR "/rt.comp", source_len);

	// const char *fpSrc[] = {
	// 	"#version 430\n",
	// 	"uniform sampler2D srcTex;\
	// 	 in vec2 texCoord;\
	// 	 out vec4 color;\
	// 	 void main() {\
	// 		 color = texture2D(srcTex,texCoord);\
	// 	 }"
	// };

	glShaderSource(vp, 2, vpSrc, NULL);
	glShaderSource(fp, 2, &fpSrc, NULL);

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

	// GLuint vertArray;
	// glGenVertexArrays(1, &vertArray);
	// glBindVertexArray(vertArray);

	// GLuint posBuf;
	// glGenBuffers(1, &posBuf);
	// glBindBuffer(GL_ARRAY_BUFFER, posBuf);
	// float data[] = {
	// 	-1.0f, -1.0f,
	// 	-1.0f, 1.0f,
	// 	1.0f, -1.0f,
	// 	1.0f, 1.0f
	// };
	// glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, data, GL_STREAM_DRAW);
	// GLint posPtr = glGetAttribLocation(progHandle, "pos");
	glVertexAttribPointer(0, 0, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	checkErrors("Render shaders");
	return progHandle;
}

GLuint GLWrapper::genComputeProg()
{
    // Creating the compute shader, and the program object containing the shader
	GLuint progHandle = glCreateProgram();
	GLuint cs = glCreateShader(GL_FRAGMENT_SHADER);

	char *source;
	GLint source_len;
	source = load_file(ASSETS_DIR "/rt.comp", source_len);

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


	checkErrors("Compute shader");

	delete[] source;

	return progHandle;
}

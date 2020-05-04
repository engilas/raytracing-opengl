#include "GLWrapper.h"
#include <iostream>
#include <fstream>
#include "scene.h"
#include <stb_image.h>

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

bool GLWrapper::init_window()
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

void GLWrapper::init_shaders(rt_defines defines)
{
	renderHandle = genRenderProg(defines);
}

void GLWrapper::setSkybox(unsigned textureId)
{
	skyboxHandle = textureId;
	glUniform1i(glGetUniformLocation(renderHandle, "skybox"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxHandle);
}

void GLWrapper::stop()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void GLWrapper::draw()
{
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
	checkErrors("Draw screen");
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

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

GLuint GLWrapper::genRenderProg(rt_defines defines)
{
    GLuint progHandle = glCreateProgram();
	GLuint vp = glCreateShader(GL_VERTEX_SHADER);
	GLuint fp = glCreateShader(GL_FRAGMENT_SHADER);

	const char *vpSrc[] = {
		"#version 430\n",
		"void main()\
		{\
			float x = float(((uint(gl_VertexID) + 2u) / 3u) % 2u);\
			float y = float(((uint(gl_VertexID) + 1u) / 3u) % 2u);\
			gl_Position = vec4(-1.0f + x * 2.0f, -1.0f + y * 2.0f, 0.0f, 1.0f);\
		}"
	};

	GLint source_len;
	auto fpSrcChar = load_file(ASSETS_DIR "/rt.frag", source_len);

	std::string fpS(fpSrcChar, source_len);
	replace(fpS, "{SPHERE_SIZE}", std::to_string(defines.sphere_size));
	replace(fpS, "{PLANE_SIZE}", std::to_string(defines.plane_size));
	replace(fpS, "{SURFACE_SIZE}", std::to_string(defines.surface_size));
	replace(fpS, "{BOX_SIZE}", std::to_string(defines.box_size));
	replace(fpS, "{TORUS_SIZE}", std::to_string(defines.torus_size));
	replace(fpS, "{LIGHT_POINT_SIZE}", std::to_string(defines.light_point_size));
	replace(fpS, "{LIGHT_DIRECT_SIZE}", std::to_string(defines.light_direct_size));
	replace(fpS, "{ITERATIONS}", std::to_string(defines.iterations));
	replace(fpS, "{AMBIENT_COLOR}", defines.ambient_color.toString());
	replace(fpS, "{SHADOW_AMBIENT}", defines.shadow_ambient.toString());
	//fpS.append('\0');
	auto tmp = fpS.c_str();
	source_len = fpS.size();

	glShaderSource(vp, 2, vpSrc, NULL);
	glShaderSource(fp, 1, &tmp, NULL);

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
		//exit(31);
		fprintf(stderr, "Error in compiling the fragment shader\n");
		GLchar log[10240];
		GLsizei length;
		glGetShaderInfoLog(fp, 10239, &length, log);
		fprintf(stderr, "Compiler log:\n%s\n", log);
		exit(40);
	}
	glAttachShader(progHandle, fp);

	glBindFragDataLocation(progHandle, 0, "color");
	glLinkProgram(progHandle);

	glGetProgramiv(progHandle, GL_LINK_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in linking sp\n");
		check_program_errors(progHandle);
		exit(32);
	}

	glUseProgram(progHandle);

	delete[] fpSrcChar;

	checkErrors("Render shaders");
	return progHandle;
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

unsigned int GLWrapper::loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}
#include "GLWrapper.h"
#include <iostream>
#include <fstream>
#include "scene.h"
#include <stb_image.h>
#include "shader.h"

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

GLuint GLWrapper::getProgramId()
{
	return shader.ID;
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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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


	float quadVertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		 1.0f, -1.0f, 1.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,

		-1.0f, -1.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 1.0f
	};

	// quad VAO
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);

	return true;
}

void GLWrapper::setSkybox(unsigned textureId)
{
	skyboxTex = textureId;
	shader.setInt("skybox", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
}

void GLWrapper::stop()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void GLWrapper::draw()
{
	shader.use();
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	checkErrors("Draw screen");
}

static std::string readFromFile(const char* path) {
	std::string content;
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		file.open(path);
		std::stringstream stream;
		stream << file.rdbuf();
		file.close();
		content = stream.str();
	}
	catch (std::ifstream::failure& e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		exit(1);
	}
	return content;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

void GLWrapper::init_shaders(rt_defines& defines)
{
	const std::string vertexShaderSrc = readFromFile(ASSETS_DIR "/rt.vert");
	std::string fragmentShaderSrc = readFromFile(ASSETS_DIR "/rt.frag");
	
	replace(fragmentShaderSrc, "{SPHERE_SIZE}", std::to_string(defines.sphere_size));
	replace(fragmentShaderSrc, "{PLANE_SIZE}", std::to_string(defines.plane_size));
	replace(fragmentShaderSrc, "{SURFACE_SIZE}", std::to_string(defines.surface_size));
	replace(fragmentShaderSrc, "{BOX_SIZE}", std::to_string(defines.box_size));
	replace(fragmentShaderSrc, "{TORUS_SIZE}", std::to_string(defines.torus_size));
	replace(fragmentShaderSrc, "{RING_SIZE}", std::to_string(defines.ring_size));
	replace(fragmentShaderSrc, "{LIGHT_POINT_SIZE}", std::to_string(defines.light_point_size));
	replace(fragmentShaderSrc, "{LIGHT_DIRECT_SIZE}", std::to_string(defines.light_direct_size));
	replace(fragmentShaderSrc, "{ITERATIONS}", std::to_string(defines.iterations));
	replace(fragmentShaderSrc, "{AMBIENT_COLOR}", to_string(defines.ambient_color));
	replace(fragmentShaderSrc, "{SHADOW_AMBIENT}", to_string(defines.shadow_ambient));

	shader.initFromSrc(vertexShaderSrc.c_str(), fragmentShaderSrc.c_str());

	checkErrors("Render shaders");

	shader.use();
}

std::string GLWrapper::to_string(glm::vec3 v)
{
	return std::string().append("vec3(").append(std::to_string(v.x)).append(",").append(std::to_string(v.y)).append(",").append(std::to_string(v.z)).append(")");
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

unsigned int GLWrapper::loadCubemap(std::vector<std::string> faces, bool getMipmap)
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
	if (getMipmap) {
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, getMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

unsigned int GLWrapper::loadTexture(char const* path, GLuint wrapMode)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

GLuint GLWrapper::loadTexture(int texNum, const char* name, const char* uniformName, GLuint wrapMode) const
{
	const std::string path = ASSETS_DIR "/textures/" + std::string(name);
	const unsigned int tex = loadTexture(path.c_str(), wrapMode);
	shader.setInt(uniformName, texNum);
	return tex;
}

void GLWrapper::initBuffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data) const
{
	glGenBuffers(1, ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, *ubo);
	glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
	GLuint blockIndex = glGetUniformBlockIndex(shader.ID, name);
	if (blockIndex == 0xffffffff)
	{
		fprintf(stderr, "Invalid ubo block name '%s'", name);
		exit(1);
	}
	glUniformBlockBinding(shader.ID, blockIndex, bindingPoint);
	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, *ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GLWrapper::updateBuffer(GLuint ubo, size_t size, void* data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

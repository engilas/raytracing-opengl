#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "shader.h"

struct rt_defines;

class GLWrapper
{
public:
	GLWrapper(int width, int height, bool fullScreen);
	GLWrapper(bool fullScreen);
	~GLWrapper();

	int getWidth();
	int getHeight();
	GLuint getProgramId();

	bool init_window();
	void init_shaders(rt_defines& defines);
	void setSkybox(unsigned int textureId);

	void stop();

	GLFWwindow* window;

	void draw();
	static GLuint loadCubemap(std::vector<std::string> faces, bool getMipmap = false);
	GLuint loadTexture(int texNum, const char* name, const char* uniformName, GLuint wrapMode = GL_REPEAT) const;
	void initBuffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data) const;
	static void updateBuffer(GLuint ubo, size_t size, void* data);

private:
	Shader shader;
	GLuint skyboxTex;
	GLuint quadVAO, quadVBO;

	int width;
	int height;

	bool fullScreen = true;
	bool useCustomResolution = false;

	static GLuint loadTexture(char const* path, GLuint wrapMode = GL_REPEAT);

	static void checkErrors(std::string desc);

	static std::string to_string(glm::vec3 v);
};


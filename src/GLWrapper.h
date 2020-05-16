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
	static unsigned loadCubemap(std::vector<std::string> faces, bool getMipmap = false);
	static unsigned loadTexture(char const* path, GLuint wrapMode = GL_REPEAT);

private:
	Shader shader;
	GLuint skyboxTex;
	unsigned int quadVAO, quadVBO;
	
	int width;
	int height;

	bool fullScreen = true;
	bool useCustomResolution = false;
	
	static void print_shader_info_log(GLuint shader);
    static void print_program_info_log(GLuint program);
    static bool check_shader_errors(GLuint shader);
    static bool check_program_errors(GLuint program);

    static void checkErrors(std::string desc);

	static std::string to_string(glm::vec3 v);
};


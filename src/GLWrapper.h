#pragma once

#include <glad/glad.h>
#include <assert.h>
#include <stdio.h>
#include <GLFW/glfw3.h>

class GLWrapper
{
public:
	GLWrapper(int width, int height, bool fullScreen);
	GLWrapper(bool fullScreen);
	~GLWrapper();

	int getWidth();
	int getHeight();

	bool init();
	void stop();

	GLFWwindow* window;

	GLuint create_quad_vao();
	GLuint create_quad_program();

	GLuint create_compute_program();

private:
	int width;
	int height;

	bool fullScreen = true;
	bool useCustomResolution = false;

	void print_shader_info_log(GLuint shader);
	void print_program_info_log(GLuint program);
	bool check_shader_errors(GLuint shader);
	bool check_program_errors(GLuint program);
};


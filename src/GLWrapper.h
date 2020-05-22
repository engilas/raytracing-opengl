#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "shader.h"
#include "utils.h"
#include "SMAA_Builder.h"

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
	void set_skybox(unsigned int textureId);

	void stop();
	void enable_SMAA(SMAA_PRESET preset);

	GLFWwindow* window;

	void draw();
	static GLuint load_cubemap(std::vector<std::string> faces, bool genMipmap = false);
	GLuint load_texture(int texNum, const char* name, const char* uniformName, GLuint wrapMode = GL_REPEAT);
	void init_buffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data) const;
	static void update_buffer(GLuint ubo, size_t size, void* data);

private:
	Shader shader, edgeShader, blendShader, neighborhoodShader;
	GLuint skyboxTex, areaTex, searchTex;
	GLuint quadVAO, quadVBO;
	GLuint fboColor, fboTexColor, fboEdge, fboTexEdge, fboBlend, fboTexBlend;
	std::vector<GLuint> textures;

	int width;
	int height;

	bool fullScreen = true;
	bool useCustomResolution = false;
	bool SMAA_enabled = false;
	SMAA_PRESET SMAA_preset;

	void gen_framebuffer(GLuint* fbo, GLuint* fboTex, GLenum internalFormat, GLenum format) const;
	
	static GLuint load_texture(char const* path, GLuint wrapMode = GL_REPEAT);
	static std::string to_string(glm::vec3 v);
};


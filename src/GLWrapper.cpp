#include "GLWrapper.h"
#include <iostream>
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
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadVBO);

	if (SMAA_enabled)
	{
		glDeleteFramebuffers(1, &fboColor);
		glDeleteTextures(1, &fboTexColor);

		glDeleteFramebuffers(1, &fboEdge);
		glDeleteTextures(1, &fboTexEdge);

		glDeleteFramebuffers(1, &fboBlend);
		glDeleteTextures(1, &fboTexBlend);
	}
	
	glDeleteTextures(1, &skyboxTex);
	glDeleteTextures(textures.size(), textures.data());
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

	window = glfwCreateWindow(width, height, "RayTracing", fullScreen ? monitor : NULL, NULL);
	glfwGetWindowSize(window, &width, &height);

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

	float quadVertices[] = 
	{
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

	// SMAA framebuffers
	if (SMAA_enabled)
	{
		gen_framebuffer(&fboColor, &fboTexColor, GL_RGBA, GL_RGBA);
		gen_framebuffer(&fboEdge, &fboTexEdge, GL_RG, GL_RG);
		gen_framebuffer(&fboBlend, &fboTexBlend, GL_RGBA, GL_RGBA);
	}

	return true;
}

void GLWrapper::set_skybox(unsigned textureId)
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

void GLWrapper::enable_SMAA(SMAA_PRESET preset)
{
	SMAA_enabled = true;
	SMAA_preset = preset;
}

void GLWrapper::draw()
{
	shader.use();
	glBindVertexArray(quadVAO);
	if (SMAA_enabled) 
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fboColor);
	}
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	checkGlErrors("Draw raytraced image");

	if (!SMAA_enabled)
	{
		return;
	}
	
	edgeShader.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboTexColor);
	glBindFramebuffer(GL_FRAMEBUFFER, fboEdge);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	checkGlErrors("Draw edge");
	
	blendShader.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboTexEdge);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, areaTex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, searchTex);
	glBindFramebuffer(GL_FRAMEBUFFER, fboBlend);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	checkGlErrors("Draw blend");

	neighborhoodShader.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fboTexColor);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fboTexBlend);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	checkGlErrors("Draw screen");

	shader.use();
}

void GLWrapper::gen_framebuffer(GLuint* fbo, GLuint* fboTex, GLenum internalFormat, GLenum format) const
{
	glGenFramebuffers(1, fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

	glGenTextures(1, fboTex);
	glBindTexture(GL_TEXTURE_2D, *fboTex);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *fboTex, 0);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		exit(1);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLWrapper::init_shaders(rt_defines& defines)
{
	const std::string vertexShaderSrc = readStringFromFile(ASSETS_DIR "/shaders/quad.vert");
	std::string fragmentShaderSrc = readStringFromFile(ASSETS_DIR "/shaders/rt.frag");
	
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

	if (SMAA_enabled)
	{
		SMAA_Builder smaaBuilder(width, height, SMAA_preset);
		smaaBuilder.init_edge_shader(edgeShader);
		smaaBuilder.init_blend_shader(blendShader);
		smaaBuilder.init_neighborhood_shader(neighborhoodShader);

		edgeShader.use();
		edgeShader.setInt("color_tex", 0);

		blendShader.use();
		blendShader.setInt("edge_tex", 0);
		blendShader.setInt("area_tex", 1);
		blendShader.setInt("search_tex", 2);

		neighborhoodShader.use();
		neighborhoodShader.setInt("color_tex", 0);
		neighborhoodShader.setInt("blend_tex", 1);

		areaTex = smaaBuilder.load_area_texture();
		searchTex = smaaBuilder.load_search_texture();
	}

	shader.use();

	checkGlErrors("Shader creation");
}

std::string GLWrapper::to_string(glm::vec3 v)
{
	return std::string().append("vec3(").append(std::to_string(v.x)).append(",").append(std::to_string(v.y)).append(",").append(std::to_string(v.z)).append(")");
}

unsigned int GLWrapper::load_cubemap(std::vector<std::string> faces, bool genMipmap)
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
	if (genMipmap) {
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, genMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

unsigned int GLWrapper::load_texture(char const* path, GLuint wrapMode)
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

GLuint GLWrapper::load_texture(int texNum, const char* name, const char* uniformName, GLuint wrapMode)
{
	const std::string path = ASSETS_DIR "/textures/" + std::string(name);
	const unsigned int tex = load_texture(path.c_str(), wrapMode);
	shader.setInt(uniformName, texNum);
	textures.push_back(tex);
	return tex;
}

void GLWrapper::init_buffer(GLuint* ubo, const char* name, int bindingPoint, size_t size, void* data) const
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

void GLWrapper::update_buffer(GLuint ubo, size_t size, void* data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

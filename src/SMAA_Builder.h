#pragma once

#include <string>
#include "utils.h"
#include "shader.h"
#include "AreaTex.h"
#include "SearchTex.h"

enum SMAA_PRESET
{
	LOW, MEDIUM, HIGH, ULTRA
};

class SMAA_Builder
{
public:
	SMAA_Builder(int width, int height, SMAA_PRESET preset)
	{
		smaa_shader_body = readStringFromFile(ASSETS_DIR "/shaders/SMAA.h");

		std::string preset_str;
		switch (preset)
		{
			case LOW: preset_str = "SMAA_PRESET_LOW"; break;
			case MEDIUM: preset_str = "SMAA_PRESET_MEDIUM"; break;
			case HIGH: preset_str = "SMAA_PRESET_HIGH"; break;
			case ULTRA: preset_str = "SMAA_PRESET_ULTRA"; break;
		}

		char buff[256];
		snprintf(buff, sizeof(buff),
			"\n#define SMAA_RT_METRICS float4(1.0 / %d.0, 1.0 / %d.0, %d.0, %d.0)\n#define %s", width, height, width, height, preset_str.c_str());
		header = glsl_header + std::string(buff);
	}
	
	void init_edge_shader(Shader& shader) const
	{
		shader.initFromSrc(create_edge_vs(), create_edge_ps());
	}

	void init_blend_shader(Shader& shader) const
	{
		shader.initFromSrc(create_blend_vs(), create_blend_ps());
	}

	void init_neighborhood_shader(Shader& shader) const
	{
		shader.initFromSrc(create_neighborhood_vs(), create_neighborhood_ps());
	}

	static GLuint load_area_texture()
	{
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, AREATEX_WIDTH, AREATEX_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, areaTexBytes);
		glBindTexture(GL_TEXTURE_2D, 0);

		return tex;
	}

	static GLuint load_search_texture()
	{
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, searchTexBytes);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		return tex;
	}
	
private:
	std::string smaa_shader_body;
	std::string header;
	
	std::string create_edge_vs() const
	{
		return header + header_vs + smaa_shader_body + edge_vs;
	}

	std::string create_edge_ps() const
	{
		return header + header_ps + smaa_shader_body + edge_ps;
	}

	std::string create_blend_vs() const
	{
		return header + header_vs + smaa_shader_body + blend_vs;
	}

	std::string create_blend_ps() const
	{
		return header + header_ps + smaa_shader_body + blend_ps;
	}

	std::string create_neighborhood_vs() const
	{
		return header + header_vs + smaa_shader_body + neighborhood_vs;
	}

	std::string create_neighborhood_ps() const
	{
		return header + header_ps + smaa_shader_body + neighborhood_ps;
	}
	
	const std::string glsl_header = R"X(
#version 330
#define SMAA_GLSL_3
)X";

	const std::string header_vs = R"X(
#define SMAA_INCLUDE_PS 0

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
)X";

	const std::string header_ps = R"X(
#define SMAA_INCLUDE_VS 0
)X";

	const std::string edge_vs = R"X(
out vec2 texCoord;
out vec4 offset[3];
void main()
{
	texCoord = aTexCoords;
	SMAAEdgeDetectionVS(texCoord, offset);
	gl_Position = vec4(aPos, 0, 1);
}
)X";

	const std::string edge_ps = R"X(
uniform sampler2D color_tex;
in vec2 texCoord;
in vec4 offset[3];
out vec2 FragColor;
void main()
{
	FragColor = SMAALumaEdgeDetectionPS(texCoord, offset, color_tex);
}
)X";

	const std::string blend_vs = R"X(
out vec2 texCoord;
out vec2 pixCoord;
out vec4 offset[3];
void main()
{
	texCoord = aTexCoords;
	SMAABlendingWeightCalculationVS(texCoord, pixCoord, offset);
	gl_Position = vec4(aPos, 0, 1);
}
)X";

	const std::string blend_ps = R"X(
uniform sampler2D edge_tex;
uniform sampler2D area_tex;
uniform sampler2D search_tex;
in vec2 texCoord;
in vec2 pixCoord;
in vec4 offset[3];
out vec4 FragColor;
void main()
{
	FragColor = SMAABlendingWeightCalculationPS(texCoord, pixCoord, offset, edge_tex, area_tex, search_tex, vec4(0));
}
)X";

	const std::string neighborhood_vs = R"X(
out vec2 texCoord;
out vec4 offset;
void main()
{
	texCoord = aTexCoords;
	SMAANeighborhoodBlendingVS(texCoord, offset);
	gl_Position = vec4(aPos, 0, 1);
}
)X";

	const std::string neighborhood_ps = R"X(
uniform sampler2D color_tex;
uniform sampler2D blend_tex;
in vec2 texCoord;
in vec4 offset;
out vec4 FragColor;
void main()
{
	FragColor = SMAANeighborhoodBlendingPS(texCoord, offset, color_tex, blend_tex);
}
)X";
};
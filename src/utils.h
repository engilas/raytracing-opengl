#pragma once

#include <glad/glad.h>
#include <string>
#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>

static void readBytesFromFile(const char* path, std::vector<char> & buffer) {
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		file.open(path, std::ios_base::binary);
		std::vector<char> bytes(
			(std::istreambuf_iterator<char>(file)),
			(std::istreambuf_iterator<char>()));
		buffer.insert(std::end(buffer), std::begin(bytes), std::end(bytes));
		file.close();
	}
	catch (std::ifstream::failure & e)
	{
		std::cout << "ERROR::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
		exit(1);
	}
}

static std::string readStringFromFile(const char* path) {
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
	catch (std::ifstream::failure & e)
	{
		std::cout << "ERROR::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
		exit(1);
	}
	return content;
}

static bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

static void checkGlErrors(std::string desc)
{
	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		fprintf(stderr, "OpenGL error in \"%s\": (%d)\n", desc.c_str(), e); //todo error must be here
		exit(20);
	}
}
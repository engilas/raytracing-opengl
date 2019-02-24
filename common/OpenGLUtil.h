#ifndef __OPENGL_UTIL_H__
#define __OPENGL_UTIL_H__

#include <glad/glad.h>

typedef struct {
    GLuint vertex;
    GLuint fragment;
} shaders_t;

char* loadFile(const char *fname, GLint &fSize);

// printShaderInfoLog
// From OpenGL Shading Language 3rd Edition, p215-216
// Display (hopefully) useful error messages if shader fails to compile
void printShaderInfoLog(GLint shader);

void printLinkInfoLog(GLint prog);

shaders_t loadShaders(const char * vert_path, const char * frag_path);

void attachAndLinkProgram( GLuint program, shaders_t shaders);

GLuint initShaders(const char* vshaderpath, const char* fshaderpath);

GLuint createTexture2D(int width, int height, void* data=0);

GLuint createBuffer(int size, const float* data, GLenum usage);

#endif//__OPENGL_UTIL_H__

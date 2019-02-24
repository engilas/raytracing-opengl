#include "OpenGLUtil.h"

#include <iostream>
#include <fstream>

char* loadFile(const char *fname, GLint &fSize)
{
    std::ifstream file (fname,std::ios::in|std::ios::binary|std::ios::ate);
    if (file.is_open())
    {
        unsigned int size = (unsigned int)file.tellg();
        fSize = size;
        char *memblock = new char [size];
        file.seekg (0, std::ios::beg);
        file.read (memblock, size);
        file.close();
        std::cout << "file " << fname << " loaded" << std::endl;
        return memblock;
    }

    std::cout << "Unable to open file " << fname << std::endl;
    return NULL;
}

void printShaderInfoLog(GLint shader)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    if (infoLogLen > 1)
    {
        infoLog = new GLchar[infoLogLen];
        glGetShaderInfoLog(shader,infoLogLen, &charsWritten, infoLog);
        std::cout << "InfoLog:" << std::endl << infoLog << std::endl;
        delete [] infoLog;
    }
}

void printLinkInfoLog(GLint prog)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

    if (infoLogLen > 1)
    {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        glGetProgramInfoLog(prog,infoLogLen, &charsWritten, infoLog);
        std::cout << "InfoLog:" << std::endl << infoLog << std::endl;
        delete [] infoLog;
    }
}

shaders_t loadShaders(const char * vert_path, const char * frag_path) {
    GLuint f, v;

    char *vs,*fs;

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);

    // load shaders & get length of each
    GLint vlen;
    GLint flen;
    vs = loadFile(vert_path,vlen);
    fs = loadFile(frag_path,flen);

    const char * vv = vs;
    const char * ff = fs;

    glShaderSource(v, 1, &vv,&vlen);
    glShaderSource(f, 1, &ff,&flen);

    GLint compiled;

    glCompileShader(v);
    glGetShaderiv(v, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        std::cout << "Vertex shader not compiled." << std::endl;
    }
    printShaderInfoLog(v);

    glCompileShader(f);
    glGetShaderiv(f, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        std::cout << "Fragment shader not compiled." << std::endl;
    }
    printShaderInfoLog(f);

    shaders_t out; out.vertex = v; out.fragment = f;

    delete [] vs; // dont forget to free allocated memory
    delete [] fs; // we allocated this in the loadFile function...

    return out;
}

void attachAndLinkProgram( GLuint program, shaders_t shaders) {
    glAttachShader(program, shaders.vertex);
    glAttachShader(program, shaders.fragment);

    glLinkProgram(program);
    GLint linked;
    glGetProgramiv(program,GL_LINK_STATUS, &linked);
    if (!linked)
    {
        std::cout << "Program did not link." << std::endl;
    }
    printLinkInfoLog(program);
}

GLuint initShaders(const char* vshaderpath, const char* fshaderpath)
{
    shaders_t shaders = loadShaders(vshaderpath, fshaderpath);
    GLuint shader_program = glCreateProgram();
    attachAndLinkProgram(shader_program, shaders);
    return shader_program;
}

GLuint createTexture2D(int width, int height, void* data)
{
    GLuint ret_val = 0;
    glGenTextures(1,&ret_val);
    glBindTexture(GL_TEXTURE_2D,ret_val);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_FLOAT,data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0);
    return ret_val;
}

GLuint createBuffer(int size, const float* data, GLenum usage)
{
    GLuint ret_val = 0;
    glGenBuffers(1,&ret_val);
    glBindBuffer(GL_ARRAY_BUFFER,ret_val);
    glBufferData(GL_ARRAY_BUFFER,size*sizeof(float),data,usage);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    return ret_val;
}

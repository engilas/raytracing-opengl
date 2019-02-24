#ifndef __OPENCL_UTIL_H__
#define __OPENCL_UTIL_H__

#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"

#if defined (__APPLE__) || defined(MACOSX)
static const std::string CL_GL_SHARING_EXT = "cl_APPLE_gl_sharing";
#else
static const std::string CL_GL_SHARING_EXT = "cl_khr_gl_sharing";
#endif

static const std::string NVIDIA_PLATFORM = "NVIDIA";
static const std::string AMD_PLATFORM = "AMD";
static const std::string MESA_PLATFORM = "Clover";
static const std::string INTEL_PLATFORM = "Intel";
static const std::string APPLE_PLATFORM = "Apple";

cl::Platform getPlatform(std::string pName, cl_int &error);

cl::Platform getPlatform();

bool checkExtnAvailability(cl::Device pDevice, std::string pName);

cl::Program getProgram(cl::Context pContext, std::string file, cl_int &error);

#endif//__OPENCL_UTIL_H__

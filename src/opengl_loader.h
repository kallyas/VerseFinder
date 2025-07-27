#ifndef OPENGL_LOADER_H
#define OPENGL_LOADER_H

#ifdef IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <GL/gl.h>
#endif
#endif

#endif // OPENGL_LOADER_H
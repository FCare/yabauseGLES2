#ifndef SHARED_H
#define SHARED_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

typedef struct {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    EGLConfig config;
    EGLint numConfigs;
} Context;

Context create_context(EGLDisplay display, EGLint attribList[], EGLint contextAttribs[]);
EGLSurface create_surface_from_shared_pixmap(EGLDisplay display, EGLConfig config, EGLint* global_image, int width, int height);
GLuint create_texture_from_shared_pixmap(EGLDisplay display, EGLint* global_image);
EGLImageKHR create_image_from_shared_pixmap(EGLDisplay display, EGLint* global_image);
GLuint create_texture_from_image(EGLImageKHR image);
EGLNativeWindowType create_native_window(int* width, int* height);
void context_make_current(Context* context);
void destroy_context(Context* context);
void destroy_native_window();
GLint create_gl_program(const char* vertexShaderSource, const char* fragmentShaderSource);

#endif

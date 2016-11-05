#ifndef __GL_ES20_UTILS_H__
#define __GL_ES20_UTILS_H__

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdlib.h>
#include <stdio.h>

typedef struct fbo_s {
    int fb;
    int tex;
    int stencil;
    int width;
    int height;
} gl_fbo;


extern int gles20_createFBO(gl_fbo* fbo, int w, int h);
extern int gles20_createProgram(GLbyte* vShader, GLbyte* fShader);

#endif

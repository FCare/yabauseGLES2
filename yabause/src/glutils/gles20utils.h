#ifndef __GL_ES20_UTILS_H__
#define __GL_ES20_UTILS_H__

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdlib.h>
#include <stdio.h>


extern int gles20_createFBO();
extern int gles20_createProgram(GLbyte* vShader, GLbyte* fShader);

#endif

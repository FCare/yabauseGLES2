#ifndef __GLES20_PROGRAM_H__
#define __GLES20_PROGRAM_H__

#include "../patternManager.h"
#include "gles20utils.h"
#include "../asyncRenderer.h"

void drawPattern(Pattern* pattern, GLfloat* vertex,  render_context *ctx);
void drawPriority(Pattern* pattern, GLfloat* vertex, int priority, render_context *ctx);
void createPatternProgram( render_context *ctx);
void createPriorityProgram( render_context *ctx);

#endif

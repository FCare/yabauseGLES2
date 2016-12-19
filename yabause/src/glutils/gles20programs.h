#ifndef __GLES20_PROGRAM_H__
#define __GLES20_PROGRAM_H__

#include "../patternManager.h"
#include "gles20utils.h"

void drawPattern(Pattern* pattern, GLfloat* vertex, int index);
void drawPriority(Pattern* pattern, GLfloat* vertex, int priority, int index);
void createPatternProgram();
void createPriorityProgram();

void prepareSpriteRenderer();
void prepareSpriteRenderer();
void updateRendererVertex(GLfloat *vert, int size);

#endif

#ifndef __GLES20_PROGRAM_H__
#define __GLES20_PROGRAM_H__

#include "../patternManager.h"
#include "gles20utils.h"

void drawPattern(Pattern* pattern, GLfloat* vertex);
void drawPriority(Pattern* pattern, GLfloat* vertex, int priority);
void createPatternProgram();
void createPriorityProgram();

#endif

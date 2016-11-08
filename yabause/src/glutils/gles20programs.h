#ifndef __GLES20_PROGRAM_H__
#define __GLES20_PROGRAM_H__

#include "../patternManager.h"
#include "gles20utils.h"

void drawPattern(Pattern* pattern, GLfloat* vertex, int nbVertex);
void drawPriority(Pattern* pattern, GLfloat* vertex, int priority, int nbVertex);
void createPatternProgram();
void createPriorityProgram();

#endif

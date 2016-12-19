#ifndef __ASYNC_RENDERER_H__
#define __ASYNC_RENDERER_H__

#include "glutils/gles20utils.h"
#include "patternManager.h"

typedef enum {
	VDP1START,
	VDP1QUAD,
	VDP1STOP,
} RenderingOperation;

typedef struct s_vdp1renderer vdp1RenderList;

struct s_vdp1renderer{
  RenderingOperation op;
  GLfloat* vertices;
  int vertIndex;
  int nbVertices;
  Pattern* pattern;
  int priority;
  struct s_vdp1renderer* next;
  struct s_vdp1renderer* previous;
};

void addToVdp1Renderer(Pattern* pattern, RenderingOperation op, const float* vertices, int nbvertices, int prio);
void addVdp1Renderer(RenderingOperation op);

#endif

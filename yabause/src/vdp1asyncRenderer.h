#ifndef __ASYNC_RENDERER_H__
#define __ASYNC_RENDERER_H__

#include "glutils/gles20utils.h"
#include "patternManager.h"

typedef enum {
	VDP1QUAD,
} RenderingOperation;

typedef struct s_vdp1renderer vdp1RenderList;

struct s_vdp1renderer{
  RenderingOperation op;
  GLfloat* vertices;
  int nbVertices;
  Pattern* pattern;
  int priority;
  struct s_vdp1renderer* next;
  struct s_vdp1renderer* previous;
};

#endif

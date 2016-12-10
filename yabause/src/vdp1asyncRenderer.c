#include <stdlib.h>
#include "vdp1asyncRenderer.h"
#include "glutils/gles20programs.h"

static vdp1RenderList *mStartRenderList;
static vdp1RenderList *mEndRenderList;

static int priorityfb;
static int spritefb;
static int width;
static int height;

#define PATTERNMAX 20000

static GLfloat mergedVertices[PATTERNMAX];
static int sizeVertices;

static void executeOpSprite(vdp1RenderList* cur) {
	switch (cur->op) {
		case (VDP1QUAD):
			drawPattern(cur->pattern, cur->vertices, cur->vertIndex);
			break;
	}
}

static void executeOpPriority(vdp1RenderList* cur) {
	switch (cur->op) {
		case (VDP1QUAD):
			drawPriority(cur->pattern, cur->vertices, cur->priority, cur->vertIndex);
			break;
	}
}
 
static void clearList() {
	vdp1RenderList* curList;
	vdp1RenderList* del;
	curList = mStartRenderList;
	mStartRenderList = NULL;
	mEndRenderList = NULL;
	while (curList != NULL) {
		del = curList;
		curList = curList->next;
		pushCachePattern(del->pattern);
		free(del->vertices);
		free(del);
	}
}


void addVdp1Renderer(RenderingOperation op) {
	if ((op != VDP1START) && (op != VDP1STOP)) return;
	switch (op){
		case (VDP1START):
			sizeVertices = 0;
			break;
		case (VDP1STOP): {
				vdp1RenderList* curList = mStartRenderList;
				int i = 0;
				if (sizeVertices > PATTERNMAX) printf("Damned\n");
				while (curList != NULL) {
					memcpy(&mergedVertices[i], curList->vertices, curList->nbVertices*sizeof(GLfloat));
					curList->vertIndex = i;
					i += curList->nbVertices;
					curList = curList->next;
				}
				updateRendererVertex(mergedVertices, sizeVertices);
				renderVdp1();
			}
			break;
	}
}

void addToVdp1Renderer(Pattern* pattern, RenderingOperation op, const float* vertices, int nbvertices, int prio) {
	vdp1RenderList* curList;
	if (pattern == NULL) return;
	curList = calloc(1, sizeof *curList);
	curList->op = op;
	curList->vertices = calloc(nbvertices,sizeof *curList->vertices);
	memcpy(curList->vertices, vertices, nbvertices*sizeof *curList->vertices);
	curList->nbVertices = nbvertices;
	sizeVertices += nbvertices;
	curList->pattern = pattern;
	curList->priority = prio;
	curList->next = NULL;
	if (mStartRenderList == NULL) {
		mStartRenderList = curList;
	}
	if (mEndRenderList == NULL) {
		mEndRenderList = curList;
	} else {
		curList->previous = mEndRenderList;
		mEndRenderList->next = curList;
		mEndRenderList = curList;
	}
}

void renderVdp1() {
	vdp1RenderList* curList = mStartRenderList;

	glBindFramebuffer(GL_FRAMEBUFFER, spritefb);
        glViewport(0,0,width,height);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	prepareSpriteRenderer();
	while (curList != NULL) {
		executeOpSprite(curList);
		curList = curList->next;
	}
	curList = mStartRenderList;

	glBindFramebuffer(GL_FRAMEBUFFER, priorityfb);
        glViewport(0,0,width,height);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	preparePriorityRenderer();
	while (curList != NULL) {
		executeOpPriority(curList);
		curList = curList->next;
	}
	clearList();
}

void setupVdp1(int fb, int prio, int w, int h) {
	priorityfb = prio;
	spritefb = fb;
	width = w;
	height = h;
}


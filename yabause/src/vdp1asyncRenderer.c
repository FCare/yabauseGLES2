#include "vdp1asyncRenderer.h"
#include "glutils/gles20programs.h"

static vdp1RenderList *mStartRenderList;
static vdp1RenderList *mEndRenderList;

static int priorityfb;
static int spritefb;
static int width;
static int height;

static void executeOpSprite(vdp1RenderList* cur) {
	switch (cur->op) {
		case (VDP1QUAD):
			if (cur->nbVertices == 20)
				drawPattern(cur->pattern, cur->vertices);
			break;
	}
}

static void executeOpPriority(vdp1RenderList* cur) {
	switch (cur->op) {
		case (VDP1QUAD):
			if (cur->nbVertices == 20)
				drawPriority(cur->pattern, cur->vertices, cur->priority);
			break;
	}
}
 
static void clearList() {
	vdp1RenderList* curList;
	vdp1RenderList* del;
	del = mStartRenderList;
	mStartRenderList = NULL;
	mEndRenderList = NULL;
	while (del != NULL) {
		curList = del;
		if (del->pattern->managed == 0) deleteCachePattern(del->pattern);
		free(del->vertices);
		free(del);
		del = curList->next;
	}
}


void addToVdp1Renderer(Pattern* pattern, RenderingOperation op, GLfloat* vertices, int nbvertices, int prio) {
	vdp1RenderList* curList;
	vdp1RenderList* last;
	if (pattern == NULL) return;
	curList = (vdp1RenderList*) calloc(sizeof(vdp1RenderList),1);
	curList->op = op;
	curList->vertices = (GLfloat*)malloc(nbvertices*sizeof(GLfloat));
	memcpy(curList->vertices, vertices, nbvertices*sizeof(GLfloat));
	curList->nbVertices = nbvertices;
	curList->pattern = pattern;
	curList->priority = prio;
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


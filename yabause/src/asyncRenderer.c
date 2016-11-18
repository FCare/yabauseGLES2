#include "asyncRenderer.h"

controledList mFrameList;
controledList mRenderList;

struct
{
   volatile int need_draw[5];
   volatile int draw_finished[5];
} frame_render_thread_context;


renderingStack* removeFromList(controledList* clist);
void executeOp(render_context *ctx, RenderingOperation op);
int initRender_context(render_context *ctx);
void setupCtxFromFrame(render_context *ctx, renderingStack* frame);

#define DECLARE_FRAME_RENDER_THREAD(FUNC_NAME, THREAD_NUMBER) \
void FUNC_NAME(void* data) \
{ \
   render_context *ctx = (render_context *)calloc(sizeof(render_context), 1); \
   ctx->glContext = -1;\
   for (;;) \
   { \
	renderingStack* frame = removeFromList(&mRenderList); \
	if (frame != NULL) { \
		if (ctx->glContext == -1) ctx->glContext = SDL_GL_CreateContext(frame->tt_context->glWindow); \
		setupCtxFromFrame(ctx, frame); \
	   	if (initRender_context(ctx)!= 0) { \
			printf("Error during init of frame render thread\n"); \
			return; \
	   	} \
		while (frame->operation != NULL) { \
			operationList* tmp = frame->operation; \
			frame->operation = frame->operation->next; \
			executeOp(ctx, tmp->current); \
			free(tmp); \
		} \
		glFinish(); \
		SDL_GL_MakeCurrent(frame->tt_context->glWindow, NULL); \
		PushFrameToDisplay(ctx); \
		releaseRenderingStack(frame); \
	} \
   } \
}

#if NB_GL_RENDERER >= 1
DECLARE_FRAME_RENDER_THREAD(frameRenderThread0, 0);
#endif
#if NB_GL_RENDERER >= 2
DECLARE_FRAME_RENDER_THREAD(frameRenderThread1, 1);
#endif
#if NB_GL_RENDERER >= 3
DECLARE_FRAME_RENDER_THREAD(frameRenderThread2, 2);
#endif

void setupCtxFromFrame(render_context *ctx, renderingStack* frame) {
	ctx->Vdp2Regs = frame->Vdp2Regs;
	ctx->Vdp2Ram = frame->Vdp2Ram;
	ctx->Vdp1Regs = frame->Vdp1Regs;
	ctx->Vdp2Lines = frame->Vdp2Lines;
	ctx->Vdp2ColorRam = frame->Vdp2ColorRam;
	ctx->cell_scroll_data = frame->cell_scroll_data;
	ctx->tt_context = frame->tt_context;
	ctx->frameId = frame->id;
}

int initRender_context(render_context *ctx) {
	if (TitanGLInit(ctx) != 0) printf("Error TitanGLInit\n");
   createPatternProgram();
   createPriorityProgram();
	memset(ctx->bad_cycle_setting, 0, 6*sizeof(int));
	return 0;
}

void executeOp(render_context *ctx, RenderingOperation op) {
	switch (op) {
		case (VDP2START):
			//printf("VDP2START\n");
			FrameVdp2DrawStart(ctx);
			break;
		case (VDP2END):
			//printf("VDP2END\n");
			FrameVdp2DrawEnd(ctx);
			break;
		case (VDP2SCREENS):
			//printf("VDP2SCREENS\n");
			FrameVdp2DrawScreens(ctx);
			break;
		case (VDP1START):
			//printf("VDP1START\n");
			FrameVdp1DrawStart(ctx);
			break;
	}
}

void addToList(renderingStack* stack, controledList* clist) {
	list* curList;
	while (sem_wait(&clist->lock) != 0);
	curList = (list*) calloc(sizeof(list),1);
	curList->current = stack;
	curList->next = clist->list;
	clist->list = curList;
	while (sem_post(&clist->lock) != 0);
	while (sem_post(&clist->elem) != 0);
}

renderingStack* removeFromList(controledList* clist) {
	renderingStack* cur;
	list* tbd;
	while (sem_wait(&clist->elem) != 0);
	while (sem_wait(&clist->lock) != 0);
	cur = clist->list->current;
	tbd = clist->list;
	clist->list = clist->list->next;
	free(tbd);
	while (sem_post(&clist->lock) != 0);
	return cur;
}

renderingStack* createRenderingStacks(int nb, SDL_Window *gl_window, SDL_GLContext *gl_context) {
	int i;
	renderingStack* render = (renderingStack*)calloc(sizeof(renderingStack), nb);
	sem_init(&mFrameList.lock, 0, 1);
	sem_init(&mFrameList.elem, 0, 0);
	sem_init(&mRenderList.lock, 0, 1);
	sem_init(&mRenderList.elem, 0, 0);
	for (i=0; i < nb; i++) {
		render[i].id = -1;
		render[i].fb = (u8 *)calloc(sizeof(u8), 0x40000);
		render[i].Vdp2Regs = (Vdp2*)calloc(sizeof(Vdp2), 1);
		render[i].Vdp2Lines = (Vdp2*)calloc(sizeof(Vdp2), 270);
		render[i].Vdp1Regs = (Vdp1*)calloc(sizeof(Vdp1), 1);
		render[i].Vdp2Ram = (u8*)calloc(0x80000, 1);
		render[i].Vdp2ColorRam = (u8*)calloc(0x1000, 1);
		render[i].cell_scroll_data = (struct CellScrollData*) calloc(sizeof(struct CellScrollData), 270);
		render[i].operation = NULL;
		render[i].tt_context = (struct TitanGLContext*) calloc(sizeof(struct TitanGLContext), 1);
		render[i].tt_context->glWindow = gl_window;
		addToList(&render[i], &mFrameList);
	}
#if NB_GL_RENDERER >= 1
	YabThreadStart(YAB_THREAD_VIDSOFT_FRAME_RENDER_0, frameRenderThread0, NULL);
#endif
#if NB_GL_RENDERER >= 2
      	YabThreadStart(YAB_THREAD_VIDSOFT_FRAME_RENDER_1, frameRenderThread1, NULL);
#endif
#if NB_GL_RENDERER >= 3
      	YabThreadStart(YAB_THREAD_VIDSOFT_FRAME_RENDER_2, frameRenderThread2, NULL);
#endif
}

renderingStack* getFrame() {
	return removeFromList(&mFrameList);
}

void releaseRenderingStack(renderingStack* old) {
	addToList(old, &mFrameList);
}

void initRenderingStack(renderingStack* stack, int id, Vdp2* Vdp2Regs, u8* Vdp2Ram,Vdp1* Vdp1Regs,Vdp2* Vdp2Lines,u8* Vdp2ColorRam)
{
	stack->id = id;
	memcpy(stack->Vdp2Regs, Vdp2Regs, sizeof(Vdp2));
	memcpy(stack->Vdp2Lines, Vdp2Lines, sizeof(Vdp2)*270);
	memcpy(stack->Vdp1Regs, Vdp1Regs, sizeof(Vdp1));
	memcpy(stack->Vdp2Ram, Vdp2Ram, 0x80000);
	memcpy(stack->Vdp2ColorRam, Vdp2ColorRam, 0x1000);
	memcpy(stack->cell_scroll_data, cell_scroll_data, 270*sizeof(struct CellScrollData));
}

renderingStack* addOperation(renderingStack* stack, RenderingOperation op) {
	operationList* lastOp;	
	if (stack == NULL) return NULL;
	lastOp = stack->operation;
	if (lastOp == NULL)
	{
		stack->operation = (operationList*)calloc(sizeof(operationList), 1);
		stack->operation->current = op;
		stack->operation->next = NULL;
	} else {
		while (lastOp->next != NULL) lastOp = lastOp->next;	
		lastOp->next = (operationList*)calloc(sizeof(operationList), 1);
		lastOp->next->current = op;
		lastOp->next->next = NULL;
	}
	if (op == VDP2END) {
		addToList(stack, &mRenderList);
		return NULL;
	}
	return stack;
}



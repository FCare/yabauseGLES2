#ifndef __ASYNC_RENDERER_H__
#define __ASYNC_RENDERER_H__

#include <semaphore.h>
#include "vdp2.h"
#include "vdp1.h"
#include "glutils/gles20utils.h"
#include "threads.h"
#include "titangl/titangl.h"

typedef enum {
	VDP2START,
	VDP2END,
	VDP2SCREENS,
	VDP1START
} RenderingOperation;

typedef struct {
	int bad_cycle_setting[6];
	Vdp2* Vdp2Regs;
	u8* Vdp2Ram; //0x80000
	Vdp1* Vdp1Regs;
	Vdp2* Vdp2Lines;
	u8* Vdp2ColorRam; //0x1000
	struct CellScrollData *cell_scroll_data;
	struct TitanGLContext* tt_context;
	int frameId;
	sem_t frameDisplayed;
} render_context;

typedef struct s_operation operationList;

struct s_operation{
  RenderingOperation current;
  struct s_operation* next;
};

typedef struct {
	int id;
	operationList* operation;
	Vdp2* Vdp2Regs;
	u8* Vdp2Ram; //0x80000
	Vdp1* Vdp1Regs;
	Vdp2* Vdp2Lines;
	u8* Vdp2ColorRam; //0x1000
	struct CellScrollData *cell_scroll_data;
	u8* fb;
	struct TitanGLContext* tt_context;
	SDL_GLContext *glContext;
	SDL_Window *glWindow;
} renderingStack;

typedef struct s_List list;
struct s_List{
  renderingStack* current;
  struct s_List* next;
};

typedef struct {
	list* list;
	sem_t lock;
	sem_t elem;
} controledList;

typedef struct {
	gl_fbo *fbo;
	int id;
	sem_t *done;
} numberedFrame;

typedef struct s_Fbo renderFrame;
struct s_Fbo{
  numberedFrame *current;
  struct s_Fbo* next;
  struct s_Fbo* previous;
};

typedef struct {
	renderFrame* frame;
	sem_t lock;
	sem_t elem;
} controledFbo;




renderingStack* createRenderingStacks(int nb, SDL_Window *glWindow, SDL_GLContext *gl_context);


#endif

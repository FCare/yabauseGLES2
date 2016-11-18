/*  Copyright 2012 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef TITANGL_H
#define TITANGL_H

#include <SDL2/SDL.h>
#include <semaphore.h>
#include "../vdp2.h"
#include "../vdp1.h"
#include "../core.h"
#include "../vidshared.h"
#include "../glutils/gles20utils.h"

#define TITAN_BLEND_TOP     0
#define TITAN_BLEND_BOTTOM  1
#define TITAN_BLEND_ADD     2

#define TITAN_SPRITE 5
#define TITAN_RBG0 4
#define TITAN_NBG0 3
#define TITAN_NBG1 2
#define TITAN_NBG2 1
#define TITAN_NBG3 0
#define TITAN_BACK -1

#define TITAN_NORMAL_SHADOW 1
#define TITAN_MSB_SHADOW 2

struct StencilData{
   u8 linescreen : 2 ;
   u8 shadow_type : 2 ;
   u8 shadow_enabled : 1;
};
typedef struct {
	u8* fb;
	gl_fbo fbo;	
	gl_fbo priority;
} framebuffer;

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
        SDL_GLContext glContext;
} render_context;

extern sem_t lockGL;

typedef u32 (*TitanGLBlendFunc)(u32 top, u32 bottom);
typedef int FASTCALL (*TitanGLTransFunc)(u32 pixel);
typedef u32 PixelData;

struct TitanGLContext {
   int inited;
   PixelData * vdp2framebuffer[6];
   struct StencilData * vdp2stencil[6];
   u8 *vdp2priority[6];
   int vdp2fbo[6];
   int vdp2prio[6];
   u32 * linescreen[4];
   int vdp2width;
   int vdp2height;
   int glwidth;
   int glheight;
   gl_fbo fbo;
   framebuffer* vdp1framebuffer;
   TitanGLBlendFunc blend;
   TitanGLTransFunc trans;
   PixelData * backscreen;
   int layer_priority[6];
   GLuint g_VertexSWBuffer;
   GLuint titanBackProg;
   GLuint positionLoc;
   GLuint texCoordLoc;
   GLuint stexCoordLoc;
   GLuint samplerLoc;
   GLuint backLoc;

   GLuint programGeneralPriority;
   GLuint posGPrioLoc;
   GLuint tCoordGPrioLoc;
   GLuint sCoordGPrioLoc;
   GLuint spriteLoc;
   GLuint layerLoc;
   GLuint prioLoc;
   GLuint refPrioLoc;

   SDL_Window *glWindow;	
};

int TitanGLInit(render_context *ctx);
int TitanGLDeInit(struct TitanGLContext *tt_context);
void TitanGLErase(struct TitanGLContext *tt_context);

void TitanGLSetResolution(int width, int height, struct TitanGLContext *tt_context);
void TitanGLGetResolution(int * width, int * height, struct TitanGLContext *tt_context);

void TitanGLSetBlendingMode(int blend_mode, struct TitanGLContext *tt_context);

void TitanGLPutBackHLine(s32 y, u32 color, struct TitanGLContext *tt_context);

void TitanGLPutLineHLine(int linescreen, s32 y, u32 color, struct TitanGLContext *tt_context);

void TitanGLPutPixel(int priority, s32 x, s32 y, u32 color, int linescreen, vdp2draw_struct* info, struct TitanGLContext *tt_context);
void TitanGLPutHLine(int priority, s32 x, s32 y, s32 width, u32 color, struct TitanGLContext *tt_context);

void TitanGLRenderFBO(render_context *ctx);
void TitanGLSetVdp2Fbo(int fb, int nb, struct TitanGLContext *tt_context);
void TitanGLSetVdp2Priority(int fb, int nb, struct TitanGLContext *tt_context);

#endif

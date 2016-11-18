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

#include "titangl.h"
#include "../vidshared.h"
#include "../vidsoft.h"

#include "../profiler.h"

#include <stdlib.h>
#include <semaphore.h>

/* private */

sem_t lockGL;

#if defined WORDS_BIGENDIAN
#ifdef USE_RGB_555
static INLINE u32 TitanGLFixAlpha(u32 pixel) { return (((pixel >> 27) & 0x1F) | ((pixel >> 14) & 0x7C0) | (pixel >> 1) & 0xF8); }
#elif USE_RGB_565
static INLINE u32 TitanGLFixAlpha(u32 pixel) { return (((pixel >> 27) & 0x1F) | ((pixel >> 13) & 0x7E0) | (pixel & 0xF8)); }
#else
static INLINE u32 TitanGLFixAlpha(u32 pixel) { return ((((pixel & 0x3F) << 2) + 0x03) | (pixel & 0xFFFFFF00)); }
#endif

static INLINE u8 TitanGLGetAlpha(u32 pixel) { return pixel & 0x3F; }
static INLINE u8 TitanGLGetRed(u32 pixel) { return (pixel >> 8) & 0xFF; }
static INLINE u8 TitanGLGetGreen(u32 pixel) { return (pixel >> 16) & 0xFF; }
static INLINE u8 TitanGLGetBlue(u32 pixel) { return (pixel >> 24) & 0xFF; }
static INLINE u32 TitanGLCreatePixel(u8 alpha, u8 red, u8 green, u8 blue) { return alpha | (red << 8) | (green << 16) | (blue << 24); }
#else
#ifdef USE_RGB_555
static INLINE u32 TitanGLFixAlpha(u32 pixel) { return (((pixel << 7) & 0x7C00) | ((pixel >> 6) & 0x3C0) | ((pixel >> 19) & 0x1F)); }
#elif USE_RGB_565
static INLINE u32 TitanGLFixAlpha(u32 pixel) { return (((pixel << 8) & 0xF800) | ((pixel >> 5) & 0x7C0) | ((pixel >> 19) & 0x1F)); }
#else
static INLINE u32 TitanGLFixAlpha(u32 pixel) { return ((((pixel & 0x3F000000) << 2) + 0x03000000) | (pixel & 0x00FFFFFF)); }
#endif

static INLINE u8 TitanGLGetAlpha(u32 pixel) { return (pixel >> 24) & 0x3F; }
static INLINE u8 TitanGLGetRed(u32 pixel) { return (pixel >> 16) & 0xFF; }
static INLINE u8 TitanGLGetGreen(u32 pixel) { return (pixel >> 8) & 0xFF; }
static INLINE u8 TitanGLGetBlue(u32 pixel) { return pixel & 0xFF; }
static INLINE u32 TitanGLCreatePixel(u8 alpha, u8 red, u8 green, u8 blue) { return (alpha << 24) | (red << 16) | (green << 8) | blue; }
#endif


void TitanGLSetVdp2Fbo(int fb, int nb, struct TitanGLContext *tt_context){
	tt_context->vdp2fbo[nb] = fb;
}

void TitanGLSetVdp2Priority(int fb, int nb, struct TitanGLContext *tt_context) {
	tt_context->vdp2prio[nb] = fb;
}

static u32 TitanGLBlendPixelsTop(u32 top, u32 bottom)
{
   u8 alpha, ralpha, tr, tg, tb, br, bg, bb;

   alpha = (TitanGLGetAlpha(top) << 2) + 3;
   ralpha = 0xFF - alpha;

   tr = (TitanGLGetRed(top) * alpha) / 0xFF;
   tg = (TitanGLGetGreen(top) * alpha) / 0xFF;
   tb = (TitanGLGetBlue(top) * alpha) / 0xFF;

   br = (TitanGLGetRed(bottom) * ralpha) / 0xFF;
   bg = (TitanGLGetGreen(bottom) * ralpha) / 0xFF;
   bb = (TitanGLGetBlue(bottom) * ralpha) / 0xFF;

   return TitanGLCreatePixel(0x3F, tr + br, tg + bg, tb + bb);
}

static u32 TitanGLBlendPixelsBottom(u32 top, u32 bottom)
{
   u8 alpha, ralpha, tr, tg, tb, br, bg, bb;

   if ((top & 0x80000000) == 0) return top;

   alpha = (TitanGLGetAlpha(bottom) << 2) + 3;
   ralpha = 0xFF - alpha;

   tr = (TitanGLGetRed(top) * alpha) / 0xFF;
   tg = (TitanGLGetGreen(top) * alpha) / 0xFF;
   tb = (TitanGLGetBlue(top) * alpha) / 0xFF;

   br = (TitanGLGetRed(bottom) * ralpha) / 0xFF;
   bg = (TitanGLGetGreen(bottom) * ralpha) / 0xFF;
   bb = (TitanGLGetBlue(bottom) * ralpha) / 0xFF;

   return TitanGLCreatePixel(TitanGLGetAlpha(top), tr + br, tg + bg, tb + bb);
}

static u32 TitanGLBlendPixelsAdd(u32 top, u32 bottom)
{
   u32 r, g, b;

   r = TitanGLGetRed(top) + TitanGLGetRed(bottom);
   if (r > 0xFF) r = 0xFF;

   g = TitanGLGetGreen(top) + TitanGLGetGreen(bottom);
   if (g > 0xFF) g = 0xFF;

   b = TitanGLGetBlue(top) + TitanGLGetBlue(bottom);
   if (b > 0xFF) b = 0xFF;

   return TitanGLCreatePixel(0x3F, r, g, b);
}

static INLINE int FASTCALL TitanGLTransAlpha(u32 pixel)
{
   return TitanGLGetAlpha(pixel) < 0x3F;
}

static INLINE int FASTCALL TitanGLTransBit(u32 pixel)
{
   return pixel & 0x80000000;
}

static u32 TitanGLDigPixel(int pos, int y, struct TitanGLContext *tt_context)
{
   PixelData pixel_stack[2] = { 0 };
   struct StencilData stencil_stack[2] = { 0 };
   u8 prio_stack[2] = { 0 };

   int pixel_stack_pos = 0;

   int priority;

   //sort the pixels from highest to lowest priority
   for (priority = 7; priority > 0; priority--)
   {
      int which_layer;

      for (which_layer = TITAN_SPRITE; which_layer >= 0; which_layer--)
      {
         if (tt_context->vdp2priority[which_layer][pos] == priority)
         {
            pixel_stack[pixel_stack_pos] = tt_context->vdp2framebuffer[which_layer][pos];
            stencil_stack[pixel_stack_pos] = tt_context->vdp2stencil[which_layer][pos];
	    prio_stack[pixel_stack_pos] = tt_context->vdp2priority[which_layer][pos];
            pixel_stack_pos++;

            if (pixel_stack_pos == 2)
               goto finished;//backscreen is unnecessary in this case
         }
      }
   }
   pixel_stack[pixel_stack_pos] = tt_context->backscreen[y];
   memset(&stencil_stack[pixel_stack_pos], 0, sizeof(struct StencilData));
   memset(&prio_stack[pixel_stack_pos], 0, sizeof(u8));


finished:
   if (stencil_stack[0].linescreen)
   {
      pixel_stack[0] = tt_context->blend(pixel_stack[0], tt_context->linescreen[stencil_stack[0].linescreen][y]);
   }

   if ((stencil_stack[0].shadow_type == TITAN_MSB_SHADOW) && ((pixel_stack[0] & 0xFFFFFF) == 0))
   {
      //transparent sprite shadow
      if (stencil_stack[1].shadow_enabled)
      {
         pixel_stack[0] = TitanGLBlendPixelsTop(0x20000000, pixel_stack[1]);
      }
      else
      {
         pixel_stack[0] = pixel_stack[1];
      }
   }
   else if (stencil_stack[0].shadow_type == TITAN_MSB_SHADOW && ((pixel_stack[0] & 0xFFFFFF) != 0))
   {
      if (tt_context->trans(pixel_stack[0]))
      {
         u32 bottom = pixel_stack[1];
         pixel_stack[0] = tt_context->blend(pixel_stack[0], bottom);
      }

      //sprite self-shadowing, only if sprite window is not enabled
      if (!(Vdp2Regs->SPCTL & 0x10))
         pixel_stack[0] = TitanGLBlendPixelsTop(0x20000000, pixel_stack[0]);
   }
   else if (stencil_stack[0].shadow_type == TITAN_NORMAL_SHADOW)
   {
      if (stencil_stack[1].shadow_enabled)
      {
         pixel_stack[0] = TitanGLBlendPixelsTop(0x20000000, pixel_stack[1]);
      }
      else
      {
         pixel_stack[0] = pixel_stack[1];
      }
   }
   else
   {
      if (tt_context->trans(pixel_stack[0]))
      {
         u32 bottom = pixel_stack[1];
         pixel_stack[0] = tt_context->blend(pixel_stack[0], bottom);
      }
   }

   return pixel_stack[0];
}

/* public */
int TitanGLInit(render_context *ctx)
{
   int i;
   if (ctx->tt_context->inited == 0)
   {
   	ctx->tt_context->g_VertexSWBuffer = 0;
   	ctx->tt_context->titanBackProg = 0;
   	ctx->tt_context->positionLoc = 0;
   	ctx->tt_context->texCoordLoc = 0;
   	ctx->tt_context->stexCoordLoc = 0;
   	ctx->tt_context->samplerLoc = 0;
   	ctx->tt_context->backLoc = 0;

   	ctx->tt_context->programGeneralPriority = 0;
   	ctx->tt_context->posGPrioLoc = 0;
   	ctx->tt_context->tCoordGPrioLoc = 0;
   	ctx->tt_context->sCoordGPrioLoc = 0;
   	ctx->tt_context->spriteLoc = 0;
   	ctx->tt_context->layerLoc = 0;
   	ctx->tt_context->prioLoc = 0;
   	ctx->tt_context->refPrioLoc = 0;

	gles20_createFBO(&ctx->tt_context->fbo, 704, 512, 0);
   	// Initialize VDP1 framebuffer
   	if ((ctx->tt_context->vdp1framebuffer = (framebuffer *)calloc(sizeof(framebuffer), 1)) == NULL)
      		return -1;

   	if ((ctx->tt_context->vdp1framebuffer->fb = (u8 *)calloc(sizeof(u8), 0x40000)) == NULL)
      		return -1;

	gles20_createFBO(&ctx->tt_context->vdp1framebuffer->fbo, 1024, 512, 0);
   	gles20_createFBO(&ctx->tt_context->vdp1framebuffer->priority, 1024, 512, 1);

      for(i = 0;i < 6;i++)
      {
         if ((ctx->tt_context->vdp2framebuffer[i] = (PixelData *)calloc(sizeof(PixelData), 704 * 256)) == NULL)
            printf("Error during vdp2framebuffer[%d] init\n", i);
         if ((ctx->tt_context->vdp2stencil[i] = (struct StencilData*)calloc(sizeof(struct StencilData), 704 * 256)) == NULL)
            printf("Error during vdp2stencil[%d] init\n", i);
         if ((ctx->tt_context->vdp2priority[i] = (u8*)calloc(sizeof(u8), 704 * 256)) == NULL)
            printf("Error during vdp2framebuffer[%d] init\n", i);
	 ctx->tt_context->vdp2fbo[i] = -1;
         ctx->tt_context->vdp2prio[i] = -1;
      }

      ctx->tt_context->glwidth = 0;
      ctx->tt_context->glheight= 0;

      /* linescreen 0 is not initialized as it's not used... */
      for(i = 1;i < 4;i++)
      {
         if ((ctx->tt_context->linescreen[i] = (u32 *)calloc(sizeof(u32), 512)) == NULL)
            return -1;
      }

      if ((ctx->tt_context->backscreen = (PixelData  *)calloc(sizeof(PixelData), 512)) == NULL)
         return -1;

      createGLPrograms(ctx);
      ctx->tt_context->inited = 1;
   }

   for(i = 0;i < 6;i++) {
      if (ctx->tt_context->vdp2framebuffer[i] != NULL) memset(ctx->tt_context->vdp2framebuffer[i], 0, sizeof(u32) * 704 * 256);
      if (ctx->tt_context->vdp2stencil[i] != NULL) memset(ctx->tt_context->vdp2stencil[i], 0, sizeof(struct StencilData) * 704 * 256);
      if (ctx->tt_context->vdp2priority[i] != NULL) memset(ctx->tt_context->vdp2priority[i], 0, sizeof(u8) * 704 * 256);
   }

   for(i = 1;i < 4;i++)
      memset(ctx->tt_context->linescreen[i], 0, sizeof(u32) * 512);

   return 0;
}

void TitanGLErase(struct TitanGLContext *tt_context)
{
   int i = 0;

   int height = tt_context->vdp2height;

   for (i = 0; i < 6; i++) {
      if (tt_context->vdp2framebuffer[i] != NULL) memset(tt_context->vdp2framebuffer[i], 0, sizeof(PixelData) * tt_context->vdp2width * height);
      if (tt_context->vdp2stencil[i] != NULL) memset(tt_context->vdp2stencil[i], 0, sizeof(struct StencilData) * tt_context->vdp2width * height);
      if (tt_context->vdp2priority[i] != NULL) memset(tt_context->vdp2priority[i], 0, sizeof(struct StencilData) * tt_context->vdp2width * height);
   }
}

int TitanGLDeInit(struct TitanGLContext *tt_context)
{
   int i;

   for(i = 0;i < 6;i++) {
      if (tt_context->vdp2framebuffer[i] != NULL) free(tt_context->vdp2framebuffer[i]);
      if (tt_context->vdp2stencil[i] != NULL) free(tt_context->vdp2stencil[i]);
      if (tt_context->vdp2priority[i] != NULL) free(tt_context->vdp2priority[i]);
      tt_context->vdp2framebuffer[i] = NULL;
      tt_context->vdp2stencil[i] = NULL;
      tt_context->vdp2priority[i] = NULL;
   }

   for(i = 1;i < 4;i++) {
      if (tt_context->linescreen[i] != NULL) free(tt_context->linescreen[i]);
      tt_context->linescreen[i] = NULL;
   }

   if (tt_context->backscreen != NULL) free(tt_context->backscreen);
   tt_context->backscreen = NULL;

   tt_context->inited = 0;

   return 0;
}

void TitanGLSetResolution(int width, int height, struct TitanGLContext *tt_context)
{
   tt_context->vdp2width = width;
   tt_context->vdp2height = height;
}

void TitanGLGetResolution(int * width, int * height, struct TitanGLContext *tt_context)
{
   *width = tt_context->vdp2width;
   *height = tt_context->vdp2height;
}

void TitanGLSetBlendingMode(int blend_mode, struct TitanGLContext *tt_context)
{
   if (blend_mode == TITAN_BLEND_BOTTOM)
   {
      tt_context->blend = TitanGLBlendPixelsBottom;
      tt_context->trans = TitanGLTransBit;
   }
   else if (blend_mode == TITAN_BLEND_ADD)
   {
      tt_context->blend = TitanGLBlendPixelsAdd;
      tt_context->trans = TitanGLTransBit;
   }
   else
   {
      tt_context->blend = TitanGLBlendPixelsTop;
      tt_context->trans = TitanGLTransAlpha;
   }
}

void TitanGLPutBackHLine(s32 y, u32 color, struct TitanGLContext *tt_context)
{
   PixelData* buffer = &tt_context->backscreen[(y)];
   int i;

   *buffer = color;
}

void TitanGLPutLineHLine(int linescreen, s32 y, u32 color, struct TitanGLContext *tt_context)
{
   if (linescreen == 0) return;

   {
      u32 * buffer = tt_context->linescreen[linescreen] + y;
      *buffer = color;
   }
}

void TitanGLPutPixel(int priority, s32 x, s32 y, u32 color, int linescreen, vdp2draw_struct* info, struct TitanGLContext *tt_context)
{
   if (priority == 0) return;

   {
      int pos = (y * tt_context->vdp2width) + x;
      tt_context->vdp2framebuffer[info->titan_which_layer][pos] = color;
      tt_context->vdp2priority[info->titan_which_layer][pos] = priority;
      tt_context->vdp2stencil[info->titan_which_layer][pos].linescreen = linescreen;
      tt_context->vdp2stencil[info->titan_which_layer][pos].shadow_enabled = info->titan_shadow_enabled;
      tt_context->vdp2stencil[info->titan_which_layer][pos].shadow_type = info->titan_shadow_type;
   }
}

void TitanGLPutHLine(int priority, s32 x, s32 y, s32 width, u32 color, struct TitanGLContext *tt_context)
{
   if (priority == 0) return;

   {
      PixelData * buffer = &tt_context->vdp2framebuffer[priority][ (y * tt_context->vdp2width) + x];
      int i;

      memset(buffer, color, width*sizeof(PixelData));
   }
}

void createGLPrograms(render_context *ctx) {

   GLbyte vShaderStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "attribute vec2 b_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "varying vec2 s_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "   s_texCoord = b_texCoord;  \n"
      "}                            \n";

   GLbyte fShaderStr[] =
      "varying vec2 v_texCoord;                            \n"
      "varying vec2 s_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "uniform sampler2D b_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  vec4 sprite = texture2D( s_texture, s_texCoord );\n"
      "  vec4 back = texture2D( b_texture, v_texCoord );\n"
      "  gl_FragColor = sprite.a*sprite + (1.0 - sprite.a)*back; \n"
      "}                                                   \n";

   GLbyte vShaderGPrioStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "attribute vec2 b_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "varying vec2 s_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "   s_texCoord = b_texCoord;  \n"
      "}                            \n";

   GLbyte fShaderGPrioStr[] =
      "varying vec2 v_texCoord;                            \n"
      "varying vec2 s_texCoord;                            \n"
      "uniform sampler2D sprite;                        \n"
      "uniform sampler2D layer;                        \n"
      "uniform sampler2D priority;                        \n"
      "uniform float layerpriority;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  vec4 prio = texture2D( priority, s_texCoord );\n"  
      "  vec4 spritepix = texture2D( sprite, s_texCoord );\n"
      "  vec4 layerpix = texture2D( layer, v_texCoord );\n" 
      "  if ((prio.a*255.0 + 0.5) >= layerpriority) {\n"
      "        if (spritepix.a >= (1.0/255.0))\n"
      "            gl_FragColor = spritepix.a*spritepix + (1.0 - spritepix.a)*layerpix; \n"
      "        else discard;\n"
      "  } else {;\n"
      "        if (layerpix.a >= (1.0/255.0))\n"
      "             gl_FragColor = layerpix.a*layerpix + (1.0 - layerpix.a)*spritepix;\n"
      "        else discard;\n"
      "  }\n"
      "}                                                   \n";


   while (sem_wait(&lockGL) != 0);
   SDL_GL_MakeCurrent(ctx->tt_context->glWindow, ctx->glContext);

   // Create the program object
   ctx->tt_context->titanBackProg = gles20_createProgram (vShaderStr, fShaderStr);

   if ( ctx->tt_context->titanBackProg == 0 ){
      fprintf (stderr,"Can not create a program\n");
      SDL_GL_MakeCurrent(ctx->tt_context->glWindow, NULL);
      while (sem_post(&lockGL) != 0);
      return 0;
   }

   // Get the attribute locations
   ctx->tt_context->positionLoc = glGetAttribLocation ( ctx->tt_context->titanBackProg, "a_position" );
   ctx->tt_context->texCoordLoc = glGetAttribLocation ( ctx->tt_context->titanBackProg, "a_texCoord" );
   ctx->tt_context->stexCoordLoc = glGetAttribLocation ( ctx->tt_context->titanBackProg, "b_texCoord" );

   // Get the sampler location
   ctx->tt_context->samplerLoc = glGetUniformLocation ( ctx->tt_context->titanBackProg, "s_texture" );
   // Get the sampler location
   ctx->tt_context->backLoc = glGetUniformLocation ( ctx->tt_context->titanBackProg, "b_texture" );

   ctx->tt_context->programGeneralPriority = gles20_createProgram (vShaderGPrioStr, fShaderGPrioStr);

   if (ctx->tt_context->programGeneralPriority == 0) {
      fprintf(stderr, "Can not create programGeneralPriority\n");
   }

   ctx->tt_context->posGPrioLoc = glGetAttribLocation( ctx->tt_context->programGeneralPriority, "a_position");
   ctx->tt_context->tCoordGPrioLoc = glGetAttribLocation( ctx->tt_context->programGeneralPriority, "a_texCoord");
   ctx->tt_context->sCoordGPrioLoc = glGetAttribLocation ( ctx->tt_context->programGeneralPriority, "b_texCoord" );
   ctx->tt_context->spriteLoc = glGetUniformLocation( ctx->tt_context->programGeneralPriority, "sprite");
   ctx->tt_context->layerLoc = glGetUniformLocation( ctx->tt_context->programGeneralPriority, "layer");
   ctx->tt_context->prioLoc = glGetUniformLocation( ctx->tt_context->programGeneralPriority, "priority");
   ctx->tt_context->refPrioLoc = glGetUniformLocation( ctx->tt_context->programGeneralPriority, "layerpriority");

   SDL_GL_MakeCurrent(ctx->tt_context->glWindow, NULL);
   while (sem_post(&lockGL) != 0);
}

static float swVertices [] = {
   -1.0f, -1.0f, 0, 0, 0, 0,
   1.0f, -1.0f, 1.0f, 0, 1.0f, 0,
   1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
   -1.0f,1.0f, 0, 1.0f, 0, 1.0f,
};

static int layer_tex = -1;
static int back_tex = -1;
static int sprite_tex = -1;
static int stencil_tex = -1;

void TitanGLRenderFBO(render_context *ctx) {
   struct TitanGLContext *tt_context = ctx->tt_context;
   int width = tt_context->vdp2width;
   int height = tt_context->vdp2height;
   gl_fbo *fbo = &tt_context->fbo;
   int error;

   int x, y, i, layer, j;
   int sorted_layers[8] = { 0 };
   int num_layers = 0;

   if ((width == 0) || (height == 0)) return;

   if (!tt_context->inited || (!tt_context->trans))
   {
      return;
   }

   while (sem_wait(&lockGL) != 0);
   SDL_GL_MakeCurrent(tt_context->glWindow, ctx->glContext);

   if ((tt_context->glwidth != tt_context->vdp2width) || (tt_context->glheight != tt_context->vdp2height)) {
       tt_context->glwidth = tt_context->vdp2width;
       tt_context->glheight = tt_context->vdp2height;
       if (back_tex == -1) glDeleteTextures(1,&back_tex);
       if (layer_tex == -1) glDeleteTextures(1,&layer_tex);
       if (sprite_tex == -1) glDeleteTextures(1,&sprite_tex);
       if (stencil_tex == -1) glDeleteTextures(1,&stencil_tex);
       back_tex = layer_tex = sprite_tex = stencil_tex = -1;
   }

   if (tt_context->vdp2fbo[TITAN_SPRITE] != -1) {
	swVertices[4] = (fbo->width - (float)width)/fbo->width;
	swVertices[5] = (fbo->height - (float)height)/fbo->height;
	swVertices[11] = (fbo->height - (float)height)/fbo->height;
	swVertices[22] = (fbo->width - (float)width)/fbo->width;
   }

   tt_context->layer_priority[TITAN_NBG0] = Vdp2Regs->PRINA & 0x7;
   tt_context->layer_priority[TITAN_NBG1] = ((Vdp2Regs->PRINA >> 8) & 0x7);
   tt_context->layer_priority[TITAN_NBG2] = (Vdp2Regs->PRINB & 0x7);
   tt_context->layer_priority[TITAN_NBG3] = ((Vdp2Regs->PRINB >> 8) & 0x7);
   tt_context->layer_priority[TITAN_RBG0] = (Vdp2Regs->PRIR & 0x7);

   glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
   glViewport(0,0,fbo->width, fbo->height);
   glClearColor(0.0, 0.0, 0.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);

   if (back_tex == -1) {
	glGenTextures(1, &back_tex);
	glBindTexture(GL_TEXTURE_2D, back_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//NULL means reserve texture memory, but texels are undefined
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   }

   if (sprite_tex == -1) {
	glGenTextures(1, &sprite_tex);
	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//NULL means reserve texture memory, but texels are undefined
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   }

   if (layer_tex == -1) {
	glGenTextures(1, &layer_tex);
	glBindTexture(GL_TEXTURE_2D, layer_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//NULL means reserve texture memory, but texels are undefined
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   }

   if (stencil_tex == -1) {
	glGenTextures(1, &stencil_tex);
        glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, stencil_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//NULL means reserve texture memory, but texels are undefined
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
   }

   sorted_layers[num_layers++] = TITAN_BACK;

   //pre-sort the layers so it doesn't have to be done per-pixel
   for (i = 0; i < 8; i++)
   {
      for (layer = 0; layer <= TITAN_RBG0; layer++)
      {
         if (tt_context->layer_priority[layer] > 0 && tt_context->layer_priority[layer] == i)
            sorted_layers[num_layers++] = layer;
      }
   }
   
   if( tt_context->g_VertexSWBuffer == 0 )
   {
	glGenBuffers(1, &tt_context->g_VertexSWBuffer);
   }

   for (j = 0; j < num_layers; j++)
   {
	int bg_layer = sorted_layers[j];
        if (bg_layer == TITAN_BACK) {
	    glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, back_tex);
	    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,1,height,GL_RGBA,GL_UNSIGNED_BYTE,tt_context->backscreen);
	    glActiveTexture(GL_TEXTURE1);
	    if (tt_context->vdp2fbo[TITAN_SPRITE] == -1) {
		    glBindTexture(GL_TEXTURE_2D, sprite_tex);
		    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,tt_context->vdp2framebuffer[TITAN_SPRITE]);
	    } else {
		   glBindTexture(GL_TEXTURE_2D, tt_context->vdp2fbo[TITAN_SPRITE]);
	    }
	    glUseProgram(tt_context->titanBackProg);
	    glUniform1i(tt_context->backLoc, 0);
	    glUniform1i(tt_context->samplerLoc, 1);
	    glBindBuffer(GL_ARRAY_BUFFER, tt_context->g_VertexSWBuffer);
	    glBufferData(GL_ARRAY_BUFFER, sizeof(swVertices),swVertices,GL_STATIC_DRAW);
	    glVertexAttribPointer ( tt_context->positionLoc, 2, GL_FLOAT,  GL_FALSE, 6 * sizeof(GLfloat), 0 );
	    glVertexAttribPointer ( tt_context->texCoordLoc, 2, GL_FLOAT,  GL_FALSE, 6 * sizeof(GLfloat), (void*)(sizeof(float)*2) );
	    glVertexAttribPointer ( tt_context->stexCoordLoc, 2, GL_FLOAT,  GL_FALSE, 6 * sizeof(GLfloat), (void*)(sizeof(float)*4) );
	    glEnableVertexAttribArray ( tt_context->positionLoc );
	    glEnableVertexAttribArray ( tt_context->texCoordLoc );
	    glEnableVertexAttribArray ( tt_context->stexCoordLoc );
	    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        } else {
	    glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, layer_tex);
	    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,tt_context->vdp2framebuffer[bg_layer]);
	    glActiveTexture(GL_TEXTURE1);
	    if (tt_context->vdp2fbo[TITAN_SPRITE] == -1) {
	    	glBindTexture(GL_TEXTURE_2D, sprite_tex);
	    	glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,tt_context->vdp2framebuffer[TITAN_SPRITE]);
	    } else {
		glBindTexture(GL_TEXTURE_2D, tt_context->vdp2fbo[TITAN_SPRITE]);
	    }
	    glActiveTexture(GL_TEXTURE2);
	    if (tt_context->vdp2prio[TITAN_SPRITE] == -1) {
	    	glBindTexture(GL_TEXTURE_2D, stencil_tex);
	    	glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,width,height,GL_ALPHA,GL_UNSIGNED_BYTE,tt_context->vdp2priority[TITAN_SPRITE]);
            } else {
		glBindTexture(GL_TEXTURE_2D, tt_context->vdp2prio[TITAN_SPRITE]);
	    }
	    glUseProgram(tt_context->programGeneralPriority);
	    glUniform1i(tt_context->layerLoc, 0);
	    glUniform1i(tt_context->spriteLoc, 1);
	    glUniform1i(tt_context->prioLoc, 2);
	    glUniform1f(tt_context->refPrioLoc, (float)(tt_context->layer_priority[bg_layer]));
	    glBindBuffer(GL_ARRAY_BUFFER, tt_context->g_VertexSWBuffer);
	    glBufferData(GL_ARRAY_BUFFER, sizeof(swVertices),swVertices,GL_STATIC_DRAW);
	    glVertexAttribPointer ( tt_context->posGPrioLoc, 2, GL_FLOAT,  GL_FALSE, 6 * sizeof(GLfloat), 0 );
	    glVertexAttribPointer ( tt_context->tCoordGPrioLoc, 2, GL_FLOAT,  GL_FALSE, 6 * sizeof(GLfloat), (void*)(sizeof(float)*2) );
	    glVertexAttribPointer ( tt_context->sCoordGPrioLoc, 2, GL_FLOAT,  GL_FALSE, 6 * sizeof(GLfloat), (void*)(sizeof(float)*4) );
	    glEnableVertexAttribArray ( tt_context->posGPrioLoc );
	    glEnableVertexAttribArray ( tt_context->tCoordGPrioLoc );
	    glEnableVertexAttribArray ( tt_context->sCoordGPrioLoc );
	    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
   }
   int err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("GL error 0x%x\n", err);
	}

   SDL_GL_MakeCurrent(ctx->tt_context->glWindow, NULL);
   while (sem_post(&lockGL) != 0);
}

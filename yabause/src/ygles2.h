/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau

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

#if defined(HAVE_LIBGLES)
#define GL_GLEXT_PROTOTYPES 1
        #define GLX_GLXEXT_PROTOTYPES 1
//#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#ifndef YGLES2_H
#define YGLES2_H

#include <stdarg.h>
#include <string.h>

#include "core.h"
#include "threads.h"
#include "vidshared.h"

typedef struct {
	float vertices[8];
	int w;
	int h;
	int flip;
	int priority;
	int dst;
    int uclipmode;
    int blendmode;
    s32 cor;
    s32 cog;
    s32 cob;
    int linescreen;

} YglEsSprite;

typedef struct {
	float x;
	float y;
} YglEsCache;

typedef struct {
	unsigned int * textdata;
	unsigned int w;
} YglEsTexture;


typedef struct {
	unsigned int currentX;
	unsigned int currentY;
	unsigned int yMax;
	unsigned int * texture;
	unsigned int width;
	unsigned int height;
} YglEsTextureManager;

extern YglEsTextureManager * YglEsTM;

void YglEsTMInit(unsigned int, unsigned int);
void YglEsTMDeInit(void);
void YglEsTMReset(void);
void YglEsTMAllocate(YglEsTexture *, unsigned int, unsigned int, unsigned int *, unsigned int *);

enum
{
   PG_NORMAL=1,
   PG_VDP1_NORMAL,
   PG_VFP1_GOURAUDSAHDING,
   PG_VFP1_STARTUSERCLIP,
   PG_VFP1_ENDUSERCLIP,
   PG_VFP1_HALFTRANS,    
   PG_VFP1_GOURAUDSAHDING_HALFTRANS, 
   PG_VDP2_ADDBLEND,
   PG_VDP2_DRAWFRAMEBUFF,    
   PG_VDP2_STARTWINDOW,
   PG_VDP2_ENDWINDOW,
   PG_WINDOW,
   PG_LINECOLOR_INSERT,
   PG_VDP2_DRAWFRAMEBUFF_LINECOLOR,
   PG_VDP2_DRAWFRAMEBUFF_ADDCOLOR,
   PG_MAX,
};

typedef struct {
   int prgid;
   GLuint prg;
   GLuint vertexBuffer;
   float * quads;
   float * textcoords;
   float * vertexAttribute;
   int currentQuad;
   int maxQuad;
   int vaid;
   char uClipMode;
   short ux1,uy1,ux2,uy2;
   int blendmode;
   int bwin0,logwin0,bwin1,logwin1,winmode;
   GLuint vertexp;
   GLuint texcoordp;
   GLuint mtxModelView;
   GLuint mtxTexture;
   GLuint color_offset;
   GLuint tex0;
   GLuint tex1;
   float color_offset_val[4];
   int (*setupUniform)(void *);
   int (*cleanupUniform)(void *);
} YglEsProgram;

typedef struct {
   int prgcount;
   int prgcurrent;
   int uclipcurrent;
   short ux1,uy1,ux2,uy2;
   int blendmode;
   YglEsProgram * prg;
} YglEsLevel;

typedef struct
{
    GLfloat   m[4][4];
} YglEsMatrix;

typedef struct {
   GLuint texture;
   GLuint pixelBufferID;
   int st;
   char message[512];
   int msglength;
   unsigned int width;
   unsigned int height;
   unsigned int depth;

   float clear_r;
   float clear_g;
   float clear_b;
   
   // VDP1 Info
   int vdp1_maxpri;
   int vdp1_minpri;
   
   // VDP1 Framebuffer
   int rwidth;
   int rheight;
   int drawframe;
   int readframe;
   GLuint rboid_depth;
   GLuint rboid_stencil;
   GLuint vdp1fbo;
   GLuint vdp1FrameBuff[2];
   GLuint smallfbo;
   GLuint smallfbotex;
   GLuint vdp1pixelBufferID;
   void * pFrameBuffer;

   // Message Layer
   int msgwidth;
   int msgheight;
   GLuint msgtexture;
   u32 * messagebuf;

   int bUpdateWindow;
   int win0v[512*4];
   int win0_vertexcnt;
   int win1v[512*4];
   int win1_vertexcnt;

   YglEsMatrix mtxModelView;
   YglEsMatrix mtxTexture;

   YglEsProgram windowpg;
   YglEsProgram renderfb;

   YglEsLevel * levels;
   
   u32 lincolor_tex;
   u32 linecolor_pbo;
   u32 * lincolor_buf;

}  YglEs;

extern YglEs * _YglEs;


int YglEsGLInit(int, int);
int YglEsInit(int, int, unsigned int);
void YglEsDeInit(void);
float * YglEsQuad(YglEsSprite *, YglEsTexture *,YglEsCache * c);
void YglEsQuadOffset(YglEsSprite * input, YglEsTexture * output, YglEsCache * c, int cx, int cy, float sx, float sy);
void YglEsCachedQuadOffset(YglEsSprite * input, YglEsCache * cache, int cx, int cy, float sx, float sy);
void YglEsCachedQuad(YglEsSprite *, YglEsCache *);
void YglEsRender(void);
void YglEsReset(void);
void YglEsShowTexture(void);
void YglEsChangeResolution(int, int);
void YglEsCacheQuadGrowShading(YglEsSprite * input, float * colors, YglEsCache * cache);
int YglEsQuadGrowShading(YglEsSprite * input, YglEsTexture * output, float * colors,YglEsCache * c);
void YglEsSetClearColor(float r, float g, float b);
void YglEsStartWindow( vdp2draw_struct * info, int win0, int logwin0, int win1, int logwin1, int mode );
void YglEsEndWindow( vdp2draw_struct * info );

void YglEsCacheInit(void);
void YglEsCacheDeInit(void);
int YglEsIsCached(u32,YglEsCache *);
void YglEsCacheAdd(u32,YglEsCache *);
void YglEsCacheReset(void);

// 0.. no belnd, 1.. Alpha, 2.. Add 
int YglEsSetLevelBlendmode( int pri, int mode );

void YglEs_uniformVDP2DrawFramebuffer_linecolor(void * p, float from, float to, float * offsetcol);
int YglEs_uniformVDP2DrawFramebuffer_addcolor(void * p, float from, float to, float * offsetcol);
void YglEs_uniformVDP2DrawFramebuffer( void * p,float from, float to , float * offsetcol );

void YglEsNeedToUpdateWindow();

void YglEsScalef(YglEsMatrix *result, GLfloat sx, GLfloat sy, GLfloat sz);
void YglEsTranslatef(YglEsMatrix *result, GLfloat tx, GLfloat ty, GLfloat tz);
void YglEsRotatef(YglEsMatrix *result, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void YglEsFrustum(YglEsMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);
void YglEsPerspective(YglEsMatrix *result, float fovy, float aspect, float nearZ, float farZ);
void YglEsOrtho(YglEsMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ);
void YglEsLoadIdentity(YglEsMatrix *result);
void YglEsMatrixMultiply(YglEsMatrix *result, YglEsMatrix *srcA, YglEsMatrix *srcB);

int YglEsInitVertexBuffer( int initsize );
void YglEsDeleteVertexBuffer();
int YglEsUnMapVertexBuffer();
int YglEsMapVertexBuffer();
int YglEsUserDirectVertexBuffer();
int YglEsUserVertexBuffer();
int YglEsGetVertexBuffer( int size, void ** vpos, void **tcpos, void **vapos );
int YglEsExpandVertexBuffer( int addsize, void ** vpos, void **tcpos, void **vapos );
intptr_t YglEsGetOffset( void* address );
int YglEsBlitFramebuffer(u32 srcTexture, u32 targetFbo, float w, float h);

void YglEsRenderVDP1(void);
u32 * YglEsGetLineColorPointer();
void YglEsSetLineColor(u32 * pbuf, int size);

int YglEs_uniformWindow(void * p );
int YglEsProgramInit();
int YglEsProgramChange( YglEsLevel * level, int prgid );

int YglEsEs20_uniformWindow(void * p );
int YglEsEs20ProgramInit();
int YglEsEs20ProgramChange( YglEsLevel * level, int prgid );
int YglEsEs20BlitFramebuffer(u32 srcTexture, u32 targetFbo, float w, float h);
void YglEsEs20_uniformVDP2DrawFramebuffer_linecolor(void * p, float from, float to, float * offsetcol);
int YglEsEs20_uniformVDP2DrawFramebuffer_addcolor(void * p, float from, float to, float * offsetcol);
void YglEsEs20_uniformVDP2DrawFramebuffer( void * p,float from, float to , float * offsetcol );

#if !defined(__APPLE__) && !defined(__ANDROID__) && !defined(_OGLES3_)

extern GLuint (STDCALL *glCreateProgram)(void);
extern GLuint (STDCALL *glCreateShader)(GLenum);
extern void (STDCALL *glShaderSource)(GLuint,GLsizei,const GLchar **,const GLint *);
extern void (STDCALL *glCompileShader)(GLuint);
extern void (STDCALL *glAttachShader)(GLuint,GLuint);
extern void (STDCALL *glLinkProgram)(GLuint);
extern void (STDCALL *glUseProgram)(GLuint);
extern GLint (STDCALL *glGetUniformLocation)(GLuint,const GLchar *);
extern void (STDCALL *glUniform1i)(GLint,GLint);
extern void (STDCALL *glGetShaderInfoLog)(GLuint,GLsizei,GLsizei *,GLchar *);
extern void (STDCALL *glVertexAttribPointer)(GLuint index,GLint size, GLenum type, GLboolean normalized, GLsizei stride,const void *pointer);
extern void (STDCALL *glBindAttribLocation)( GLuint program, GLuint index, const GLchar * name);
extern void (STDCALL *glGetProgramiv)( GLuint    program, GLenum pname, GLint * params);
extern void (STDCALL *glGetShaderiv)(GLuint shader,GLenum pname,GLint *    params);
extern GLint (STDCALL *glGetAttribLocation)(GLuint program,const GLchar *    name);

extern void (STDCALL *glEnableVertexAttribArray)(GLuint index);
extern void (STDCALL *glDisableVertexAttribArray)(GLuint index);


//GL_ARB_framebuffer_object
extern PFNGLISRENDERBUFFERPROC glIsRenderbuffer;
extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv;
extern PFNGLISFRAMEBUFFERPROC glIsFramebuffer;
extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
extern PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
extern PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;
extern PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
extern PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample;
extern PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer;

extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;

#endif // !defined(__APPLE__) && !defined(__ANDROID__)

#endif // YGL_H

#endif // defined(HAVE_LIBGL) || defined(__ANDROID__)

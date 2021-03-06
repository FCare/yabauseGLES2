/*  Copyright 2006 Guillaume Duhamel
    Copyright 2006 Fabien Coulon
    Copyright 2005 Joost Peters

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

#ifdef HAVE_LIBGL
//SDL Port does not support OGL3 or OGLES3 so we need to fore SOFT CORE in that case
#define FORCE_CORE_SOFT
#endif

#include <SDL2/SDL.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "../glutils/gles20utils.h"

#include "../yabause.h"
#include "../gameinfo.h"
#include "../yui.h"
#include "../peripheral.h"
#include "../sh2core.h"
#include "../sh2int.h"
#ifdef HAVE_LIBGLES
#include "../vidsoftgles.h"
#endif
#ifdef HAVE_LIBGL
#include "../vidogl.h"
#endif
#include "../vidsoft.h"
#include "../cs0.h"
#include "../cs2.h"
#include "../cdbase.h"
#include "../scsp.h"
#include "../sndsdl.h"
#include "../sndal.h"
#include "../persdljoy.h"
#ifdef ARCH_IS_LINUX
#include "../perlinuxjoy.h"
#endif
#include "../debug.h"
#include "../m68kcore.h"
#include "../m68kc68k.h"
#include "../vdp1.h"
#include "../vdp2.h"
#include "../cdbase.h"
#include "../peripheral.h"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600


M68K_struct * M68KCoreList[] = {
&M68KDummy,
#ifdef HAVE_MUSASHI
&M68KMusashi,
#endif
#ifdef HAVE_C68K
&M68KC68K,
#endif
#ifdef HAVE_Q68
&M68KQ68,
#endif
NULL
};

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
#ifdef TEST_PSP_SH2
&SH2PSP,
#endif
#ifdef SH2_DYNAREC
&SH2Dynarec,
#endif
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
#ifdef HAVE_LIBSDL
&PERSDLJoy,
#endif
#ifdef ARCH_IS_LINUX
&PERLinuxJoy,
#endif
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
#ifndef UNKNOWN_ARCH
&ArchCD,
#endif
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
#ifdef HAVE_LIBSDL
&SNDSDL,
#endif
#ifdef HAVE_LIBAL
&SNDAL,
#endif
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
#ifdef HAVE_LIBGLES
&VIDSoftGLES,
#endif
#ifdef HAVE_LIBGL
&VIDOGL,
#endif
&VIDSoft,
NULL
};

static GLint g_FrameBuffer = 0;
static GLint g_SWFrameBuffer = 0;
static GLint g_VertexBuffer = 0;
static GLint g_VertexDevBuffer = 0;
static GLint g_VertexSWBuffer = 0;
static GLint programObject  = 0;
static GLint positionLoc    = 0;
static GLint texCoordLoc    = 0;
static GLint samplerLoc     = 0;

int g_buf_width = -1;
int g_buf_height = -1;

int fbo_buf_width = -1;
int fbo_buf_height = -1;

static int resizeFilter = GL_NEAREST;
static int fullscreen = 0;

static char biospath[256] = "\0";
static char cdpath[256] = "\0";

SDL_Window *window;
SDL_Texture* fb;
SDL_Renderer* rdr;

yabauseinit_struct yinit;

static int error;

static float vertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};

static float swVertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};

static const float squareVertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};


static float devVertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};

void YuiErrorMsg(const char * string) {
    fprintf(stderr, "%s\n\r", string);
}

void DrawSWFBO() {
    int error,i;
    int buf_width, buf_height;

    glUseProgram(programObject);
    if( g_SWFrameBuffer == 0 )
    {
       glGenTextures(1,&g_SWFrameBuffer);
       glActiveTexture ( GL_TEXTURE0 );
       glBindTexture(GL_TEXTURE_2D, g_SWFrameBuffer);
       glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, NULL);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
       glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, resizeFilter );
       glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, resizeFilter );
    } else {
       glActiveTexture ( GL_TEXTURE0 );
       glBindTexture(GL_TEXTURE_2D, g_SWFrameBuffer);
    }
    

    VIDCore->GetGlSize(&buf_width, &buf_height);
    if ((buf_width == 0) || (buf_height == 0)) return;
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,512,256,GL_RGBA,GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT,VIDCore->getSWFbo());


      if( buf_width != fbo_buf_width ||  buf_height != fbo_buf_height )
      {
	  memcpy(swVertices, squareVertices, sizeof(swVertices));
          swVertices[6]=swVertices[10]=(float)buf_width/1024.f;
          swVertices[11]=swVertices[15]=(float)buf_height/1024.f;
          fbo_buf_width  = buf_width;
          fbo_buf_height = buf_height;
          for (i=0; i<4; i++) {
              swVertices[0+i*4] = swVertices[0+i*4]/3.0f + 2.0f/3.0f;
              swVertices[1+i*4] = swVertices[1+i*4]/3.0f + 2.0f/3.0f;
          }
       }
   if( g_VertexSWBuffer == 0 )
   {
      glGenBuffers(1, &g_VertexSWBuffer);
   }
   glBindBuffer(GL_ARRAY_BUFFER, g_VertexSWBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),swVertices,GL_STATIC_DRAW);
   glVertexAttribPointer ( positionLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), 0 );
   glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), (void*)(sizeof(float)*2) );
   glEnableVertexAttribArray ( positionLoc );
   glEnableVertexAttribArray ( texCoordLoc );
   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void DrawDevFBO() {
    int error,i;
    int buf_width, buf_height;
    int tex = VIDCore->getDevFbo();

    glUseProgram(programObject);

    glActiveTexture ( GL_TEXTURE0 );
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, resizeFilter );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, resizeFilter );
   if( g_VertexDevBuffer == 0 )
   {
      glGenBuffers(1, &g_VertexDevBuffer);
   }

   VIDCore->GetGlSize(&buf_width, &buf_height);
   if ((buf_width == 0) || (buf_height == 0)) return;

   if( buf_width != fbo_buf_width ||  buf_height != fbo_buf_height )
   {
       fbo_buf_width = buf_width;
       fbo_buf_height = buf_height;
    }

      glBindBuffer(GL_ARRAY_BUFFER, g_VertexDevBuffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(devVertices),devVertices,GL_STATIC_DRAW);
      glVertexAttribPointer ( positionLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), 0 );
      glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), (void*)(sizeof(float)*2) );
      glEnableVertexAttribArray ( positionLoc );
      glEnableVertexAttribArray ( texCoordLoc );
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void YuiDrawSoftwareBuffer() {

    int buf_width, buf_height;
    int error;

    VIDCore->GetGlSize(&buf_width, &buf_height);

#ifdef _OGLES_    
    glUseProgram(programObject);

    if( g_FrameBuffer == 0 )
    {
       glGenTextures(1,&g_FrameBuffer);
       glActiveTexture ( GL_TEXTURE0 );
       glBindTexture(GL_TEXTURE_2D, g_FrameBuffer);
       glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
       glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, resizeFilter );
       glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, resizeFilter );
    }
    glActiveTexture ( GL_TEXTURE0 );
    glBindTexture(GL_TEXTURE_2D, g_FrameBuffer);

    if ((buf_width == 0) || (buf_height == 0)) return;
    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,buf_width,buf_height,GL_RGBA,GL_UNSIGNED_BYTE,VIDCore->getFramebuffer());

    if( g_VertexBuffer == 0 )
    {
       glGenBuffers(1, &g_VertexBuffer);
    }

   if( buf_width != g_buf_width ||  buf_height != g_buf_height )
   {
      vertices[6]=vertices[10]=(float)buf_width/1024.f;
      vertices[11]=vertices[15]=(float)buf_height/1024.f;
      g_buf_width  = buf_width;
      g_buf_height = buf_height;
   }
   glBindBuffer(GL_ARRAY_BUFFER, g_VertexBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),vertices,GL_STATIC_DRAW);
   glVertexAttribPointer ( positionLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), 0 );
   glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT,  GL_FALSE, 4 * sizeof(GLfloat), (void*)(sizeof(float)*2) );
   glEnableVertexAttribArray ( positionLoc );
   glEnableVertexAttribArray ( texCoordLoc );

   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
   SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = buf_width;
  rect.h = buf_height;
  SDL_Rect dest;
  dest.x = 0;
  dest.y = 0;
  dest.w = WINDOW_WIDTH;
  dest.h = WINDOW_HEIGHT;
   SDL_UpdateTexture(fb,&rect,VIDCore->getFramebuffer(),buf_width*sizeof(pixel_t));
   SDL_RenderCopy(rdr,fb,&rect,&dest);
   SDL_RenderPresent(rdr);
#endif
}

static unsigned long nextFrameTime = 0;
static unsigned long delayUs = 1000000/60;

static int frameskip = 1;

static unsigned long getCurrentTimeUs(unsigned long offset) {
    struct timeval s;

    gettimeofday(&s, NULL);

    return (s.tv_sec * 1000000 + s.tv_usec) - offset;
}

static unsigned long time_left(void)
{
    unsigned long now;

    now = getCurrentTimeUs(0);
    if(nextFrameTime <= now)
        return 0;
    else
        return nextFrameTime - now;
}

void YuiSwapBuffers(void) {
  int SDLWidth, SDLHeight;

   if( window == NULL ){
      return;
   }
   int buf_width, buf_height;
   int glWidth, glHeight;
   SDL_GetWindowSize(window, &SDLWidth, &SDLHeight);
   VIDCore->GetGlSize(&buf_width, &buf_height);

   float ar = (float)buf_width/(float)buf_height;

   float dar = (float)SDLWidth/(float)SDLHeight;

   if (ar <= dar) {
     glHeight = SDLHeight;
     glWidth = ar * glHeight;
   } else {
     glWidth = SDLWidth;
     glHeight = glWidth/ar;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glViewport((SDLWidth-glWidth)/2,(SDLHeight-glHeight)/2,glWidth, glHeight);

   if(VIDCore->getFramebuffer() != NULL){
       YuiDrawSoftwareBuffer();
   }
#ifdef _OGLES_ 
   else

   if ((VIDCore->getSWFbo != NULL) && (VIDCore->getSWFbo() != NULL)) {
       DrawSWFBO();
   } else

   if (( VIDCore->getDevFbo!= NULL) && (VIDCore->getDevFbo() != -1)) {
       DrawDevFBO();
   }  
#endif
   SDL_GL_SwapWindow(window);
  if (frameskip == 1)
    usleep(time_left());

  nextFrameTime += delayUs;

   //lastFrameTime = getCurrentTimeUs(0);
}

void YuiInit() {
	yinit.m68kcoretype = M68KCORE_MUSASHI;
	yinit.percoretype = PERCORE_SDLJOY;
	yinit.sh2coretype = SH2CORE_DEFAULT;
#ifdef FORCE_CORE_SOFT
        yinit.vidcoretype = VIDCORE_SOFT;
#else
	yinit.vidcoretype = VIDCORE_OGLES; //VIDCORE_SOFT  
#endif
	yinit.sndcoretype = SNDCORE_SDL;
	yinit.cdcoretype = CDCORE_DEFAULT;
	yinit.carttype = CART_NONE;
	yinit.regionid = REGION_EUROPE;
	yinit.biospath = NULL;
	yinit.cdpath = NULL;
	yinit.buppath = NULL;
	yinit.mpegpath = NULL;
	yinit.cartpath = NULL;
        yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
	yinit.osdcoretype = OSDCORE_DEFAULT;
	yinit.skip_load = 0;

	yinit.sh1coretype = SH2CORE_DEFAULT;
	yinit.use_cd_block_lle = 0;
	yinit.usethreads = 1;
	yinit.numthreads = 4;
}

void SDLInit(void) {
	SDL_GLContext context;
	Uint32 flags = (fullscreen == 1)?SDL_WINDOW_FULLSCREEN|SDL_WINDOW_OPENGL:SDL_WINDOW_OPENGL;

	SDL_InitSubSystem(SDL_INIT_VIDEO);

#ifdef _OGLES_
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	SDL_GL_SetSwapInterval(1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

	
	window = SDL_CreateWindow("OpenGL Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, flags);
	if (!window) {
    		fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
    		return;
	}

	context = SDL_GL_CreateContext(window);
	if (!context) {
    		fprintf(stderr, "Couldn't create context: %s\n", SDL_GetError());
    		return;
	}

	rdr = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

#ifdef _OGLES_
        glClearColor( 0.0f,0.0f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
#else
        SDL_RenderClear(rdr);
#ifdef USE_16BPP
        fb = SDL_CreateTexture(rdr,SDL_PIXELFORMAT_ABGR1555,SDL_TEXTUREACCESS_STREAMING, 704, 512);
#else
        fb = SDL_CreateTexture(rdr,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STREAMING, 704, 512);
#endif
#endif

  nextFrameTime = getCurrentTimeUs(0) + delayUs;
}

///
// Create a shader object, load the shader source, and
// compile the shader.
//

int YuiInitProgramForSoftwareRendering()
{
#ifdef _OGLES_
   GLbyte vShaderStr[] =
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

   GLbyte fShaderStr[] =
      "varying vec2 v_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  vec4 color = texture2D( s_texture, v_texCoord );\n"    
      "  gl_FragColor = color;\n"    
      "}                                                   \n";

   // Create the program object
   programObject = gles20_createProgram (vShaderStr, fShaderStr);

   if ( programObject == 0 ){
      fprintf (stderr,"Can not create a program\n");
      return 0;
   }

   // Get the attribute locations
   positionLoc = glGetAttribLocation ( programObject, "a_position" );
   texCoordLoc = glGetAttribLocation ( programObject, "a_texCoord" );

   // Get the sampler location
   samplerLoc = glGetUniformLocation ( programObject, "s_texture" );

   return GL_TRUE;
#else
   return 0;
#endif
}


void displayGameInfo(char *filename) {
    GameInfo info;
    if (! GameInfoFromPath(filename, &info))
    {
       return;
    }

    printf("Game Info:\n\tSystem: %s\n\tCompany: %s\n\tItemNum:%s\n\tVersion:%s\n\tDate:%s\n\tCDInfo:%s\n\tRegion:%s\n\tPeripheral:%s\n\tGamename:%s\n", info.system, info.company, info.itemnum, info.version, info.date, info.cdinfo, info.region, info.peripheral, info.gamename);
}

int main(int argc, char *argv[]) {
	int i;
	SDL_Event event;
	int quit = SDL_FALSE;
	SDL_GameController* ctrl;

	LogStart();
	LogChangeOutput( DEBUG_STDERR, NULL );

	YuiInit();

//handle command line arguments
   for (i = 1; i < argc; ++i) {
      if (argv[i]) {
         //show usage
         if (0 == strcmp(argv[i], "-h") || 0 == strcmp(argv[i], "-?") || 0 == strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
         }
			
         //set bios
         if (0 == strcmp(argv[i], "-b") && argv[i + 1]) {
            strncpy(biospath, argv[i + 1], 256);
            yinit.biospath = biospath;
	 } else if (strstr(argv[i], "--bios=")) {
            strncpy(biospath, argv[i] + strlen("--bios="), 256);
            yinit.biospath = biospath;
	 }
         //set iso
         else if (0 == strcmp(argv[i], "-i") && argv[i + 1]) {
            strncpy(cdpath, argv[i + 1], 256);
	    yinit.cdcoretype = 1;
	    yinit.cdpath = cdpath;
	    displayGameInfo(cdpath);
	 } else if (strstr(argv[i], "--iso=")) {
            strncpy(cdpath, argv[i] + strlen("--iso="), 256);
	    yinit.cdcoretype = 1;
	    yinit.cdpath = cdpath;
	 }
         //set cdrom
	 else if (0 == strcmp(argv[i], "-c") && argv[i + 1]) {
            strncpy(cdpath, argv[i + 1], 256);
	    yinit.cdcoretype = 2;
	    yinit.cdpath = cdpath;
	 } else if (strstr(argv[i], "--cdrom=")) {
            strncpy(cdpath, argv[i] + strlen("--cdrom="), 256);
	    yinit.cdcoretype = 2;
	    yinit.cdpath = cdpath;
	 }
         // Set sound
         else if (strcmp(argv[i], "-ns") == 0 || strcmp(argv[i], "--nosound") == 0) {
	    yinit.sndcoretype = 0;
	 }
         // Set sound
         else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fullscreen") == 0) {
	    fullscreen = 1;
	 }
         // Set sound
         else if (strcmp(argv[i], "-rb") == 0 || strcmp(argv[i], "--resizebilinear") == 0) {
	    resizeFilter = GL_LINEAR;
	 }
	 else if (strcmp(argv[i], "-sc") == 0 || strcmp(argv[i], "--resizebilinear") == 0) {
	    yinit.vidcoretype = VIDCORE_SOFT;
	 }
         // Auto frame skip
         else if (strstr(argv[i], "--vsyncoff")) {
              frameskip = 0;
         }
	 // Binary
	 else if (strstr(argv[i], "--binary=")) {
	    char binname[1024];
	    unsigned int binaddress;
	    int bincount;

	    bincount = sscanf(argv[i] + strlen("--binary="), "%[^:]:%x", binname, &binaddress);
	    if (bincount > 0) {
	       if (bincount < 2) binaddress = 0x06004000;

               //yui_window_run(YUI_WINDOW(yui));
	       MappedMemoryLoadExec(binname, binaddress);
	    }
	 }
      }
   }
        SDLInit();
#ifdef HAVE_LIBGLES
        if(( yinit.vidcoretype == VIDCORE_SOFT ) || ( yinit.vidcoretype == VIDCORE_OGLES )){
            if( YuiInitProgramForSoftwareRendering() != GL_TRUE ){
                fprintf(stderr, "Fail to YuiInitProgramForSoftwareRendering\n");
                return -1;
            }
        }
#endif

	YabauseDeInit();

        if (YabauseInit(&yinit) != 0) printf("YabauseInit error \n\r");


	while (quit == SDL_FALSE)
	{
		if (SDL_PollEvent(&event))
		{
   			switch(event.type)
    			{
     				case SDL_QUIT:
      					quit = SDL_TRUE;
      				break;
				case SDL_KEYDOWN:
					printf("Key down!");
				break;
     			}
  		}

	        PERCore->HandleEvents();
	}

	YabauseDeInit();
	LogStop();
	SDL_Quit();

	return 0;
}

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

#include "../asyncRenderer.h"

typedef struct s_fboElement fboElement;

struct s_fboElement{
  gl_fbo* current;
  fboElement* next;
};

fboElement *mFBOFree;
fboElement *mFBOToBeDrawn;

void initFBOFree(fboElement *elem, int nbElement, int w, int h) {
   	if (nbElement == 0) return;
	elem = malloc(sizeof(fboElement));
     	elem->current = malloc(sizeof(gl_fbo));
        gles20_createFBO(elem->current, w, h, 0);
 	initFBOFree(elem->next, nbElement-1, w, h);
}


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

static int resizeFilter = GL_NEAREST;

static char biospath[256] = "\0";
static char cdpath[256] = "\0";

yabauseinit_struct yinit;

void YuiErrorMsg(const char * string) {
    fprintf(stderr, "%s\n\r", string);
}


void YuiSwapBuffers() {

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
         else if (strcmp(argv[i], "-rb") == 0 || strcmp(argv[i], "--resizebilinear") == 0) {
	    resizeFilter = GL_LINEAR;
	 }
         // Auto frame skip
         else if (strstr(argv[i], "--autoframeskip=")) {
            int fscount;
            int fsenable;
            fscount = sscanf(argv[i] + strlen("--autoframeskip="), "%d", &fsenable);
            //if (fscount > 0)
              // yui_window_set_frameskip(YUI_WINDOW(yui), fsenable);
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

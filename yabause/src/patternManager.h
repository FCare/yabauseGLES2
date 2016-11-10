#ifndef __PATTERN_MANAGER_H__
#define __PATTERN_MANAGER_H__

#include "core.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

typedef struct sPattern {
	unsigned int tex;
	int width;
	int height;
	u32 param[3];
	int frameout;
        u32 mesh;
} Pattern;

Pattern* getCachePattern(int param0, int param1, int param2, int w, int h);
void addCachePattern(Pattern* pat);
Pattern* createCachePattern(int param0, int param1, int param2, int w, int h, int mesh);
void recycleCache();

#endif

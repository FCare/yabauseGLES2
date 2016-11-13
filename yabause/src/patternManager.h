#ifndef __PATTERN_MANAGER_H__
#define __PATTERN_MANAGER_H__

#include "core.h"

typedef struct sPattern {
	unsigned int tex;
	int width;
	int height;
	float tw;
	float th;
	u32 param[3];
	int frameout;
        u32 mesh;
} Pattern;

Pattern* getCachePattern(int param0, int param1, int param2, int w, int h);
void addCachePattern(Pattern* pat);
Pattern* createCachePattern(int param0, int param1, int param2, int w, int h, float tw, float th, int mesh);
void recycleCache();

#endif

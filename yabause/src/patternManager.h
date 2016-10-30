#ifndef __PATTERN_MANAGER_H__
#define __PATTERN_MANAGER_H__

#include "core.h"

typedef struct sPattern {
	unsigned int tex;
	int width;
	int height;
	u32 param[2];
	int frameout;
} Pattern;

Pattern* getCachePattern(int param1, int param2);
void addCachePattern(Pattern* pat);
Pattern* createCachePattern(int param0, int param1, int w, int h);
void recycleCache();

#endif

// C implementation of Heap Sort
#include <stdio.h>
#include <stdlib.h>

#include "patternManager.h"

Pattern* patternCache[0xFFFF];

#define CACHE_LIFETIME 60;

INLINE void deleteCachePattern(Pattern* pat) {
	glDeleteTextures(1, &pat->tex);
	free(pat);
}

INLINE u16 getHash(int param0, int param1) {
	u8 c[8];
	int i;
	u16 hash = 0xAAAA;
	c[0] = (param0 >> 0) & 0xFF;
	c[1] = (param0 >> 8) & 0xFF;
	c[2] = (param0 >> 16) & 0xFF;
	c[3] = (param0 >> 24) & 0xFF;
	c[4] = (param1 >> 0) & 0xFF;
	c[5] = (param1 >> 8) & 0xFF;
	c[6] = (param1 >> 16) & 0xFF;
	c[7] = (param1 >> 24) & 0xFF;
	for (i = 0; i<7; i++) {
		hash = hash ^ c[i];
	}
	for (i = 1; i<8; i++) {
		hash = hash ^ (c[i] << 8);
	}
	return hash;
}

INLINE void recycleCache() {
	int i;
	for (i = 0; i < 0xFFFF; i++) {
		Pattern* tmp = patternCache[i];
		if (tmp == NULL) continue;
		tmp->frameout--;
		if (tmp->frameout == 0) {
			deleteCachePattern(tmp);
			patternCache[i] = NULL;
		}
	}
}

INLINE Pattern* getCachePattern(int param0, int param1) {
  Pattern *pat = patternCache[getHash(param0, param1)];
  if ((pat!= NULL) && (pat->param[0]==param0) && (pat->param[1]==param1)) {
        pat->frameout = CACHE_LIFETIME;
  	return pat;
  } else
	return NULL;
}
int size = 0;

INLINE void addCachePattern(Pattern* pat) {
	Pattern *collider = patternCache[getHash(pat->param[0], pat->param[1])];
	if (collider != NULL) deleteCachePattern(collider);

	patternCache[getHash(pat->param[0], pat->param[1])] = pat;
}

INLINE Pattern* createCachePattern(int param0, int param1, int w, int h) {
	Pattern* new = malloc(sizeof(Pattern));
	new->param[0] = param0;
	new->param[1] = param1;
        new->width = w;
	new->height = h;
	new->frameout = CACHE_LIFETIME;
	return new;
}

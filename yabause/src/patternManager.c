// C implementation of Heap Sort
#include <stdio.h>
#include <stdlib.h>

#include "patternManager.h"

// A heap has current size and array of elements
typedef struct MaxHeap_s
{
    int size;
    Pattern** array;
} MaxHeap;

MaxHeap* createAndBuildHeap(int size);

MaxHeap *cache = NULL;

Pattern* patternCache[0xFFFF];

#define CACHE_LIFETIME 60;

#if 0
Pattern* getCachePattern(int param0, int param1) {
//Let's do a linear search for now. Might be dichotomia...
printf("getCachePattern\n");
	int i = 0; 
	if (cache == NULL) return NULL;
	for (i = 0; i < cache->size; i++) {
		if ((param0 == cache->array[i]->param[0]) && (param1 == cache->array[i]->param[1]))
			return cache->array[i];
	}
	return NULL;
}
void addCachePattern(Pattern* pat) {
	if (cache == NULL) {
		cache = createAndBuildHeap(1);
		cache->array[0] = pat;
        } else {
		Pattern** newCacheArray = (Pattern**)malloc((cache->size + 1) * sizeof(Pattern*));
		memcpy (newCacheArray, cache->array, cache->size);
		newCacheArray[cache->size] = pat;
		cache->size++;
		free(cache->array);
		cache->array = newCacheArray;
		//heapSort(cache);
	}
}

#else

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
			free(tmp);
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
	if (collider != NULL) free(collider);

	patternCache[getHash(pat->param[0], pat->param[1])] = pat;
}
#endif


INLINE Pattern* createCachePattern(int param0, int param1, int w, int h) {
	Pattern* new = malloc(sizeof(Pattern));
	new->param[0] = param0;
	new->param[1] = param1;
        new->width = w;
	new->height = h;
	new->frameout = CACHE_LIFETIME;
	return new;
}

// A utility function to swap to integers
void swap(Pattern** a, Pattern** b) { Pattern* t = *a; *a = *b;  *b = t; }

// The main function to heapify a Max Heap. The function
// assumes that everything under given root (element at
// index idx) is already heapified
void maxHeapify(MaxHeap* maxHeap, int idx)
{
    int largest = idx;  // Initialize largest as root
    int left = (idx << 1) + 1;  // left = 2*idx + 1
    int right = (idx + 1) << 1; // right = 2*idx + 2

    // See if left child of root exists and is greater than
    // root
printf("%d %d %d %d\n", left, right, largest, maxHeap->size);
    if (left < maxHeap->size)
        if ((maxHeap->array[left]->param[0] > maxHeap->array[largest]->param[0]) || (maxHeap->array[left]->param[1] > maxHeap->array[largest]->param[1]))
        largest = left;
printf("ok0 %d\n", maxHeap->size);
    // See if right child of root exists and is greater than
    // the largest so far
    if (right < maxHeap->size)
        if ((maxHeap->array[right]->param[0] > maxHeap->array[largest]->param[0]) || (maxHeap->array[right]->param[1] > maxHeap->array[largest]->param[1]))
        largest = right;

printf("ok1 %d\n", maxHeap->size);
    // Change root, if needed
    if (largest != idx)
    {
printf("ok2\n");
        swap(&maxHeap->array[largest], &maxHeap->array[idx]);
printf("ok3\n");
        maxHeapify(maxHeap, largest);
printf("ok4\n");
    }
}

// A utility function to create a max heap of given capacity
MaxHeap* createAndBuildHeap(int size)
{
    MaxHeap* maxHeap =
              (MaxHeap*) malloc(sizeof(MaxHeap));
    maxHeap->size = size;   // initialize size of heap
    maxHeap->array = (Pattern**)malloc(sizeof(Pattern*)*size); // Assign address of first element of array
    return maxHeap;
}

// The main function to sort an array of given size
void heapSort(MaxHeap* maxHeap)
{
    int i;
    // Start from bottommost and rightmost internal mode and heapify all
    // internal modes in bottom up way
    if (maxHeap->size <= 1) return;
printf("heapSort1\n");
    for (i = (maxHeap->size - 2) / 2; i >= 0; --i)
        maxHeapify(maxHeap, i);
printf("heapSort2\n");
    // Repeat following steps while heap size is greater than 1.
    // The last element in max heap will be the minimum element
    while (maxHeap->size > 1)
    {
	printf("heapSort3\n");
        // The largest item in Heap is stored at the root. Replace
        // it with the last item of the heap followed by reducing the
        // size of heap by 1.
        swap(&maxHeap->array[0], &maxHeap->array[maxHeap->size - 1]);
        --maxHeap->size;  // Reduce heap size
	printf("heapSort4\n");
        // Finally, heapify the root of tree.
        maxHeapify(maxHeap, 0);
	printf("heapSort5\n");
    }
}


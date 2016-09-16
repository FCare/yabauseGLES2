
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>


typedef struct profiler_s {
    int started;
    unsigned long duration;
    unsigned long nb_frames;
    unsigned long entry[8];
} profiler_t;

static profiler_t profiler;
static unsigned long start[8];
static unsigned long glob_start;
static unsigned long duration;

static unsigned long getCurrentTimeMs(unsigned long offset) {
    struct timeval s;

    gettimeofday(&s, NULL);

    return (s.tv_sec * 1000 + s.tv_usec/1000) - offset;
}

void resetProfiler(unsigned long d) {
    int i;
    duration = d;
    profiler.nb_frames = 0;
    for (i = 0; i<8; i++) profiler.entry[i] = 0;
    glob_start = getCurrentTimeMs(0);
    profiler.started = 1;
}

static void stopProfiler() {
    profiler.duration = getCurrentTimeMs(glob_start);
    printf("FPS = %f\n", profiler.nb_frames * 1000.0 / profiler.duration);
    printf("\t Profile %d : %f %\n", 0, 100.0 * (float) profiler.entry[0]/(float) profiler.duration);
    profiler.started = 0;
}

int updateProfiler() {
    if (profiler.started == 0) return 1;
    profiler.nb_frames++;
    if (getCurrentTimeMs(glob_start) >= duration) {
        stopProfiler();
        return 1;
    }
    return 0;
}

void startLocalProfile(int id) {
    start[id] = getCurrentTimeMs(0);
}

void stopLocalProfile(int id) {
    profiler.entry[id] += getCurrentTimeMs(start[id]);
}



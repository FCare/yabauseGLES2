#ifndef __PROFILER_H__
#define __PROFILER_H__


extern void resetProfiler(unsigned long d);
extern int updateProfiler();
extern void startLocalProfile(int id);
extern void stopLocalProfile(int id);

#endif //__PROFILER_H__

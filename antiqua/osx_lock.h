#ifndef _OSX_LOCK_H_
#define _OSX_LOCK_H_

#include "types.h"
#include <pthread.h>

// audio thread
extern int runThreadAudio;
extern pthread_mutex_t runMutexAudio;
extern pthread_cond_t runConditionAudio;

// input thread
extern int runThreadInput;
extern pthread_mutex_t runMutexInput;
extern pthread_cond_t runConditionInput;

MONExternC void lockThread(int conditionVariable, pthread_mutex_t mutex, pthread_cond_t condition);
MONExternC void unlockThread(int conditionVariable, pthread_mutex_t mutex, pthread_cond_t condition);
MONExternC void waitIfBlocked(int conditionVariable, pthread_mutex_t mutex, pthread_cond_t condition);

#endif

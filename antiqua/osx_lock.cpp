#include "osx_lock.h"

int runThreadAudio = 1;
pthread_mutex_t runMutexAudio = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t runConditionAudio = PTHREAD_COND_INITIALIZER;

int runThreadInput = 1;
pthread_mutex_t runMutexInput = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t runConditionInput = PTHREAD_COND_INITIALIZER;

void lockThread(int conditionVariable, pthread_mutex_t mutex, pthread_cond_t condition)
{
  pthread_mutex_lock(&mutex);
  conditionVariable = 0;
  pthread_mutex_unlock(&mutex);
}

void unlockThread(int conditionVariable, pthread_mutex_t mutex, pthread_cond_t condition)
{
  pthread_mutex_lock(&mutex);
  conditionVariable = 1;
  pthread_cond_signal(&condition);
  pthread_mutex_unlock(&mutex);
}

void waitIfBlocked(int conditionVariable, pthread_mutex_t mutex, pthread_cond_t condition)
{
  pthread_mutex_lock(&mutex);
  while (!conditionVariable)
  {
    pthread_cond_wait(&condition, &mutex);
  }
  pthread_mutex_unlock(&mutex);
}

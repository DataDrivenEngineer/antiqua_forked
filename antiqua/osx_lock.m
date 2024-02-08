#include <pthread.h>

static int runThreadAudio = 1;
static pthread_mutex_t runMutexAudio = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t runConditionAudio = PTHREAD_COND_INITIALIZER;

LOCK_AUDIO_THREAD(lockAudioThread)
{
  pthread_mutex_lock(&runMutexAudio);
  runThreadAudio = 0;
  pthread_mutex_unlock(&runMutexAudio);
}

MONExternC UNLOCK_AUDIO_THREAD(unlockAudioThread)
{
  pthread_mutex_lock(&runMutexAudio);
  runThreadAudio = 1;
  pthread_cond_signal(&runConditionAudio);
  pthread_mutex_unlock(&runMutexAudio);
}

MONExternC WAIT_IF_AUDIO_BLOCKED(waitIfAudioBlocked)
{
  pthread_mutex_lock(&runMutexAudio);
  while (!runThreadAudio)
  {
    pthread_cond_wait(&runConditionAudio, &runMutexAudio);
  }
  pthread_mutex_unlock(&runMutexAudio);
}

static int runThreadInput = 1;
static pthread_mutex_t runMutexInput = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t runConditionInput = PTHREAD_COND_INITIALIZER;

MONExternC LOCK_INPUT_THREAD(lockInputThread)
{
  pthread_mutex_lock(&runMutexInput);
  runThreadInput = 0;
  pthread_mutex_unlock(&runMutexInput);
}

MONExternC UNLOCK_INPUT_THREAD(unlockInputThread)
{
  pthread_mutex_lock(&runMutexInput);
  runThreadInput = 1;
  pthread_cond_signal(&runConditionInput);
  pthread_mutex_unlock(&runMutexInput);
}

MONExternC WAIT_IF_INPUT_BLOCKED(waitIfInputBlocked)
{
  pthread_mutex_lock(&runMutexInput);
  while (!runThreadInput)
  {
    pthread_cond_wait(&runConditionInput, &runMutexInput);
  }
  pthread_mutex_unlock(&runMutexInput);
}

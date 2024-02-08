#ifndef _OSX_LOCK_H_
#define _OSX_LOCK_H_

LOCK_AUDIO_THREAD(lockAudioThread);
UNLOCK_AUDIO_THREAD(unlockAudioThread);
WAIT_IF_AUDIO_BLOCKED(waitIfAudioBlocked);

LOCK_INPUT_THREAD(lockInputThread);
UNLOCK_INPUT_THREAD(unlockInputThread);
WAIT_IF_INPUT_BLOCKED(waitIfInputBlocked);

#endif

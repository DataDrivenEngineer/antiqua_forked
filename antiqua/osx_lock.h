#ifndef _OSX_LOCK_H_
#define _OSX_LOCK_H_

MONExternC LOCK_AUDIO_THREAD(lockAudioThread);
MONExternC UNLOCK_AUDIO_THREAD(unlockAudioThread);
MONExternC WAIT_IF_AUDIO_BLOCKED(waitIfAudioBlocked);

MONExternC LOCK_INPUT_THREAD(lockInputThread);
MONExternC UNLOCK_INPUT_THREAD(unlockInputThread);
MONExternC WAIT_IF_INPUT_BLOCKED(waitIfInputBlocked);

#endif

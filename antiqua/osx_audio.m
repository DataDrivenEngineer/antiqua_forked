#include "types.h"
#include "osx_audio.h"
#include "osx_time.h"
#include "osx_dynamic_loader.h"
#include "CustomNSView.h"

struct SoundState soundState = {0};
static AudioObjectID device = kAudioObjectUnknown;
static _Nullable AudioDeviceIOProcID procID;

static OSStatus appIOProc(AudioObjectID inDevice,
                        const AudioTimeStamp* _Nonnull   inNow,
                        const AudioBufferList* _Nonnull inInputData,
                        const AudioTimeStamp*   _Nonnull inInputTime,
                        AudioBufferList*        _Nonnull outOutputData,
                        const AudioTimeStamp*   _Nonnull inOutputTime,
                        void* __nullable        inClientData)
{
  // TODO(dima): tighten up audio sync when we start mixing sounds
  struct SoundState *soundState = (struct SoundState *) inClientData;
  // TODO(dima): do we need to account for cases when there is >1 buffer?
  // # of frames needed = total # of bytes / num of channels in frame (2) / size of sample (4 = float)
  waitIfAudioBlocked(&thread);
  soundState->needFrames = outOutputData->mBuffers->mDataByteSize / 2 / 4;
  soundState->frames = (r32 *) outOutputData->mBuffers[0].mData;
#if !XCODE_BUILD
  if (gameCode.fillSoundBuffer)
  {
    gameCode.fillSoundBuffer(&thread, soundState);
  }
#else
  fillSoundBuffer(&thread, soundState);
#endif

  return kAudioHardwareNoError;     
}

b32 playAudio(void)
{
  OSStatus err = kAudioHardwareNoError;

  if (soundState.soundPlaying) return 0;
  
  err = AudioDeviceCreateIOProcID(device, appIOProc, (void *) &soundState, &procID);
  if (err != kAudioHardwareNoError) return 0;

  err = AudioDeviceStart(device, procID);				// start playing sound through the device
  if (err != kAudioHardwareNoError) return 0;

  soundState.soundPlaying = 1; // set the playing status global to true
  return 1;
}

b32 stopAudio(void)
{
    OSStatus err = kAudioHardwareNoError;
    
    if (!soundState.soundPlaying) return 0;
    
    err = AudioDeviceStop(device, procID);				// stop playing sound through the device
    if (err != kAudioHardwareNoError) 
    {
      if (err == kAudioHardwareBadDeviceError)
      {
	// Device not found, could be because e.g. headphones got unplugged - still must set sound to not playing
	soundState.soundPlaying = 0;
      }
      return 0;
    }

    err = AudioDeviceDestroyIOProcID(device, procID);
    if (err != kAudioHardwareNoError) return 0;

    soundState.soundPlaying = 0;						// set the playing status global to false
    return 1;
}

OSStatus appObjPropertyListenerProc(AudioObjectID nObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress* __nullable inAddresses, void* __nullable inClientData)
{
  for (s32 i = 0; i < inNumberAddresses; i++)
  {
    if (inAddresses[i].mSelector == kAudioHardwarePropertyDefaultOutputDevice)
    {
      fprintf(stderr, "Default audio device changed!\n");
      stopAudio();
    }
  }
  return kAudioHardwareNoError;     
}

void initAudio(void)
{
  OSStatus err = kAudioHardwareNoError;
  u32 tempSize;
  AudioObjectPropertyAddress tempPropertyAddress = {0};
  AudioObjectPropertyAddress devicePropertyAddress = {0};

  soundState.sampleRate = 48000;
  soundState.toneHz = 256;
  soundState.tSine = 0;
 
  // get the default output device for the HAL
  devicePropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  devicePropertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
  devicePropertyAddress.mElement = kAudioObjectPropertyElementMain;
  tempSize = sizeof(device);		// it is required to pass the size of the data to be returned
  err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &devicePropertyAddress, 0, 0, &tempSize, (void *) &device);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "get kAudioHardwarePropertyDefaultOutputDevice error %d\n", err);
      return;
  }

  // tell the HAL to manage its own thread for notifications. Need this so that the listener for audio default device change is called
  AudioObjectPropertyAddress loopPropertyAddress = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
  if(!AudioObjectHasProperty(kAudioObjectSystemObject, &loopPropertyAddress))
  {
    CFRunLoopRef theRunLoop = 0;
    err = AudioObjectSetPropertyData(kAudioObjectSystemObject, &loopPropertyAddress, 0, 0, sizeof(CFRunLoopRef), &theRunLoop);
    if (err != kAudioHardwareNoError) {
	fprintf(stderr, "failed to register run loop, error: %d\n", err);
	return;
    }
  }

  err = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &devicePropertyAddress, appObjPropertyListenerProc, 0);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "failed to add listener for kAudioHardwarePropertyDefaultOutputDevice property, error: %d\n", err);
  }

  AudioStreamBasicDescription audioDataFormat = {0};
  tempSize = sizeof(audioDataFormat);				// it is required to pass the size of the data to be returned
  tempPropertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
  tempPropertyAddress.mScope = kAudioObjectPropertyScopeOutput;
  tempPropertyAddress.mElement = kAudioObjectPropertyElementMain;
  // Values below are for stereo 
  audioDataFormat.mSampleRate = 48000.f;
  audioDataFormat.mFormatID = kAudioFormatLinearPCM;
  audioDataFormat.mFormatFlags = kAudioFormatFlagIsFloat | kLinearPCMFormatFlagIsPacked;
  audioDataFormat.mBitsPerChannel = 32;
  audioDataFormat.mBytesPerFrame = 8;
  audioDataFormat.mChannelsPerFrame = 2;
  audioDataFormat.mBytesPerPacket = 8;
  audioDataFormat.mFramesPerPacket = 1;

  err = AudioObjectSetPropertyData(device, &tempPropertyAddress, 0, 0, tempSize, &audioDataFormat);
  if (err != kAudioHardwareNoError)
  {
      fprintf(stderr, "set kAudioDevicePropertyStreamFormat error %d\n", err);
  }

#if ANTIQUA_INTERNAL
  // latency test
  tempPropertyAddress.mSelector = kAudioStreamPropertyLatency;
  tempPropertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
  tempPropertyAddress.mElement = kAudioObjectPropertyElementMain;
  if (AudioObjectHasProperty(device, &tempPropertyAddress))
  {
    u32 streamLatency;
    tempSize = sizeof(streamLatency);
    err = AudioObjectGetPropertyData(device, &tempPropertyAddress, 0, 0, &tempSize, (void *)&streamLatency);
    if (err != kAudioHardwareNoError)
    {
      fprintf(stderr, "Failed to get kAudioStreamPropertyLatency, error: %d\n", err);
    }
    else
    {
      fprintf(stderr, "kAudioStreamPropertyLatency: %d\n", streamLatency);
    }
  }

  tempPropertyAddress.mSelector = kAudioDevicePropertyLatency;
  tempPropertyAddress.mScope = kAudioObjectPropertyScopeOutput;
  tempPropertyAddress.mElement = kAudioObjectPropertyElementMain;
  if (AudioObjectHasProperty(device, &tempPropertyAddress))
  {
    u32 deviceLatency;
    tempSize = sizeof(deviceLatency);
    err = AudioObjectGetPropertyData(device, &tempPropertyAddress, 0, 0, &tempSize, (void *) &deviceLatency);
    if (err != kAudioHardwareNoError)
    {
      fprintf(stderr, "Failed to get kAudioDevicePropertyLatency, error: %d\n", err);
    }
    else
    {
      fprintf(stderr, "kAudioDevicePropertyLatency: %d\n", deviceLatency);
    }
  }
#endif
}

OSStatus resetAudio(void)
{
  OSStatus err = kAudioHardwareNoError;
  err = AudioHardwareUnload();
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "failed to unload hardware, error: %d\n", err);
  }
  return err;
}

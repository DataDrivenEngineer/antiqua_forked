#ifndef osx_audio_h
#define osx_audio_h

#include <CoreAudio/AudioHardware.h>

#include "osx_audio.h"
#include "antiqua.h"

u8 soundPlaying = 0;
static struct SoundState soundState = {0};
static AudioObjectID device = kAudioObjectUnknown;
static AudioDeviceIOProcID procID;

static OSStatus appIOProc(AudioObjectID inDevice,
                        const AudioTimeStamp*   inNow,
                        const AudioBufferList*  inInputData,
                        const AudioTimeStamp*   inInputTime,
                        AudioBufferList*        outOutputData,
                        const AudioTimeStamp*   inOutputTime,
                        void* __nullable        inClientData)
{
  struct SoundState *soundState = (struct SoundState *) inClientData;
  soundState->frames = (r32 *) outOutputData->mBuffers[0].mData;
  fillSoundBuffer(soundState);

  return kAudioHardwareNoError;     
}

u8 playAudio()
{
  OSStatus err = kAudioHardwareNoError;

  if (soundPlaying) return 0;
  
  err = AudioDeviceCreateIOProcID(device, appIOProc, (void *) &soundState, &procID);
  if (err != kAudioHardwareNoError) return 0;

  err = AudioDeviceStart(device, procID);				// start playing sound through the device
  if (err != kAudioHardwareNoError) return 0;

  soundPlaying = 1; // set the playing status global to true
  return 1;
}

static u8 stopAudio()
{
    OSStatus err = kAudioHardwareNoError;
    
    if (!soundPlaying) return 0;
    
    err = AudioDeviceStop(device, procID);				// stop playing sound through the device
    if (err != kAudioHardwareNoError) 
    {
      if (err == kAudioHardwareBadDeviceError)
      {
	// Device not found, could be because e.g. headphones got unplugged - still must set sound to not playing
	soundPlaying = 0;
      }
      return 0;
    }

    err = AudioDeviceDestroyIOProcID(device, procID);
    if (err != kAudioHardwareNoError) return 0;

    soundPlaying = 0;						// set the playing status global to false
    return 1;
}

OSStatus appObjPropertyListenerProc(AudioObjectID nObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress* inAddresses, void* __nullable inClientData)
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
  u32 newBufferSize;

  soundState.sampleRate = 48000;
  soundState.toneHz = 256;
  // Num of frames enough for playing audio for 1/30th of a second
  soundState.needFrames = soundState.sampleRate / 30;
  // Each frame has 2 samples, each sample is a float (4 bytes)
  newBufferSize = soundState.needFrames * 2 * 4;
 
  // get the default output device for the HAL
  devicePropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  devicePropertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
  devicePropertyAddress.mElement = kAudioObjectPropertyElementMain;
  tempSize = sizeof(device);		// it is required to pass the size of the data to be returned
  err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &devicePropertyAddress, 0, 0, &tempSize, (void *) &device);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "get kAudioHardwarePropertyDefaultOutputDevice error %ld\n", err);
      return;
  }

  // tell the HAL to manage its own thread for notifications. Need this so that the listener for audio default device change is called
  AudioObjectPropertyAddress loopPropertyAddress = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
  if(!AudioObjectHasProperty(kAudioObjectSystemObject, &loopPropertyAddress))
  {
    CFRunLoopRef theRunLoop = 0;
    err = AudioObjectSetPropertyData(kAudioObjectSystemObject, &loopPropertyAddress, 0, 0, sizeof(CFRunLoopRef), &theRunLoop);
    if (err != kAudioHardwareNoError) {
	fprintf(stderr, "failed to register run loop, error: %ld\n", err);
	return;
    }
  }

  err = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &devicePropertyAddress, appObjPropertyListenerProc, 0);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "failed to add listener for kAudioHardwarePropertyDefaultOutputDevice property, error: %ld\n", err);
  }

  tempPropertyAddress.mSelector = kAudioDevicePropertyBufferSize;
  tempPropertyAddress.mScope = kAudioObjectPropertyScopeOutput;
  tempPropertyAddress.mElement = kAudioObjectPropertyElementMain;
  err = AudioObjectSetPropertyData(device, &tempPropertyAddress, 0, 0, sizeof(newBufferSize), &newBufferSize);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "failed to update buffer size, error: %ld\n", err);
      return;
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
      fprintf(stderr, "set kAudioDevicePropertyStreamFormat error %ld\n", err);
  }
}

void resetAudio()
{
  OSStatus err = kAudioHardwareNoError;
  err = AudioHardwareUnload();
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "failed to unload hardware, error: %ld\n", err);
  }
}

#endif

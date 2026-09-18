#import "AudioUnitDriver.h"

static OSStatus MetronomeRenderCallback(
    void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData) {
    
    AudioUnitDriver *selfInstance = (__bridge AudioUnitDriver *)inRefCon;
    if (!selfInstance.isRunning || selfInstance.engineHandle == nullptr) {
        // Sessiz buffer doldur
        for (UInt32 i = 0; i < ioData->mNumberBuffers; ++i) {
            memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
        }
        return noErr;
    }

    // Interleaved Stereo Float32 Buffer kontrolü
    if (ioData->mNumberBuffers > 0 && ioData->mBuffers[0].mData != nullptr) {
        float *stereoBuffer = (float *)ioData->mBuffers[0].mData;
        // C++20 Core render çağrısı (Real-Time Safe, Zero Memory Allocation)
        polyrhythm_render(selfInstance.engineHandle, stereoBuffer, inNumberFrames);
    }

    return noErr;
}

@interface AudioUnitDriver () {
    AudioUnit _audioUnit;
    double _sampleRate;
}
@end

@implementation AudioUnitDriver

- (instancetype)initWithSampleRate:(double)sampleRate {
    self = [super init];
    if (self) {
        _sampleRate = (sampleRate > 0) ? sampleRate : 48000.0;
        _engineHandle = polyrhythm_create((uint32_t)_sampleRate);
        _isRunning = NO;
        [self setupAudioUnit];
    }
    return self;
}

- (void)dealloc {
    [self stopAudioUnit];
    if (_audioUnit) {
        AudioComponentInstanceDispose(_audioUnit);
        _audioUnit = NULL;
    }
    if (_engineHandle) {
        polyrhythm_destroy(_engineHandle);
        _engineHandle = NULL;
    }
}

- (void)setupAudioUnit {
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent comp = AudioComponentFindNext(NULL, &desc);
    if (!comp) return;

    OSStatus status = AudioComponentInstanceNew(comp, &_audioUnit);
    if (status != noErr) return;

    UInt32 enableOutput = 1;
    AudioUnitSetProperty(_audioUnit,
                         kAudioOutputUnitProperty_EnableIO,
                         kAudioUnitScope_Output,
                         0,
                         &enableOutput,
                         sizeof(enableOutput));

    // Float32 Interleaved Stereo Format
    AudioStreamBasicDescription audioFormat;
    audioFormat.mSampleRate = _sampleRate;
    audioFormat.mFormatID = kAudioFormatLinearPCM;
    audioFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    audioFormat.mFramesPerPacket = 1;
    audioFormat.mChannelsPerFrame = 2;
    audioFormat.mBitsPerChannel = 32;
    audioFormat.mBytesPerPacket = sizeof(float) * 2;
    audioFormat.mBytesPerFrame = sizeof(float) * 2;

    AudioUnitSetProperty(_audioUnit,
                         kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Input,
                         0,
                         &audioFormat,
                         sizeof(audioFormat));

    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = MetronomeRenderCallback;
    callbackStruct.inputProcRefCon = (__bridge void *)self;

    AudioUnitSetProperty(_audioUnit,
                         kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Global,
                         0,
                         &callbackStruct,
                         sizeof(callbackStruct));

    AudioUnitInitialize(_audioUnit);
}

- (BOOL)startAudioUnit:(NSError **)error {
    if (_isRunning) return YES;
    
    if (_engineHandle) {
        polyrhythm_start(_engineHandle);
    }
    
    OSStatus status = AudioOutputUnitStart(_audioUnit);
    if (status == noErr) {
        _isRunning = YES;
        return YES;
    }
    
    if (error) {
        *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:status userInfo:nil];
    }
    return NO;
}

- (void)stopAudioUnit {
    if (!_isRunning) return;
    
    if (_audioUnit) {
        AudioOutputUnitStop(_audioUnit);
    }
    if (_engineHandle) {
        polyrhythm_stop(_engineHandle);
    }
    _isRunning = NO;
}

- (void)reset {
    if (_engineHandle) {
        polyrhythm_reset(_engineHandle);
    }
}

@end

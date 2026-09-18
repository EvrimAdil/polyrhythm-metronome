#pragma once

#import <Foundation/Foundation.h>
#import <AudioUnit/AudioUnit.h>
#import <AVFoundation/AVFoundation.h>
#include "c_bridge.h"

NS_ASSUME_NONNULL_BEGIN

@interface AudioUnitDriver : NSObject

@property (nonatomic, readonly) BOOL isRunning;
@property (nonatomic, readonly) PolyrhythmEngineHandle engineHandle;

- (instancetype)initWithSampleRate:(double)sampleRate;
- (BOOL)startAudioUnit:(NSError **)error;
- (void)stopAudioUnit;
- (void)reset;

@end

NS_ASSUME_NONNULL_END

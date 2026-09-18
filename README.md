# Polyrhythm Metronome - Zero-Drift Low-Latency Mobile Audio Engine

Çok kanallı ($N$-layer), poliritmik, polimetrik ve Euclidean algoritmalarına sahip, sıfır gecikmeli (zero-drift) profesyonel mobil metronom projesi.

## Mimari Genel Bakış
- **Shared C++20 Core (`shared_core`)**:
  * `AudioScheduler`: 64-bit tam sayı ve rasyonel numune sayısı sayacı (`uint64_t total_samples_rendered_`). 10 Milyon numunede kayma (drift) $< 0.025$ sample (tamamen sıfır-kaymalı / zero-drift).
  * `PolyrhythmEngine`: Gerçek zamanlı ses iş parçacığında dinamik bellek ayırmayan (no allocation in RT thread), kilit içermeyen (`LockFreeRingBuffer` SPSC) mimari.
  * `Bjorklund`: $E(k, n)$ Euclidean ritim üreteci.
  * `SoundGenerator`: Dinamik üssel sönümlü transient sinüs klikleri + 16/24/32-bit PCM WAV örnek havuzu, constant-power stereo pan, pitch-shift ve soft-clipping limiter.
  * `TrainerEngine`: Tempo Trainer (ivmelenme) & Mute Trainer (içsel zamanlama testi).
  * `c_bridge.h`: iOS Swift ve Android JNI için saf C ABI köprüsü.
- **Android Projesi (`android`)**:
  * Google Oboe / AAudio düşük gecikmeli ses motoru sürücüsü (`OboeAudioDriver.cpp`).
  * JNI katmanı (`native-lib.cpp`) ve Kotlin köprüsü (`MetronomeNativeBridge.kt`).
  * `MetronomeForegroundService`: `WAKE_LOCK`, `USAGE_MEDIA`, Android 14+ `mediaPlayback` servisi.
  * Jetpack Compose UI: `PolygonPhaseCanvas` geometrik N-gen faz dönüş animasyonu, `JogWheel` rotary dial tempo kontrolü, Room veritabanı ön ayarları.
- **iOS Projesi (`ios`)**:
  * CoreAudio `kAudioUnitSubType_RemoteIO` render callback (`AudioUnitDriver.mm`).
  * `AudioSessionManager.swift`: `AVAudioSessionCategoryPlayback`, `MPNowPlayingInfoCenter`, `MPRemoteCommandCenter`.
  * SwiftUI: `PolygonPhaseView` (`TimelineView` + `Canvas`), `JogWheelView` dairesel rotary dial, `PresetManagerView`.

---

## C++20 Çekirdek Testlerini Çalıştırma
```bash
cd shared_core/tests
./run_tests.sh
```

Test çıktıları:
1. `test_scheduler_drift`: 10,000,000 numunede drift testi (PASSED - Max drift: 0.02484 samples).
2. `test_bjorklund`: Euclidean $E(3,8), E(5,8), E(4,12)$ dağılım doğrulaması (PASSED).
3. `test_polyrhythm`: Çok kanallı 3:4 poliritmik mikser, faz çakışması ve C-Bridge entegrasyonu (PASSED).

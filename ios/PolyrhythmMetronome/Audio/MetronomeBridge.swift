import Foundation
import Combine

struct SwiftBeatEvent: Identifiable {
    let id = UUID()
    let timestamp: UInt64
    let layerIndex: Int
    let stepIndex: Int
    let totalSteps: Int
    let accent: Int // 0: Mute, 1: Normal, 2: Downbeat
    let currentPhase: Float
    let bpm: Int
}

final class MetronomeBridge: ObservableObject {
    @Published var isPlaying: Bool = false
    @Published var bpm: Double = 120.0
    @Published var layerCount: Int = 0

    let beatSubject = PassthroughSubject<SwiftBeatEvent, Never>()

    private var driver: AudioUnitDriver?
    private var pollingTimer: Timer?

    init() {
        AudioSessionManager.shared.configurePlaybackSession()
        driver = AudioUnitDriver(sampleRate: 48000.0)

        AudioSessionManager.shared.setupRemoteCommandCenter(
            onPlay: { [weak self] in self?.start() },
            onPause: { [weak self] in self?.stop() },
            onToggle: { [weak self] in self?.toggle() }
        )
    }

    func start() {
        guard let driver = driver, !isPlaying else { return }
        var error: NSError?
        if driver.startAudioUnit(&error) {
            isPlaying = true
            startPolling()
            AudioSessionManager.shared.updateNowPlaying(bpm: bpm, isPlaying: true, activeLayers: layerCount)
        } else {
            print("[MetronomeBridge] Start failed: \(String(describing: error))")
        }
    }

    func stop() {
        guard let driver = driver, isPlaying else { return }
        driver.stopAudioUnit()
        isPlaying = false
        stopPolling()
        AudioSessionManager.shared.updateNowPlaying(bpm: bpm, isPlaying: false, activeLayers: layerCount)
    }

    func toggle() {
        if isPlaying {
            stop()
        } else {
            start()
        }
    }

    func setBpm(_ newBpm: Double) {
        self.bpm = max(20.0, min(400.0, newBpm))
        if let handle = driver?.engineHandle {
            polyrhythm_set_bpm(handle, self.bpm)
        }
        AudioSessionManager.shared.updateNowPlaying(bpm: self.bpm, isPlaying: isPlaying, activeLayers: layerCount)
    }

    func addLayer(mode: Int, pulses: Int, totalSteps: Int, subdiv: Int = 1, volume: Float = 0.8, pan: Float = 0.0, pitch: Float = 1.0, synthFreq: Float = 1000.0) {
        guard let handle = driver?.engineHandle else { return }
        var cfg = CLayerConfig()
        cfg.mode = UInt8(mode)
        cfg.ratio_pulses = UInt16(pulses)
        cfg.total_steps = UInt16(totalSteps)
        cfg.subdivision = UInt16(subdiv)
        cfg.volume = volume
        cfg.pan = pan
        cfg.pitch_shift = pitch
        cfg.synth_frequency = synthFreq
        cfg.accent_count = UInt16(totalSteps)
        cfg.accents.0 = 2 // Downbeat
        // Swift C struct array element access
        _ = polyrhythm_add_layer(handle, &cfg)
        layerCount = Int(polyrhythm_get_layer_count(handle))
    }

    func removeLayer(at index: Int) {
        guard let handle = driver?.engineHandle else { return }
        polyrhythm_remove_layer(handle, index)
        layerCount = Int(polyrhythm_get_layer_count(handle))
    }

    func getLayerPhase(_ index: Int) -> Float {
        guard let handle = driver?.engineHandle else { return 0.0 }
        return polyrhythm_get_layer_phase(handle, index)
    }

    func getBarPhase() -> Float {
        guard let handle = driver?.engineHandle else { return 0.0 }
        return polyrhythm_get_bar_phase(handle)
    }

    private func startPolling() {
        pollingTimer?.invalidate()
        // 120 FPS akıcı görsel senkronizasyon için ~8ms timer
        pollingTimer = Timer.scheduledTimer(withTimeInterval: 0.008, repeats: true) { [weak self] _ in
            guard let self = self, let handle = self.driver?.engineHandle else { return }
            var event = CBeatEvent()
            while polyrhythm_pop_beat_event(handle, &event) {
                let swiftEvt = SwiftBeatEvent(
                    timestamp: event.sample_timestamp,
                    layerIndex: Int(event.layer_index),
                    stepIndex: Int(event.step_index),
                    totalSteps: Int(event.total_steps),
                    accent: Int(event.accent),
                    currentPhase: event.current_phase,
                    bpm: Int(event.bpm)
                )
                self.beatSubject.send(swiftEvt)
            }
        }
        RunLoop.main.add(pollingTimer!, forMode: .common)
    }

    private func stopPolling() {
        pollingTimer?.invalidate()
        pollingTimer = nil
    }
}

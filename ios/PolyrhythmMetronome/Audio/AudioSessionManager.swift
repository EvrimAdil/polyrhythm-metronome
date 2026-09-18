import Foundation
import AVFoundation
import MediaPlayer

final class AudioSessionManager {
    static let shared = AudioSessionManager()

    private init() {}

    func configurePlaybackSession() {
        let session = AVAudioSession.sharedInstance()
        do {
            try session.setCategory(.playback, mode: .default, options: [])
            try session.setPreferredSampleRate(48000.0)
            try session.setPreferredIOBufferDuration(0.005) // ~5ms low latency buffer
            try session.setActive(true)
        } catch {
            print("[AudioSessionManager] Failed to configure audio session: \(error)")
        }
    }

    func setupRemoteCommandCenter(
        onPlay: @escaping () -> Void,
        onPause: @escaping () -> Void,
        onToggle: @escaping () -> Void
    ) {
        let commandCenter = MPRemoteCommandCenter.shared()

        commandCenter.playCommand.isEnabled = true
        commandCenter.playCommand.addTarget { _ in
            onPlay()
            return .success
        }

        commandCenter.pauseCommand.isEnabled = true
        commandCenter.pauseCommand.addTarget { _ in
            onPause()
            return .success
        }

        commandCenter.togglePlayPauseCommand.isEnabled = true
        commandCenter.togglePlayPauseCommand.addTarget { _ in
            onToggle()
            return .success
        }
    }

    func updateNowPlaying(bpm: Double, isPlaying: BooleanLiteralType, activeLayers: Int) {
        var info = [String: Any]()
        info[MPMediaItemPropertyTitle] = "Polyrhythm Metronome"
        info[MPMediaItemPropertyArtist] = "\(Int(bpm)) BPM | \(activeLayers) Katman"
        info[MPNowPlayingInfoPropertyPlaybackRate] = isPlaying ? 1.0 : 0.0

        MPNowPlayingInfoCenter.default().nowPlayingInfo = info
    }
}

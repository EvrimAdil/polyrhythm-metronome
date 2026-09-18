import SwiftUI

struct ContentView: View {
    @StateObject private var bridge = MetronomeBridge()
    @State private var showingPresets = false

    // Görsel poligon katmanları
    @State private var visualLayers: [PolygonLayerItem] = [
        PolygonLayerItem(id: 0, sides: 3, color: .cyan, phase: 0.0),
        PolygonLayerItem(id: 1, sides: 4, color: .orange, phase: 0.0)
    ]

    var body: some View {
        ZStack {
            Color(red: 0.04, green: 0.05, blue: 0.07)
                .ignoresSafeArea()

            VStack(spacing: 16) {
                // Üst Bar
                HStack {
                    VStack(alignment: .leading, spacing: 2) {
                        Text("POLYRHYTHM")
                            .font(.system(size: 20, weight: .black, design: .monospaced))
                            .foregroundColor(.white)
                        Text("ZERO-DRIFT AUDIO CORE")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(.cyan)
                    }

                    Spacer()

                    Button(action: { showingPresets = true }) {
                        Image(systemName: "slider.horizontal.3")
                            .font(.title2)
                            .foregroundColor(.white)
                            .padding(8)
                            .background(Color(white: 0.15))
                            .clipShape(Circle())
                    }
                }
                .padding(.horizontal, 20)

                // 1. Poligon Tabanlı Faz Çakışma Görselleştirmesi
                PolygonPhaseView(layers: visualLayers, isPlaying: bridge.isPlaying)
                    .padding(.horizontal, 16)

                // 2. Rotary Jog Wheel Tempo Kontrolü
                JogWheelView(bpm: $bridge.bpm, onTapTempo: handleTapTempo)

                Spacer()

                // 3. Başlat / Durdur Ana Butonu
                Button(action: { bridge.toggle() }) {
                    HStack(spacing: 12) {
                        Image(systemName: bridge.isPlaying ? "pause.fill" : "play.fill")
                            .font(.title3)
                        Text(bridge.isPlaying ? "DURDUR" : "BAŞLAT")
                            .font(.system(size: 18, weight: .bold))
                    }
                    .foregroundColor(bridge.isPlaying ? .black : .white)
                    .frame(maxWidth: .infinity, minHeight: 56)
                    .background(bridge.isPlaying ? Color.cyan : Color(white: 0.2))
                    .cornerRadius(14)
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 12)
            }
        }
        .sheet(isPresented: $showingPresets) {
            PresetManagerView { preset in
                bridge.setBpm(preset.bpm)
                showingPresets = false
            }
        }
        .onAppear {
            setupInitialLayers()
        }
        .onReceive(bridge.beatSubject) { event in
            // Katman fazlarını güncelle
            if event.layerIndex < visualLayers.count {
                visualLayers[event.layerIndex].phase = Double(bridge.getLayerPhase(event.layerIndex))
            }
        }
    }

    private func setupInitialLayers() {
        // 3:4 Poliritmik başlangıç katmanları
        bridge.addLayer(mode: 0, pulses: 3, totalSteps: 3, volume: 0.85, pan: -0.4, synthFreq: 900.0)
        bridge.addLayer(mode: 0, pulses: 4, totalSteps: 4, volume: 0.85, pan: 0.4, synthFreq: 1300.0)
    }

    @State private var tapTimestamps: [Date] = []
    private func handleTapTempo() {
        let now = Date()
        tapTimestamps.append(now)
        if tapTimestamps.count > 4 {
            tapTimestamps.removeFirst()
        }
        if tapTimestamps.count >= 2 {
            var totalInterval: Double = 0
            for i in 1..<tapTimestamps.count {
                totalInterval += tapTimestamps[i].timeIntervalSince(tapTimestamps[i-1])
            }
            let avgInterval = totalInterval / Double(tapTimestamps.count - 1)
            if avgInterval > 0.15 && avgInterval < 3.0 {
                let calculatedBpm = 60.0 / avgInterval
                bridge.setBpm(calculatedBpm)
            }
        }
    }
}

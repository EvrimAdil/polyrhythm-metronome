import SwiftUI

struct MetronomePreset: Identifiable, Codable {
    var id = UUID()
    var name: String
    var bpm: Double
    var timeSignatureNumerator: Int
    var timeSignatureDenominator: Int
    var layerDescriptions: [String]
    var tempoTrainerEnabled: Bool = false
    var muteTrainerEnabled: Bool = false
    var createdAt = Date()
}

@MainActor
final class PresetStore: ObservableObject {
    @Published var presets: [MetronomePreset] = []
    private let saveKey = "SavedMetronomePresets_v1"

    init() {
        loadPresets()
    }

    func savePreset(name: String, bpm: Double, num: Int, den: Int, layers: [String]) {
        let newPreset = MetronomePreset(
            name: name,
            bpm: bpm,
            timeSignatureNumerator: num,
            timeSignatureDenominator: den,
            layerDescriptions: layers
        )
        presets.insert(newPreset, at: 0)
        persist()
    }

    func deletePreset(at offsets: IndexSet) {
        presets.remove(atOffsets: offsets)
        persist()
    }

    private func persist() {
        if let encoded = try? JSONEncoder().encode(presets) {
            UserDefaults.standard.set(encoded, forKey: saveKey)
        }
    }

    private func loadPresets() {
        if let data = UserDefaults.standard.data(forKey: saveKey),
           let decoded = try? JSONDecoder().decode([MetronomePreset].self, from: data) {
            presets = decoded
        } else {
            // Örnek başlangıç poliritmik ön ayarları
            presets = [
                MetronomePreset(
                    name: "Klasik Poliritim 3:4",
                    bpm: 120,
                    timeSignatureNumerator: 4,
                    timeSignatureDenominator: 4,
                    layerDescriptions: ["3 Pulses (Polyrhythm)", "4 Pulses (Polyrhythm)"]
                ),
                MetronomePreset(
                    name: "Afro-Küba 5:4 (Euclidean)",
                    bpm: 132,
                    timeSignatureNumerator: 4,
                    timeSignatureDenominator: 4,
                    layerDescriptions: ["E(5,8) Cinquillo", "4 Pulses Steady"]
                )
            ]
        }
    }
}

struct PresetManagerView: View {
    @StateObject private var store = PresetStore()
    let onSelectPreset: (MetronomePreset) -> Void

    var body: some View {
        NavigationStack {
            List {
                ForEach(store.presets) { preset in
                    Button(action: { onSelectPreset(preset) }) {
                        VStack(alignment: .leading, spacing: 4) {
                            Text(preset.name)
                                .font(.headline)
                                .foregroundColor(.white)
                            Text("\(Int(preset.bpm)) BPM | \(preset.timeSignatureNumerator)/\(preset.timeSignatureDenominator) | \(preset.layerDescriptions.joined(separator: " vs "))")
                                .font(.subheadline)
                                .foregroundColor(.gray)
                        }
                        .padding(.vertical, 4)
                    }
                }
                .onDelete(perform: store.deletePreset)
            }
            .navigationTitle("Ön Ayarlar (Presets)")
            .scrollContentBackground(.hidden)
            .background(Color(white: 0.08))
        }
    }
}

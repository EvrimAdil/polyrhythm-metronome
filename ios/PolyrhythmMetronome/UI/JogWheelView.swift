import SwiftUI

struct JogWheelView: View {
    @Binding var bpm: Double
    let onTapTempo: () -> Void

    @State private var rotationAngle: Angle = .zero
    @State private var previousDragAngle: Angle? = nil

    var body: some View {
        VStack(spacing: 20) {
            // Dairesel Rotary Dial
            ZStack {
                // Kadran Arka Planı ve Çentikler
                Circle()
                    .fill(
                        RadialGradient(
                            gradient: Gradient(colors: [Color(white: 0.18), Color(white: 0.08)]),
                            center: .center,
                            startRadius: 20,
                            endRadius: 110
                        )
                    )
                    .frame(width: 220, height: 220)
                    .overlay(
                        Circle()
                            .stroke(Color.cyan.opacity(0.35), lineWidth: 2)
                    )

                // Dönen Çentikler (Notches)
                ForEach(0..<60) { i in
                    let isMajor = (i % 5 == 0)
                    Rectangle()
                        .fill(isMajor ? Color.cyan : Color.white.opacity(0.25))
                        .frame(width: isMajor ? 2.5 : 1.5, height: isMajor ? 14 : 8)
                        .offset(y: -95)
                        .rotationEffect(.degrees(Double(i) * 6.0) + rotationAngle)
                }

                // Merkez BPM Göstergesi
                Circle()
                    .fill(Color(white: 0.05))
                    .frame(width: 130, height: 130)
                    .overlay(
                        VStack(spacing: 2) {
                            Text("\(Int(bpm))")
                                .font(.system(size: 42, weight: .bold, design: .rounded))
                                .foregroundColor(.white)
                            Text("BPM")
                                .font(.system(size: 13, weight: .semibold))
                                .foregroundColor(.cyan)
                        }
                    )
            }
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { value in
                        let center = CGPoint(x: 110, y: 110)
                        let currentAngle = Angle(radians: atan2(Double(value.location.y - center.y), Double(value.location.x - center.x)))

                        if let prev = previousDragAngle {
                            var delta = currentAngle.radians - prev.radians
                            if delta > .pi { delta -= 2 * .pi }
                            if delta < -.pi { delta += 2 * .pi }

                            rotationAngle += Angle(radians: delta)
                            let bpmChange = (delta / (.pi / 16.0)) * 1.5
                            bpm = max(20.0, min(400.0, (bpm + bpmChange).rounded()))
                        }
                        previousDragAngle = currentAngle
                    }
                    .onEnded { _ in
                        previousDragAngle = nil
                    }
            )

            // Tap Tempo ve +/- Butonları
            HStack(spacing: 16) {
                Button(action: { bpm = max(20.0, bpm - 1) }) {
                    Text("-1")
                        .font(.headline)
                        .foregroundColor(.white)
                        .frame(width: 54, height: 44)
                        .background(Color(white: 0.15))
                        .cornerRadius(10)
                }

                Button(action: onTapTempo) {
                    Text("TAP TEMPO")
                        .font(.system(size: 15, weight: .black))
                        .foregroundColor(.black)
                        .frame(minWidth: 130, height: 44)
                        .background(Color.cyan)
                        .cornerRadius(10)
                }

                Button(action: { bpm = min(400.0, bpm + 1) }) {
                    Text("+1")
                        .font(.headline)
                        .foregroundColor(.white)
                        .frame(width: 54, height: 44)
                        .background(Color(white: 0.15))
                        .cornerRadius(10)
                }
            }
        }
    }
}

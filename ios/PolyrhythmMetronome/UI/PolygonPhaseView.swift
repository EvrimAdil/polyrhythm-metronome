import SwiftUI

struct PolygonLayerItem: Identifiable {
    let id: Int
    let sides: Int       // 3: Üçgen, 4: Kare, 5: Beşgen, vb.
    let color: Color
    var phase: Double   // 0.0 ... 1.0
}

struct PolygonPhaseView: View {
    let layers: [PolygonLayerItem]
    let isPlaying: Bool

    var body: some View {
        TimelineView(.animation) { timeline in
            Canvas { context, size in
                let center = CGPoint(x: size.width / 2, y: size.height / 2)
                let maxRadius = min(size.width, size.height) / 2 * 0.85

                // 1. Dış dairesel kılavuz
                let guideCircle = Path(ellipseIn: CGRect(
                    x: center.x - maxRadius,
                    y: center.y - maxRadius,
                    width: maxRadius * 2,
                    height: maxRadius * 2
                ))
                context.stroke(guideCircle, with: .color(.white.opacity(0.08)), lineWidth: 1.5)

                // 2. Çoklu N-gen Poligon Çizimleri
                for (index, layer) in layers.enumerated() {
                    let radius = maxRadius * (0.4 + Double(index) / Double(max(layers.count, 1) + 1) * 0.6)
                    let sides = max(2, layer.sides)
                    let rotation = layer.phase * 2 * .pi

                    drawPolygon(
                        context: &context,
                        center: center,
                        radius: radius,
                        sides: sides,
                        rotation: rotation,
                        color: layer.color
                    )

                    // Orbital beacon
                    let beaconAngle = rotation - .pi / 2
                    let beaconX = center.x + CGFloat(cos(beaconAngle) * radius)
                    let beaconY = center.y + CGFloat(sin(beaconAngle) * radius)
                    let beaconRect = CGRect(x: beaconX - 5, y: beaconY - 5, width: 10, height: 10)

                    context.fill(Path(ellipseIn: beaconRect), with: .color(layer.color))
                    context.fill(Path(ellipseIn: CGRect(x: beaconX - 2.5, y: beaconY - 2.5, width: 5, height: 5)), with: .color(.white))
                }

                // 3. Faz Çakışma Noktası (Tepe Kesişim Noktası)
                let topApex = CGPoint(x: center.x, y: center.y - maxRadius)
                let apexRect = CGRect(x: topApex.x - 4, y: topApex.y - 4, width: 8, height: 8)
                context.fill(Path(ellipseIn: apexRect), with: .color(.yellow.opacity(0.85)))
            }
        }
        .frame(height: 320)
        .background(Color(red: 0.06, green: 0.07, blue: 0.09))
        .clipShape(RoundedRectangle(cornerRadius: 16))
    }

    private func drawPolygon(
        context: inout GraphicsContext,
        center: CGPoint,
        radius: CGFloat,
        sides: Int,
        rotation: Double,
        color: Color
    ) {
        if sides < 3 {
            var line = Path()
            line.move(to: CGPoint(x: center.x - radius, y: center.y))
            line.addLine(to: CGPoint(x: center.x + radius, y: center.y))
            context.stroke(line, with: .color(color.opacity(0.7)), lineWidth: 2)
            return
        }

        var path = Path()
        let angleStep = (2 * .pi) / Double(sides)
        let initialOffset = -Double.pi / 2 + rotation

        for i in 0..<sides {
            let angle = initialOffset + Double(i) * angleStep
            let x = center.x + CGFloat(cos(angle) * Double(radius))
            let y = center.y + CGFloat(sin(angle) * Double(radius))

            if i == 0 {
                path.move(to: CGPoint(x: x, y: y))
            } else {
                path.addLine(to: CGPoint(x: x, y: y))
            }
        }
        path.closeSubpath()

        context.stroke(path, with: .color(color.opacity(0.75)), style: StrokeStyle(lineWidth: 2.5, lineCap: .round, lineJoin: .round))

        // Köşe noktaları
        for i in 0..<sides {
            let angle = initialOffset + Double(i) * angleStep
            let vx = center.x + CGFloat(cos(angle) * Double(radius))
            let vy = center.y + CGFloat(sin(angle) * Double(radius))
            let vertexRect = CGRect(x: vx - 3, y: vy - 3, width: 6, height: 6)
            context.fill(Path(ellipseIn: vertexRect), with: .color(color))
        }
    }
}

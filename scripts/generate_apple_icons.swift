#!/usr/bin/env swift
// Run on macOS: swift scripts/generate_apple_icons.swift
// Uses the same source artwork as generate_android_icons.py. Generated PNGs
// are checked in, so ordinary Xcode builds do not need to run this script.

import CoreGraphics
import Foundation
import ImageIO
import UniformTypeIdentifiers

let root = URL(fileURLWithPath: #filePath).deletingLastPathComponent().deletingLastPathComponent()
let sourceURL = root.appendingPathComponent("images/laprdus_icon_1024x1024.png")
let output = root.appendingPathComponent("Lapplerdus/Laprdus/Laprdus/Assets.xcassets/AppIcon.appiconset")
guard let source = CGImageSourceCreateWithURL(sourceURL as CFURL, nil),
      let image = CGImageSourceCreateImageAtIndex(source, 0, nil),
      image.width == 1024, image.height == 1024,
      let colorSpace = CGColorSpace(name: CGColorSpace.sRGB) else {
    fatalError("Expected a 1024x1024 source icon at \(sourceURL.path)")
}

// Points and scale are separate in an app icon catalog (76pt at 2x is 152px).
let slots: [(idiom: String, points: Double, scales: [Int])] = [
    ("iphone", 20, [2, 3]), ("iphone", 29, [2, 3]),
    ("iphone", 40, [2, 3]), ("iphone", 60, [2, 3]),
    ("ipad", 20, [1, 2]), ("ipad", 29, [1, 2]),
    ("ipad", 40, [1, 2]), ("ipad", 76, [1, 2]),
    ("ipad", 83.5, [2]), ("ios-marketing", 1024, [1]),
    ("mac", 16, [1, 2]), ("mac", 32, [1, 2]),
    ("mac", 128, [1, 2]), ("mac", 256, [1, 2]),
    ("mac", 512, [1, 2]),
]

try FileManager.default.createDirectory(at: output, withIntermediateDirectories: true)
var entries: [[String: String]] = []
var generated = Set<Int>()
for slot in slots {
    for scale in slot.scales {
        let pixels = Int(slot.points * Double(scale))
        let points = slot.points == slot.points.rounded() ? String(Int(slot.points)) : String(slot.points)
        let filename = "AppIcon-\(pixels).png"
        entries.append([
            "filename": filename, "idiom": slot.idiom,
            "size": "\(points)x\(points)", "scale": "\(scale)x",
        ])
        guard generated.insert(pixels).inserted else { continue }

        // An RGB context removes the source's alpha channel for App Store
        // validation. Keep the square artwork; the OS applies its own mask.
        guard let context = CGContext(
            data: nil, width: pixels, height: pixels, bitsPerComponent: 8,
            bytesPerRow: 0, space: colorSpace,
            bitmapInfo: CGImageAlphaInfo.noneSkipLast.rawValue
        ) else { fatalError("Cannot create \(pixels)px image context") }
        context.interpolationQuality = .high
        context.draw(image, in: CGRect(x: 0, y: 0, width: pixels, height: pixels))
        let destinationURL = output.appendingPathComponent(filename)
        guard let resized = context.makeImage(),
              let destination = CGImageDestinationCreateWithURL(
                destinationURL as CFURL, UTType.png.identifier as CFString, 1, nil
              ) else { fatalError("Cannot create \(filename)") }
        CGImageDestinationAddImage(destination, resized, nil)
        guard CGImageDestinationFinalize(destination) else { fatalError("Cannot write \(filename)") }
        print("Generated \(filename) (\(pixels)x\(pixels), opaque RGB)")
    }
}

let catalog: [String: Any] = ["images": entries, "info": ["author": "xcode", "version": 1]]
var json = try JSONSerialization.data(withJSONObject: catalog, options: [.prettyPrinted, .sortedKeys])
json.append(0x0A)
try json.write(to: output.appendingPathComponent("Contents.json"), options: .atomic)

import CoreGraphics
import Darwin
import Foundation

let processID = Int(CommandLine.arguments[1]) ?? 0

struct Candidate {
    let layer: Int
    let area: Int
    let id: Int
}

func intValue(_ value: Any?) -> Int {
    if let number = value as? NSNumber {
        return number.intValue
    }
    return 0
}

func doubleValue(_ value: Any?, default defaultValue: Double = 0) -> Double {
    if let number = value as? NSNumber {
        return number.doubleValue
    }
    return defaultValue
}

for _ in 0..<30 {
    let windows = CGWindowListCopyWindowInfo(
        .optionOnScreenOnly, kCGNullWindowID) as? [[String: Any]] ?? []
    var candidates: [Candidate] = []

    for window in windows {
        if intValue(window[kCGWindowOwnerPID as String]) != processID {
            continue
        }
        let bounds = window[kCGWindowBounds as String] as? [String: Any] ?? [:]
        let width = intValue(bounds["Width"])
        let height = intValue(bounds["Height"])
        let windowID = intValue(window[kCGWindowNumber as String])
        let alpha = doubleValue(window[kCGWindowAlpha as String], default: 1)
        if width <= 1 || height <= 1 || windowID <= 0 || alpha <= 0 {
            continue
        }
        candidates.append(Candidate(
            layer: intValue(window[kCGWindowLayer as String]),
            area: width * height,
            id: windowID))
    }

    if let candidate = candidates.sorted(by: {
        if $0.layer == $1.layer {
            return $0.area > $1.area
        }
        return $0.layer < $1.layer
    }).first {
        print(candidate.id)
        exit(0)
    }

    Thread.sleep(forTimeInterval: 0.5)
}

fputs("Could not find an on-screen window for PID \(processID)\n", stderr)
exit(1)

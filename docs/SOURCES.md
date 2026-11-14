# Sources and compatibility record

Reviewed 14 November 2026. Upstream documentation is mutable; these links are research references, not pinned dependencies.

## Primary references

- [Swofty Launcher repository](https://github.com/Swofty-Developments/SwoftyLauncher)
- [Official addon template](https://f1.swofty.net/reference/addons/template.html)
- [Telemetry API](https://f1.swofty.net/reference/std/telemetry.html)
- [Overlay API](https://f1.swofty.net/reference/std/overlay.html)
- [Standard library](https://f1.swofty.net/reference/std/)
- [EA F1 25 UDP specification, v3 PDF](https://forums.ea.com/t5/s/tghpe58374/attachments/tghpe58374/f1-games-game-info-hub-en/61/4/Data%20Output%20from%20F1%2025%20v3.pdf)

## Research findings

Swofty documents a local-player index and car input fields, with telemetry on the flow thread and drawing on the render thread.
Its telemetry can combine memory-backed and UDP-backed values. Field-level availability and freshness still require investigation.
The overlay provides line, rectangle and text primitives. Telemetry and overlay are experimental and require the appropriate feature configuration.

EA's 2025 UDP specification represents throttle and brake as floating-point values from 0 to 1.
That does not by itself establish the units, freshness or availability of every Swofty field.

These findings justify an implementation spike. They do not certify a working addon.

## Compatibility matrix

| Item | Current status |
| --- | --- |
| Game build | Not tested or pinned |
| Launcher/core version | Not tested or pinned |
| SDK commit | Not selected |
| Compiler and build tools | Not selected |
| Windows x64 | Intended first target; untested |
| Linux/Proton, VR, split-screen | Outside initial scope |
| 2026 Season Pack | Outside initial support claim |
| Native field freshness | Unverified; release-critical |
| UDP required for pedal values | Unverified |
| Pause/flashback signals | Unverified |
| Game-reported vs raw device values | Intended display is game-reported input |

Update this matrix with actual versions, dates and evidence during M1.
Do not select a minimum supported version merely because a documentation page mentions an older API.

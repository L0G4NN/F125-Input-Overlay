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
| SDK commit | 5d538d9a0afd6f90a417118807d86a5beb9e9c71 |
| Compiler and build tools | Zig 0.14.1, C++20; GCC sanitizer tests |
| Windows x64 | Intended first target; untested |
| Linux/Proton, VR, split-screen | Outside initial scope |
| 2026 Season Pack | Outside initial support claim |
| Native field freshness | Unverified; release-critical |
| UDP required for pedal values | Required: format 2025, loopback port 20777 |
| Pause/flashback signals | Unverified |
| Game-reported vs raw device values | Intended display is game-reported input |

Update this matrix with actual versions, dates and evidence during M1.
Do not select a minimum supported version merely because a documentation page mentions an older API.

## First import package

SDK pinned at 5d538d9a0afd6f90a417118807d86a5beb9e9c71; unmodified MIT C ABI header vendored with licence. Compiler: Zig 0.14.1, target x86_64-windows-gnu. API v5 prefix used; only overlay enabled. UDP parsing follows EA v3: 29-byte header, 1352-byte telemetry packet, 60-byte car stride. This supersedes the preliminary native-telemetry approach. Runtime freshness is measured from accepted packet receipt, overall frame ID and source timestamp. No minimum game build or runtime compatibility has been certified.

Reference documentation rechecked 15 November 2026: see CONFORMANCE.md. The pinned SDK remains unchanged; no claim of tested compatibility with every current launcher/game build is made.

## Lap review source contracts (15 November 2026)

Rechecked the official overlay reference (input/focus v3, virtual-key edges, controller limitations) and EA F1 25 UDP v3 PDF. Added decoders for session packet 1 (753 bytes, track length at 33, track ID 36, spectating 44, network-game 154), lap packet 2 (1285 bytes, 22 x 57-byte records from 29; last lap ms +0, distance +20, lap number +33, invalid +37, driver status +44), and final classification packet 8 (1042 bytes, num cars at 29, 22 x 46-byte entries from 30; completed laps +1, result status +5). All are version 1. Pedals remain packet 6.

Runtime input/focus and pause-mask discovery follows the pinned SDK's `swofty/overlay.hpp` and `swofty/view.hpp` core-export feature checks. No SDK file modifications were needed. No tested game/core version is added until runtime testing is performed.

## Extended telemetry in 0.3.0

See [EXTENDED_TELEMETRY.md](EXTENDED_TELEMETRY.md) for the implemented sector markers, five F6 pages, per-wheel selector, front-wheel steering readout, verified EA offsets and unit limitations. The new total fixed pool is 52.4 MiB, superseding earlier pedal-only 24.2 MiB totals. Pedals retain 2 m bins; extra channels use 16 m extrema bins. Rendering is four 176-column arrays for the selected page only. Existing garage-only/unpaused focus safeguards and socket bounds remain in force. Game validation remains pending.


## Multi-driver review (current)

[Multi-driver review](MULTI_DRIVER_REVIEW.md) supersedes the player-only recording and trace-count descriptions above. X/Y select independent drivers and completed laps. Each car has up to 78 detailed completed laps, one active trace and 1,024 timing entries. Track length determines the per-driver quota under the 192 MiB recorder budget. Garage access and the live HUD follow the local player. Review also opens while paused. Opponent slip/aero data and restricted ERS remain unavailable. See that document for controls, retention limits, packet offsets, rewind handling and validation evidence.

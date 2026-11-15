# Swofty reference review

Reviewed 15 November 2026 against https://f1.swofty.net/reference/ and the pinned MIT SDK at `5d538d9a0afd6f90a417118807d86a5beb9e9c71`.

This is a source and package contract review, not certification of game compatibility. Runtime acceptance remains open.

| Area | Project decision and evidence | Status |
| --- | --- | --- |
| [Manifest](https://f1.swofty.net/reference/addons/manifest.html) | Schema 1, lowercase dotted ID, semver, author L0G4NN, repository URL, native DLL operation, core dependency, overlay experimental feature. No unrelated registry/game content. | Checked against documented fields; actual launcher validation pending |
| [Settings](https://f1.swofty.net/reference/addons/settings.html) | All nine options use native bindings; slider ranges have integral steps, at most 100, and defaults on the grid. Reads and watches use documented C ABI types; finite values clamped. | Source/package checks pass; live changes/persistence pending |
| [Overlay](https://f1.swofty.net/reference/std/overlay.html) | API v5 prefix only; framebuffer size queried per paint, top-left coordinates, RRGGBBAA, scale min(width/1920,height/1080). Registration retries through on_frame. Paint/Feed/socket have one render owner. | Source checked; actual painting pending |
| Overlay cost | Bounded 8,192 samples and 128 receives per paint. At 60 Hz, 5/15 seconds produce about 600/1,800 trace lines; pinned renderer batches compatible geometry. No logging per normal frame. | Bounded work verified; game CPU/GPU frame-time budget unmeasured |
| [Events](https://f1.swofty.net/reference/std/events.html) | Session and pause handlers publish atomic flags; render owns mutable history. Queue gate breaks after reset, pause/resume and render stalls. Optional event hooks may be absent on older cores. | Regression tests pass; event delivery/game lifecycle pending |
| [C ABI](https://f1.swofty.net/std/abi.html) and [compatibility](https://f1.swofty.net/std/compatibility.html) | Exports Init/Api=5, validates required API tables, registers ON_LOAD. No registry records/services/hook points are provided, so no Register export is required. Existing native interfaces remain documented as supported. | Windows x64 compilation/export inspection; runtime load pending |
| [Packaging](https://f1.swofty.net/reference/addons/publishing.html) | `.swoftyaddon` and ZIP contain root swofty.json, compiled DLL, instructions, build info and licence notices. No game assets, logs or generated source artifacts committed. No automatic publishing. | Archive validation pass; actual install/build pending |
| [Testing](https://f1.swofty.net/std/testing.html) | Portable trace/parser/layout/queue tests with GCC; sanitizer runs and Zig cross-build. Follow install, enable, build, run and logs on a Windows game PC before claiming support. | Local checks pass; Swofty game tests/performance/persistence pending |

## Deliberate limits

- Uses custom EA UDP 2025 decoding because the pinned native telemetry fields do not establish per-packet freshness. This is an adapter choice, not a Swofty-provided receiver. Nonblocking loopback receives remain on render; first socket setup can have one-off cost.
- Empty-queue recovery rejects local receive backlog after interruptions. UDP cannot prove end-to-end packet age or authenticate senders. Recently retired session IDs are bounded to 64 entries.
- Socket/callback lifetime is the game process. Restart after changing enabled addons; the native Show overlay toggle releases the socket. Hot DLL unload is not claimed.
- Minimum launcher range describes required interface availability, not tested game support. No tested platform/game-build claims are added without evidence. Linux/Proton, VR, multiplayer, split-screen, 2026-format input, HDR/frame generation remain unsupported or untested.
- No registry content, game modifications, launcher plugin, custom UI scenes, saves or publishing integration are part of this addon; those reference sections do not apply.

## Manual completion

Install the `.swoftyaddon` on the Addons page (or `swofty addons install FILE`), enable `l0g4nn.input-overlay`, run `swofty build`, then `swofty run --apply` and `swofty logs`. Verify settings restart persistence, game pause/resume and flashback, new sessions, port conflict, disable/re-enable, render stalls and 30-minute baseline versus enabled frame times. Record exact game/launcher/core versions and hardware in SOURCES.md. Automated game tests can be added once a supported runtime fixture is available.

## Experimental lap-review extension (0.2.0)

Runtime `SwoftyCoreOverlayVersion >= 3` gates every input/focus field access; older cores keep the API-v5 live HUD. `SwoftyCorePauseReasons` is queried only when exported. The same documented core-export approach is used by the pinned C++ wrappers; no new memory offsets/hooks are introduced. The v3 extension remains a runtime gate, not a claim that all launchers meeting the existing minimum support the review menu.

New native bindings use a toggle and integral 6–12 slider; package API version remains 5. UDP decoders use EA 2025 v3 sizes/strides documented in SOURCES.md. Recording is memory-only and bounded; performance and input-focus conformance need the real-game matrix. See REVIEW_VALIDATION.md.

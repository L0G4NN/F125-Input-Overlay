# Test plan

Core checks pass with GCC, AddressSanitizer and UndefinedBehaviorSanitizer (leak detection disabled because this host runs under ptrace). Covered: overlapping inputs, duplicate timestamps, pause/resume, gaps, invalid values, rewind, ring capacity, age eviction, settings bounds and graph mapping at 30/60/144 Hz. No native addon or in-game checks have run.

## Automated core tests to add with code

| Area | Meaningful cases |
| --- | --- |
| Input values | 0%, 50%, 100%, simultaneous inputs, non-finite data, out-of-range data |
| History | Empty, single sample, age eviction, capacity wrap, duplicate timestamp |
| Time | Identical trace timing at 30/60/144 FPS; irregular update cadence; backward time |
| Gaps | Source loss, cached reads, fresh unchanged values, recovery without a connecting line |
| Lifecycle | Pause/resume, flashback, restart, new session and changed local player |
| Geometry | Endpoint mapping, clipping, scale and resized viewport |
| Settings | Invalid values, defaults, range clamping and coherent application |
| Ownership | Concurrent producer/renderer stress; no partially written snapshot or unload use-after-free |

Synthetic fixtures should represent actual expected behaviours: full throttle, a hard brake step, progressive release, throttle pickup, overlap and a missing-data interval.
They must be labelled synthetic and must not be used to claim game compatibility.

## Native integration checks

Verify API availability failures, callback registration errors, feature-disabled behaviour, supported schema/ABI checks and cleanup.
Check that the local-player index is resolved from session data.
Establish a source freshness mechanism before claiming stale-data detection.

## Manual game matrix

Run on the exact versions recorded in SOURCES.md:

- Grand Prix practice and an offline race; assess Time Trial only within the launcher's supported mode.
- Controller and wheel/pedals where available.
- Zero, half and full input; simultaneous pedals; constant input for several seconds.
- Pause/menu, flashback, restart, return to menu and load another session.
- Telemetry source loss and recovery, including whether UDP must be enabled.
- 1080p and 1440p; resize and DPI scaling; HDR/frame generation if claiming support.
- Settings changes and persistence after restart.
- Clean launch with only this addon, then coexistence with another HUD addon.
- Disable/re-enable and normal game exit without leaked callbacks or crashes.

## Performance target

Measure baseline versus enabled overlay under matching conditions.
Initial targets, not guarantees: bounded memory with no growth during a 30-minute session, no blocking work in render callbacks, and no material frame-time regression.
Record CPU/GPU, resolution, frame generation, sample rate, settings and measurement method.
Set a numerical release budget once a representative baseline exists.

## Release gate

Do not release until core tests pass, the pinned native build succeeds, package validation passes and a real in-game smoke test verifies both traces.
Record missing matrix coverage as unsupported or untested.

## 0.1.0 package evidence

Zig 0.14.1 successfully cross-compiled the Windows AMD64 DLL. PE inspection confirms SwoftyAddonApi and SwoftyAddonInit exports and system DLL imports only. Added synthetic EA UDP packet checks for size/version/format/player validation, values, missing data, duplicates, ordering, flashback and session resets. Archive CRC and manifest DLL path checks pass. Actual launcher validation/import and in-game display remain pending; no Wine or Windows game runtime is available here.

## 0.1.4 regression coverage

Automated cases cover explicit resets followed by late old-session packets, same-UID restart/rewind, reordered player changes, invalid pedal gaps and frame wrap. Portable receiver-gate tests cover render stalls, initial open, session reset, pause/resume and budget exhaustion until an empty queue. Actual Winsock receive scheduling and live game recovery remain manual checks. Sanitizer tests, Windows compilation and archive validation do not replace the documented Swofty install/build/run/log checks.

## Packet validation before source tracking

Regression tests verify malformed other-session packets cannot retire a legitimate session, invalid far-forward frames do not block subsequent real frames, and invalid old/other-player packets do not disturb live state. Pause/resume preserves tracking. Header and pedal decoding occur once per candidate, before identity/order commits; storage and receive limits are unchanged.

## Lap review tests

`bash scripts/test.sh` includes portable store/plot tests and a fake overlay test for v3/focus gating, closed-state drawing, feature fallback and focus cleanup. Additional commands and measured synthetic profiles are in REVIEW_VALIDATION.md. On Windows verify fresh garage/pause gating, exact packet frame pairing, F8/rebinding, Alt-Tab, controllers/wheels, both lap selections, full races and final-lap handling before claiming runtime support.

## Setup naming regression

Portable `hud_visibility_test` checks garage/pause/unknown/stale suppression, explicit override and unallocated-pool metadata ingestion under ASan/UBSan. Fake-overlay tests check typing while closed never acquires focus and typing/Enter/gamepad input while open releases it. The Windows build and package check pass. These are policy tests, not proof that issue #8 is resolved visually. Run the stock/core-only/addon naming matrix in SETUP_NAMING.md on the affected game/runtime.

## 0.2.2 behaviour update

Interactive F8 review is now **garage-only and unpaused**, superseding earlier paused-review descriptions. Any pause closes it, even with fresh garage metadata; held keyboard/controller controls prevent focus acquisition. Test entering flashback with throttle held/released, both with review closed and after leaving the garage review, on keyboard, controller and wheel. Compare addon-disabled and Swofty-core-only runs if the fault persists: the addon never writes throttle or injects controls, and no exact replay/flashback query is available in the pinned public API. This is an input-capture mitigation, not a confirmed game-level fix.

Lap rows use separate timing and quality lines. X (cyan) and Y (amber) use separate lanes by default; “View: overlay” puts them on shared pedal axes. Connected upper-envelope lines plus min/max markers replace isolated dots. A connection requires a recorded predecessor link, including sparse bins at racing speed; gaps remain disconnected and no smoothing is applied. Zoom to inspect detail. The bin pool remains 24.2 MiB; four 192-column caches and at most 1,536 trace-line calls bound open-menu work independently of lap count.

## Extended telemetry in 0.3.0

See [EXTENDED_TELEMETRY.md](EXTENDED_TELEMETRY.md) for the implemented sector markers, five F8 pages, per-wheel selector, front-wheel steering readout, verified EA offsets and unit limitations. The new total fixed pool is 52.4 MiB, superseding earlier pedal-only 24.2 MiB totals. Pedals retain 2 m bins; extra channels use 16 m extrema bins. Rendering is four 176-column arrays for the selected page only. Existing garage-only/unpaused focus safeguards and socket bounds remain in force. Game validation remains pending.

## Paused review and rewind rollback (current)

This supersedes earlier garage-only and whole-session rewind-reset policies. F8 is allowed in an unpaused fresh garage or a pause entered with fresh lap metadata; that context is latched because paused UDP can stop. Resume on track releases focus. Held controls and non-review navigation still release/refuse focus. Pause masks do not identify replay/flashback reliably: close F8 before flashback controls; the previous stuck-input report remains an in-game validation risk.

A newer overall frame with backwards source time or lap number starts rollback. Recording waits for validated lap metadata, drops pending pairs and rejects older frames. Completed laps before the target survive; the target lap reopens with its retained prefix, and later laps are removed. Discard the entire boundary bin and suffix (2 m pedals, 16 m extended channels), not just extrema, to avoid mixing attempts. Missing target history starts partial. Rewritten laps are conservatively partial and retain conservative quality flags. Selections/cache generation resets. No allocation: rollback scans at most 1,024 lap records plus one pedal slot and 14 extended channel suffixes; normal recording cost and fixed pool size are unchanged.


## Multi-driver review (current)

[Multi-driver review](MULTI_DRIVER_REVIEW.md) supersedes the player-only recording and trace-count descriptions above. X/Y select independent drivers and completed laps. Each car has up to 70 detailed completed laps, one active trace and 1,024 timing entries. Track length determines the per-driver quota under the 256 MiB recorder budget. Local HUD and the PR #15 pause/focus gates still use only local-player state. Opponent slip/aero data and restricted ERS remain unavailable. See that document for controls, retention limits, packet offsets, rewind handling and validation evidence.


## F8 pause access update

F8 now opens whenever Swofty reports a pause, without a fresh-UDP or pre-pause latch requirement. Held keys, controller buttons, triggers, stick drift and other navigation no longer block opening or automatically close review. F8, Escape and Close still close it; resuming on track, window/focus loss, reset and disable still release focus. Unpaused access remains garage-only. Close review manually before setup typing or flashback controls. This supersedes earlier freshness/held-control restrictions above; in-game behaviour remains to be verified.

Driver dropdown regression covers independent X/Y selection, reaching car 22 by pagination, wheel scrolling, outside-click consumption, Escape dismissal, and geometry bounds. No recorder or pause-policy changes are made.

## F8 theme (PR #17)

The charcoal/slate theme uses existing rect/line/text APIs only, with a shared RRGGBBAA palette. Portable dropdown, focus, five-page comparison and 16,384-vertex budget checks pass with the new controls and axis labels; Windows x64 cross-compilation passes. A synthetic drawing capture was inspected for spacing. The mockup is design guidance, not a game screenshot; actual font rendering, DPI and game readability remain manual checks. No telemetry, recording, live-HUD or input-policy changes.

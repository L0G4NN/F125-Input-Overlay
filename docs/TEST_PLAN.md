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

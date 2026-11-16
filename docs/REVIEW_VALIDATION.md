# Experimental lap review validation

15 November 2026. Local Linux development host, GCC 14 C++20 portable tests; Zig 0.14.1 Windows AMD64 cross-build. No F1 25, Windows socket runtime or GPU session was used. Synthetic packet fixtures are not captured game telemetry.

## Reproduce

```sh
ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh

g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror \
  -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer \
  -Isrc tests/lap_review_fuzz.cpp -o build/lap_review_fuzz
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ./build/lap_review_fuzz

g++ -std=c++20 -O2 -Isrc tests/review_benchmark.cpp -o build/review_benchmark
./build/review_benchmark

ZIG=/path/to/zig bash scripts/build.sh
python3 scripts/package.py
```

Leak detection is disabled because this host is run under ptrace. ASan/UBSan remain enabled on the relevant tests. The fake overlay tests exercise our focus/state logic, not actual Windows message delivery.

## Completed checks

- Trace, existing UDP validation/ordering, receive recovery and layout regressions.
- Portable lap store: completion once, last-lap timing association, extrema/spike preservation, reversed packet arrival, missing exact-frame pairs, stale garage status, rewind generations, malformed metadata and 200-lap trace/metadata limits. Record off/on, final classification with unknown last-lap time and session replacement are covered.
- Fake overlay: no drawing while closed, no input polling while unsafe, v3 feature gate, F8, focus failure/steal, Alt-Tab state loss and Escape release. Rendering uses a fixed number of columns and no whole-session scan.
- 300,000 deterministic mutated packet/state cases with session/lap/pedal/classification packets, arbitrary lengths, reset/record toggles, and plot bounds under ASan/UBSan/float-cast checks. This is randomized fuzzing, not coverage-guided libFuzzer.
- Windows DLL compilation with warnings as errors; manifest and archive structure validation. Exports and system DLL imports should be inspected for each supplied build.

## Memory

| Storage | Bytes |
| --- | ---: |
| Fixed 129 x 8,192 x 24-byte bin pool | 25,362,432 |
| Recorder metadata, pairing/order/lifecycle state on this host | 34,360 |
| Four 192-column plot envelopes and connectivity | 18,432 |

Total review-owned storage is about 24.25 MiB, below the 32 MiB budget, before small allocator bookkeeping/Panel scalars and upstream rendering buffers. Pool allocation and initialization happen once during ON_LOAD. The pool is retained until game exit; recording off does not free/reallocate it. Generation stamps avoid a 196 KiB trace clear or copy during each lap transition.

The benchmark overrides C++ new/delete to count allocations after packet fixture and pool preparation. It observed zero C++ allocations during ingestion and plot rebuilds. There are no threads, locks, file writes, network sends or unbounded containers added to those paths.

## Synthetic CPU profiles

One local optimized run, average time for a validated metadata+pedal pair over 300,000 iterations per scenario:

| Completed laps retained | Pair ingestion (microseconds) | New C++ allocations |
| --- | ---: | ---: |
| 0 | 0.031 | 0 |
| 78 | 0.032 | 0 |
| 128 | 0.031 | 0 |
| 200 | 0.029 | 0 |

Fixture byte updates are included; socket receive, the live HUD, operating system input calls and game/GPU work are excluded. Values are microbenchmarks on shared hardware and should not be interpreted as a guaranteed game callback budget. The important structural result is unchanged bounds/allocation count at high lap counts.

For the previous 0.2.0 renderer, an additional development-only profile fed 128 full 16,384 m synthetic laps, selected X/Y, and drew the cached menu through the pinned upstream portable HUD geometry/text implementation. Over 2,000 paints it averaged about 49.3 microseconds CPU, with 12,306 vertices in one batch. This excludes actual D3D12 upload, driver/GPU work, Winsock and Windows focus. The scratch profile depends on the separately checked-out upstream source and is not part of the standalone project test command. The project's fake-overlay test remains the portable input/state check.

## Still required on a gaming PC

Record exact game, launcher/core and hardware versions. Verify metadata field behaviour and exact overall-frame pairing from real packets, lap transitions/last-lap times in qualifying and full races, garage/pause checks, F8/rebinding, coexistence and controller/wheel limitations. Compare disabled, recording-with-menu-closed and open-X/Y scenes using CPU/GPU frame-time distributions and 30-minute memory high-water observations. Do not advertise zero performance impact or full runtime compatibility from these local results.

## 0.2.2 follow-up checks

ASan/UBSan regression tests and 300,000 mutated review packet/state cases pass. New cases cover connected sparse samples, explicit and subpixel gaps, zoom boundaries, held controls on the F8 frame, pause closing without further input reads, and both comparison layouts with twelve quality-labelled rows plus the cursor readout within the 16,384-vertex budget. Zig 0.14.1 Windows compilation and packaging pass.

The pool is unchanged. Allocation-count tests again observed zero new C++ allocations during ingestion and plot rebuilds at 0/78/128/200 completed laps. The updated portable upstream draw-list profile with 128 full laps and X/Y selected used 12,552 vertices in one batch and averaged 49.1 microseconds per cached paint on this host. These are synthetic CPU observations, excluding GPU, Windows input and socket scheduling. Actual F1 25 readability and the reported flashback/stuck-throttle fault remain unverified. No claim that the input-capture mitigation fixes a core/game input problem is made.

## 0.3.0 extended channels

All regression tests and 300,000 deterministic mutated packet/state cases passed with ASan/UBSan (and float-cast checks for the new decoder/fuzzer). LeakSanitizer remains disabled for this ptrace host. Added independent decoder fixtures verify Status/Motion Ex offsets, per-car strides, exact lengths, invalid floats, sector ordering, steering freshness and late-packet sealing. Every graph page is exercised with X/Y data and sector markers under the drawing budget.

Fixed pool: 54,951,936 bytes; recorder metadata: 38,000 bytes on this host. The four-packet allocation benchmark observed zero new C++ allocations during ingestion and plot rebuilds at 0/78/128/200 completed laps. Four-packet ingestion averages were about 0.54/0.74/0.40/0.30 microseconds respectively on shared hardware; timings are illustrative, not a game performance guarantee.

A development-only 128-lap profile using the pinned upstream portable draw-list produced 10,962–13,332 vertices across the five pages with X/Y, sectors and cursor. Synthetic page layouts were inspected with an approximate-font raster preview; actual Swofty font/D3D12 rendering remains unverified. Windows cross-compilation with Zig 0.14.1 and package validation passed. See EXTENDED_TELEMETRY.md for the real-game acceptance matrix and raw-unit limitations.

## Paused review and rewind rollback (current)

This supersedes earlier garage-only and whole-session rewind-reset policies. F8 is allowed in an unpaused fresh garage or a pause entered with fresh lap metadata; that context is latched because paused UDP can stop. Resume on track releases focus. Held controls and non-review navigation still release/refuse focus. Pause masks do not identify replay/flashback reliably: close F8 before flashback controls; the previous stuck-input report remains an in-game validation risk.

A newer overall frame with backwards source time or lap number starts rollback. Recording waits for validated lap metadata, drops pending pairs and rejects older frames. Completed laps before the target survive; the target lap reopens with its retained prefix, and later laps are removed. Discard the entire boundary bin and suffix (2 m pedals, 16 m extended channels), not just extrema, to avoid mixing attempts. Missing target history starts partial. Rewritten laps are conservatively partial and retain conservative quality flags. Selections/cache generation resets. No allocation: rollback scans at most 1,024 lap records plus one pedal slot and 14 extended channel suffixes; normal recording cost and fixed pool size are unchanged.

Validation for paused review/rollback: portable suite and targeted cross-lap/repeated/trace-cap rollback checks passed under ASan/UBSan; 300,000 mutated packet/state cases passed with float-cast checks. Zig 0.14.1 Windows build passed. These do not verify flashback control behaviour in F1 25.

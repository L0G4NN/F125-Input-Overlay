# Session lap review and comparison

Status: implementation specification, documentation only. No F8 panel or full-lap recorder exists in the current 0.1.4 build. Maintainer: L0G4NN.

## Intended behaviour

Press F8 to toggle a custom telemetry review panel. After a qualifying lap, return to the garage, open it, select Lap X and optionally put Lap Y over it. The list shows completed laps recorded during the current session, lap time, validity and recording quality. Changing selection overlays the two laps on the same distance axis. This is a standalone Swofty overlay, not an insertion into Race Director or the setup screen.

The existing scrolling live trace remains available independently. Closing the panel stops review rendering and input capture; bounded session recording continues while enabled. Opening or closing does not change the game's pause state, car controls, setup or saves. No disk recording, export, network upload or automatic sharing is included.

## Controls and presentation

| Control | Behaviour |
| --- | --- |
| F8, default configurable key | Toggle once per key-down edge; holding must not repeatedly toggle |
| Escape / Close | Close and release focus; do not consume Escape while closed |
| Lap X selector | Primary lap; solid green throttle and red brake |
| Lap Y selector | Optional comparison lap; dashed green/red with an explicit X/Y legend |
| Remove comparison | Return to one lap; same lap selected twice produces a clear no-op |
| Wheel over graph | Zoom about cursor within recorded distance range |
| Drag within graph | Pan the visible distance window; no dragging the car/game menus |
| Reset view | Fit the full lap distance |
| Cursor | Show distance and both laps' available pedal values; show dashes through gaps |

Throttle and brake use separate stacked 0–100% plots sharing a distance axis in metres. Four series at most: two channels for each of two laps. Never align by percentage of lap time or stretch a shorter recording to fit a longer one. No lap-time delta curve in the first implementation.

Initial defaults: open closed, X = most recent completed lap, Y = none. Record invalid laps too, clearly labelled; do not silently choose one as a reference. Display garage/out-lap/in-lap classifications only when verified. Unknown validity, time or lap type stays unknown rather than fabricated. Opening without data shows why: waiting for lap metadata, no completed laps, recording disabled, or limits reached.

## Input and API compatibility gate

Use Swofty's documented overlay input and focus interfaces. Query the runtime overlay version before accessing v3 table fields; the current API-v5 drawing prefix alone does not guarantee v3 input. Size input structs correctly for the pinned ABI. Recheck the SDK and minimum core/launcher requirement when implementing; update compat only with evidence.

The [overlay reference](https://f1.swofty.net/reference/std/overlay.html) documents input/focus v3, Windows virtual-key codes and an F8 opening example. Use key-pressed rather than auto-repeat. Only poll the focused game window; do not install global keyboard hooks or raw OS polling. Respect text-entry suppression and another addon's focus. Rebinding must reject a missing/invalid key and provide a recoverable default through native settings.

Focus blocks many keyboard/mouse messages, but gamepads and wheels still reach the game. Initially allow opening only in a positively verified garage or paused state. If that state cannot be established, fail closed with a clear explanation; do not infer garage/pause from UDP silence. Close/release focus on transition back to driving, session replacement, disable and loss of focus. Do not auto-reclaim focus stolen by another addon. Close on Alt-Tab where the supported state can detect it, and test reacquisition without stuck input.

A supported focus failure must leave the menu closed, without a captured cursor. Missing v3 support disables only review interaction, with a diagnostic; the existing live overlay should remain usable. This behaviour needs real game verification before a support claim.

## Recording inputs and authoritative lap metadata

The current receiver decodes Car Telemetry packet 6 only. Pedal values do not establish lap completion, distance, lap time or validity. Extend ingestion with validated EA UDP 2025 lap/session metadata, or a verified supported Swofty source. Select the source after checking the pinned SDK and EA spec; no guessed packet offsets or cached freshness assumptions.

Required metadata: session UID/generation, local player, track/layout identity and length, lap number, lap distance, current/last lap time, validity and garage/pause state. Session-history data may confirm completed lap times when available. The [telemetry reference](https://f1.swofty.net/reference/std/telemetry.html) exposes lap/time/distance fields, but availability and independent packet freshness must be proved. No lap-time subtraction across a pause or rewind.

Validate packet length, format, version, index, finite/ranged values and source ordering before any session/frame/retirement state mutation. Keep ordering per packet stream/type: a lap-data packet and telemetry packet may share an overall frame and must not reject one another. Ignore unrelated packets and maintain the existing fixed receive budget and exclusive loopback binding.

Pair distance and pedals only for the same session/player and verified nearby source times/frames. Proposed maximum pairing age: 100 ms at documented 30/60 Hz output; test and adjust with fixtures. Never reuse a cached distance indefinitely. Keep unmatched candidates bounded. Discard ambiguous pairs, mark a gap and wait for fresh metadata. Handle samples before the start line, negative out-lap distance and distance wrap explicitly; never clamp them into an invented lap-start sample.

## Lap lifecycle

| Situation | Required result |
| --- | --- |
| Attach partway through a lap | Record a partial lap; never call its trace complete |
| Verified finish-line/lap-number transition | Finalize once, only with consistent session/player and timing evidence; split old/new samples correctly |
| Missing transition packets or skipped lap numbers | Add known lap metadata if available, mark trace missing/partial; never invent intermediate laps |
| Stationary in garage or pause | No fabricated samples; retain completed laps; pause recording appropriately |
| Invalidated lap | Retain it with invalid status and honest data-quality marking |
| Temporary UDP loss / render stall / receive-budget exhaustion | Reuse queue-recovery protection; preserve completed laps, break active coverage |
| Flashback / time rewind | Roll back active samples and remove/invalidate completed laps superseded by the rewind; invalidate selections and caches; if position cannot be established, reset the recording generation |
| Session UID/player/track/layout change or restart | Clear session recordings and selections before accepting new identity data; no cross-track comparisons |
| Session ends | Close review, release focus and clear the store; no persistence between sessions |
| Recording disabled | Discard partial lap and stop capture; retained completed laps can be reviewed until session ends |

Use a stable session-generation/lap identifier internally, not the displayed list index or lap number alone. Selection must never refer to freed/reused storage after rollback/reset/capacity handling. Commit finalization only once. Clear or label pending times until authoritative timing arrives; metadata updates may not re-add a lap undone by flashback.

## Bounded memory and drawing cost

Performance budgets below are proposed implementation limits, not measured guarantees. Verify sizes, high-water usage and frame times before enabling by default.

- Target a total lap-review memory ceiling of 32 MiB, including metadata, active/finished traces, temporary pairing storage and geometry caches. Do not allocate 32 MiB eagerly during a frame callback.
- Proposed compact representation: 2 m distance bins retaining min/max throttle and brake, coverage/segment flags and distance. At most 8,192 bins per lap and 128 detailed lap traces. A 24-byte bin yields 24 MiB for 128 full traces; remaining budget covers bounded metadata/scratch/caches. Confirm actual sizeof and all allocation overhead. Track lengths outside supported coverage must produce a visible limit state.
- Keep bounded metadata entries for up to 1,024 completed laps. At a trace or memory limit, preserve existing records and mark later laps as metadata-only (recording limit reached). At the metadata cap, stop adding entries and show the count limit. Never silently claim every lap was recorded, evict a selected lap or grow storage unboundedly. The normal-session goal is every completed lap; long/pathological sessions have these explicit limits.
- Distance binning is a display/storage reduction: expose it as such. Preserve within-bin extrema and gaps so brief braking spikes survive. Do not describe envelopes as lossless raw pedal samples. Cursor values in a bin should show a range when ambiguous rather than invented precise values.
- Bin updates use constant bounded work per accepted sample; no whole-lap scans, sorting, compression, per-sample allocation, file access, locks or worker waits in render callbacks. Large distance jumps mark missing coverage instead of filling thousands of bins synchronously.
- Use the current single render-owner model for UDP ingestion, recording store and review drawing where possible. Cross-thread callbacks publish atomic flags only. If a future native metadata source needs another thread, design and stress-test bounded ownership transfer before implementation.
- Draw only visible distance bins for X/Y. Aggregate to pixel columns while preserving extrema; cap review geometry at 16,384 vertices per frame so other addons retain most of Swofty's shared 131,072-vertex capacity. Rebuild bounded caches only on lap selection, zoom, pan, viewport or relevant active-data changes. No review geometry work while closed.
- Keep original socket receive and live trace limits. Excess traffic may cause missing data rather than unbounded work. No per-frame logs, extra sockets or session-history copies on every frame.

## Comparison correctness

Only compare laps from the same recording generation, player, track/layout and distance calibration. Choose the verified full track length as the axis extent; display only actual coverage. No extrapolation into missing start/end sections or interpolation across gaps. Surface partial, invalid or reduced-resolution data in the legend.

For a missing or ambiguous bin, draw a gap. Valid samples separated by a discontinuity must never join, including when both land in the same distance bin. Two laps sharing a bin can have different sampling densities; the UI must retain each lap's actual coverage independently. Repeated visits to a distance after flashback must not merge abandoned and replacement attempts.

## Implementation milestones

1. Prove overlay v3 input/focus, garage/pause gate, local-player metadata freshness and lap transitions on the pinned SDK and a recorded game/core version. Log missing capability once. No code offsets or native menu patching.
2. Implement a portable bounded lap store and metadata pairing. Add deterministic tests for every lifecycle row and budget boundary before wiring UI.
3. Add F8 menu, list, X/Y selectors, plots and focus cleanup. Preserve the existing live trace and settings. Review key binding UI support before adding manifest options.
4. Add zoom/pan/cursor, min/max bin rendering, fixed geometry budgets and cache invalidation. Exclude delta-time coaching, file exports, persistent recordings and other channels.
5. Run sanitizers, fuzzing, Windows build/package checks and the manual game/performance matrix. Update INSTALL, REQUIREMENTS, SOURCES, CONFORMANCE and changelog with actual capability/version evidence before a release.

## Acceptance and tests

- Synthetic laps with different speeds show a brake event at the same distance, not the same timestamp. Cover simultaneous pedals and isolated single-bin spikes.
- Complete, invalid, partial, out/in laps, unknown metadata and no-data cases have correct labels. Last-lap timing is associated with the lap that ended, not the new lap.
- Test independent packet ordering, truncation, index bounds, non-finite distance/time/input, hostile session/frame changes, duplicate completion and packet loss. An invalid packet cannot mutate tracking or allocate based on claimed lap counts/track length.
- Fuzz randomized packet/lap sequences with ASan/UBSan and assertions for bounds, coverage, ownership, selections and budget limits. Exercise every capacity and flashback boundary.
- F8 toggles once, Escape closes, text entry does not open the panel, unsupported v3/focus/state leaves game input available. Test Alt-Tab, another overlay taking focus, resize, disable and return to driving. Explicitly verify controllers/wheels are not mistakenly assumed captured.
- Record a qualifying lap, return to the garage, open F8, select X/Y, inspect braking, close, and restart the session. Verify no laps cross the session boundary.
- Compare the same game scene with feature disabled, recording on/menu closed, and menu open with two maximum-size traces. Include 30/60 Hz telemetry and 60/144/240 FPS, low FPS, flood/budget recovery and 30-minute memory high-water checks.
- Initial profiling target: closed-menu recording adds less than 0.1 ms CPU per render callback at normal 60 Hz telemetry; open review adds less than 0.5 ms CPU per presented frame, with no growing memory. These are engineering targets only; measure CPU and GPU frame-time distributions and revise design if missed. Never claim zero FPS impact from a synthetic CPU test.

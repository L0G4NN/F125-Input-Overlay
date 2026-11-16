# Proposed architecture

This design is provisional until the compatibility spike passes.

## Components

| Component | Responsibility |
| --- | --- |
| Swofty adapter | Read local-player inputs and session metadata; expose validity and freshness |
| Trace core | Normalize validated values, manage time and bounded history, handle discontinuities |
| Presentation | Map time and amplitude to graph coordinates; produce trace segments, bars and labels |
| Overlay adapter | Draw presentation primitives through supported Swofty APIs |
| Settings adapter | Validate settings, apply defaults and publish coherent snapshots |

Keep trace logic independent of Swofty headers so it can be tested without the game.
The addon must read state only. It must not patch physics, inject controls or modify saves.

## Flow

1. Read the session and resolve the local player.
2. Validate the source result, field availability, timestamps and input range.
3. Append a confirmed fresh sample or transition to paused/unavailable state.
4. Publish a complete immutable view of history and settings.
5. Draw from that snapshot on the overlay callback.

The native telemetry read belongs on the supported flow thread; rendering belongs on the render thread.
Do not share a mutable ring buffer or settings object between them.
Choose a bounded snapshot handoff with explicit ownership and lifetime during implementation.
Do not assume a two-buffer swap prevents races while the renderer is still reading.
No blocking waits, file I/O or unbounded allocation in callbacks. The implemented adapter permits bounded nonblocking loopback receives on the render owner.

## History and plot mathematics

Use timestamped samples, not a fixed number of rendered frames.
Proposed storage budget: 8,192 samples, a maximum history of 15 seconds ; there is no explicit ingest rate limit.
If downsampling is needed, preserve extrema or document the resulting limit; do not silently erase short braking spikes.
Evict by age and capacity, and break segments at source gaps.

For graph origin (x0, y0), width W, height H, history T, current trace time t and sample time ti:

- Horizontal coordinate: x = x0 + W × (1 − (t − ti) / T).
- Vertical coordinate: y = y0 + H × (1 − input).
- Input 0 maps to the bottom; input 1 maps to the top.
- Only samples in the visible interval are drawn; clip segments to the graph rectangle.

Use one authoritative trace clock derived from validated session timing or another verified source.
Wall-clock time can detect missing updates but must not advance a paused trace.
Identify a rewind before inserting its first sample.

## State transitions

| State | Display and history |
| --- | --- |
| Disabled | No overlay; discard or suspend ingestion according to a documented policy |
| Waiting | Empty graph and waiting status |
| Live | Append fresh samples and display live values |
| Paused | Freeze history and mark paused |
| Unavailable | Break the graph; display dashes instead of fabricated percentages |
| Reset | Clear history on restart, rewind, session or player change, then wait for fresh data |

Re-enabling starts a new history. Resuming after data loss starts a new segment.
Repeated errors are rate-limited in logs. No per-frame logs in ordinary operation.

## Dependency policy

Prefer supported native telemetry and overlay APIs. Pin an upstream commit before copying any headers and retain required notices.
Do not copy memory offsets or create signature hooks as an unreviewed fallback.
If field freshness cannot be established natively, assess EA's UDP output as a separate adapter before changing the design.
The current adapter uses a documented EA UDP decoder and Swofty drawing, without an external renderer.

## 0.1.0 implementation

The first native build uses a render-thread-owned UDP receiver and trace instead of native cached telemetry. No history is shared across threads. Only atomic settings and lifecycle flags cross threads. This supersedes the preliminary flow-thread sampling/snapshot proposal above. Silent data loss is marked unavailable; source timestamps reset on rewind, and session events clear history.

## Receiver recovery

Feed.clear clears display history while retaining source identity and overall-frame ordering. A changed session retires its predecessor; player changes are checked against session frame ordering first. Invalid pedals never change session identity, retirement or ordering. A newer invalid packet for the established session/player marks unavailable, so the next valid packet starts a new segment. Old or other-player invalid packets are ignored. Paused ingestion returns without changing source tracking.

ReceiveGate discards the socket queue after initial open, reset, pause/resume, render gaps over 500 ms and budget exhaustion. Discarding stays bounded at 128 datagrams per paint and continues until WSAEWOULDBLOCK. Later paints accept new arrivals. UDP has no authenticated send timestamp: this protects against local receive backlog, not arbitrary upstream delays or a malicious local sender.

## Session lap review (0.2.0)

The single render owner also ingests validated session/lap metadata, pairs it with same-frame pedals, and updates one 2 m envelope bin. Recording uses a startup-allocated fixed pool; completed laps transfer slot ownership with no whole-lap copies or clears. Stamps invalidate reused bins. At the 128-trace cap, subsequent completed laps are metadata-only.

The F8 panel queries v3 input only in a fresh garage while unpaused and releases focus on close/state loss. The game pause state is never changed. The renderer aggregates only selected X/Y laps into fixed 192-column caches; no plot rebuilding or review drawing occurs while closed. All callback-crossing lifecycle/settings signals remain atomic. See LAP_REVIEW.md and REVIEW_VALIDATION.md for exact storage, pairing, rewind and validation limits.

## Game-menu visibility and input handoff (0.2.1)

Fresh session/lap visibility tracking is independent of the optional trace pool. The decoder can update garage/on-track status without allocation; recording and pairing still require a successfully initialized pool. The default live HUD gate is fresh non-garage metadata AND not paused. Stale/unknown contexts produce no live primitives. A native toggle restores legacy visibility only for troubleshooting.

The review panel has no text fields or controller navigation. If it owns focus, typed characters, non-review keys, XInput button edges or active axes/triggers close it. It never replays captured input, captures while closed, or releases another addon's focus intentionally. This supplements existing close/Escape/state-loss paths. No exact game text-entry dialog query is exposed in the pinned public API; no private font/input hooks are added.

## 0.2.2 behaviour update

Interactive F8 review is now **garage-only and unpaused**, superseding earlier paused-review descriptions. Any pause closes it, even with fresh garage metadata; held keyboard/controller controls prevent focus acquisition. Test entering flashback with throttle held/released, both with review closed and after leaving the garage review, on keyboard, controller and wheel. Compare addon-disabled and Swofty-core-only runs if the fault persists: the addon never writes throttle or injects controls, and no exact replay/flashback query is available in the pinned public API. This is an input-capture mitigation, not a confirmed game-level fix.

Lap rows use separate timing and quality lines. X (cyan) and Y (amber) use separate lanes by default; “View: overlay” puts them on shared pedal axes. Connected upper-envelope lines plus min/max markers replace isolated dots. A connection requires a recorded predecessor link, including sparse bins at racing speed; gaps remain disconnected and no smoothing is applied. Zoom to inspect detail. The bin pool remains 24.2 MiB; four 192-column caches and at most 1,536 trace-line calls bound open-menu work independently of lap count.

## Extended telemetry in 0.3.0

See [EXTENDED_TELEMETRY.md](EXTENDED_TELEMETRY.md) for the implemented sector markers, five F8 pages, per-wheel selector, front-wheel steering readout, verified EA offsets and unit limitations. The new total fixed pool is 52.4 MiB, superseding earlier pedal-only 24.2 MiB totals. Pedals retain 2 m bins; extra channels use 16 m extrema bins. Rendering is four 176-column arrays for the selected page only. Existing garage-only/unpaused focus safeguards and socket bounds remain in force. Game validation remains pending.

## Paused review and rewind rollback (current)

This supersedes earlier garage-only and whole-session rewind-reset policies. F8 is allowed in an unpaused fresh garage or a pause entered with fresh lap metadata; that context is latched because paused UDP can stop. Resume on track releases focus. Held controls and non-review navigation still release/refuse focus. Pause masks do not identify replay/flashback reliably: close F8 before flashback controls; the previous stuck-input report remains an in-game validation risk.

A newer overall frame with backwards source time or lap number starts rollback. Recording waits for validated lap metadata, drops pending pairs and rejects older frames. Completed laps before the target survive; the target lap reopens with its retained prefix, and later laps are removed. Discard the entire boundary bin and suffix (2 m pedals, 16 m extended channels), not just extrema, to avoid mixing attempts. Missing target history starts partial. Rewritten laps are conservatively partial and retain conservative quality flags. Selections/cache generation resets. No allocation: rollback scans at most 1,024 lap records plus one pedal slot and 14 extended channel suffixes; normal recording cost and fixed pool size are unchanged.


## Multi-driver review (current)

[Multi-driver review](MULTI_DRIVER_REVIEW.md) supersedes the player-only recording and trace-count descriptions above. X/Y select independent drivers and completed laps. One 128-slot detailed pool is shared across all cars (including active laps), with 1,024 timing entries per car and a measured 53.21 MiB total reservation. Local HUD and the PR #15 pause/focus gates still use only local-player state. Opponent slip/aero data and restricted ERS remain unavailable. See that document for controls, retention limits, packet offsets, rewind handling and validation evidence.

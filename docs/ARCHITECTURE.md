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
No file/network I/O, blocking waits or unbounded allocation in callbacks.

## History and plot mathematics

Use timestamped samples, not a fixed number of rendered frames.
Proposed storage budget: 8,192 samples, a maximum history of 15 seconds and an ingest ceiling of 240 samples/s.
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
No hand-written UDP decoder or external overlay is included in this documentation milestone.

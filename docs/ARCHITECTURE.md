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

Feed.clear clears display history while retaining source identity and overall-frame ordering. A changed session retires its predecessor; player changes are checked against session frame ordering first. Invalid pedals in a current telemetry packet advance ordering and mark unavailable.

ReceiveGate discards the socket queue after initial open, reset, pause/resume, render gaps over 500 ms and budget exhaustion. Discarding stays bounded at 128 datagrams per paint and continues until WSAEWOULDBLOCK. Later paints accept new arrivals. UDP has no authenticated send timestamp: this protects against local receive backlog, not arbitrary upstream delays or a malicious local sender.

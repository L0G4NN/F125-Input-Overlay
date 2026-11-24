# Overlay architecture

Swofty loads the Windows addon and supplies lifecycle events, settings, draw calls and overlay input. The addon reads EA format 2025 UDP on loopback port 20777. The render callback owns the socket, live trace, recorder and review panel. Settings and lifecycle notifications cross into it through atomics.

| Component | Responsibility |
| --- | --- |
| `src/swofty/plugin.cpp` | Initialize the addon, receive UDP, apply settings and draw the live HUD |
| `src/core/receive_gate.hpp` | Drain the receive queue after startup, reset, pause transitions, a render stall or an exhausted packet budget |
| `src/core/udp.hpp`, `trace.hpp` | Validate local pedals, reject duplicates, track source identity and maintain bounded time history |
| `src/core/multi_review.hpp`, `lap_review.hpp` | Manage the session, participant identity, track-sized storage, per-car lap lifecycle and final timing |
| `src/core/extended.hpp`, `bin_block.hpp` | Pair channels with same-frame distance and retain extrema and continuity in fixed storage |
| `src/core/review_plot.hpp`, `src/swofty/review_panel.hpp` | Aggregate the two selected laps by distance and handle review controls, paint and focus |

## Live history

The live trace holds up to 8,192 timestamped samples over a selectable 2–15 seconds. Session time determines horizontal position. Wall time detects silence. Invalid data, a rewind or an interrupted queue breaks the trace. The receive loop reads at most 128 datagrams per paint. After a queue interruption it waits for an empty socket boundary before accepting new driving samples. While paused it can still process race results.

## Review

The recorder partitions one pair of backing arrays among the field when a validated session supplies the track length. The [recorder guide](MULTI_DRIVER_REVIEW.md) covers capacity, lap ownership, timing, flashback and participant changes. The [channel reference](EXTENDED_TELEMETRY.md) lists units and packet offsets.

The F6 panel requires overlay input v3, an active game window and either a pause or fresh local garage metadata. It draws only the selected page. Input capture ends on close, context loss, reset or master disable. The [installation guide](INSTALL.md#review-and-compare-laps) describes the controls.

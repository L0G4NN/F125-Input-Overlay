# Requirements

These are acceptance requirements; implementation and validation status are recorded in TEST_PLAN.md.

## First release scope

| ID | Requirement |
| --- | --- |
| R01 | Plot throttle and brake independently on one 0–100% graph. Never subtract one from the other. |
| R02 | Show newest sample at the right; older samples move left according to elapsed time, independent of rendering frame rate. |
| R03 | Default history is 5 seconds; allow 2–15 seconds in 1-second steps. |
| R04 | Label both channels and show rounded live percentages; offer vertical bars. |
| R05 | Provide enable, position, size, opacity, history and bars settings through supported Swofty settings. |
| R06 | Follow the local player's car rather than assuming car index zero or following a spectator camera. |
| R07 | Do not plot missing, stale or invalid data as zero input. Break the trace and show an unavailable indication. |
| R08 | Pause freezes the trace; flashback, restart, session replacement and player identity changes clear history before resuming. |
| R09 | Preserve input shape: no smoothing by default, no interpolation across gaps and no invented samples. |
| R10 | Store only bounded in-memory history. No telemetry recording, network upload or game-state writes in the MVP. |

## Proposed defaults

| Setting | Default | Allowed behaviour |
| --- | --- | --- |
| Enabled | On | Profile setting |
| History | 5 seconds | 2–15 seconds |
| Panel | 420 × 160 logical pixels | Width 280–900; height 120–360 |
| Anchor | Bottom-left | Four corners |
| Offset | 24 logical pixels on each axis | Clamp within drawable safe area |
| Background opacity | 25% | 0–100%; does not fade the trace lines |
| Input bars | On | Independent of trace visibility |
| Channels | Green throttle, red brake | Labels remain visible; colour selection may follow later |

Logical dimensions use min(framebuffer width / 1920, framebuffer height / 1080), matching Swofty ui_scale. The entire panel must fit after scaling or resolution changes.
Position uses settings in the first release; mouse dragging and hotkeys are optional later work.

## Data and time contract

The internal model uses finite values from 0 to 1; rendering converts them to percentages.
Verify the upstream scale before implementing conversion. Values outside the verified range are invalid, apart from explicitly documented numeric tolerance.
A genuine zero must remain distinguishable from a missing field.

Freshness must be based on evidence that the source updated, not simply that a callback ran or values changed.
Constant full throttle is valid fresh data. Repeated cached telemetry is not.
The implementation spike must identify reliable freshness, pause and rewind signals; otherwise report the unsupported state rather than implying live accuracy.

## Acceptance

A synthetic 100% brake step followed by a two-second release must retain its timing at different render rates.
The graph must show simultaneous throttle and brake without hiding either.
A stale source must become visibly unavailable within 500 ms of the last confirmed update while actively driving.
On a verified pause, retain history and mark it paused rather than timing it out.
After any discontinuity, no line may join samples from opposite sides of the discontinuity.
Settings must survive a normal restart using the supported launcher mechanism.

## Deferred

Steering/clutch, best-lap comparison, CSV export, coaching, input filtering, raw pedal device access, lap-distance plots and alternative renderers.

## Current implementation

Native settings include 0–100% horizontal/vertical travel from the chosen corner. UDP parsing is implemented. Pause/session hooks, settings persistence and actual game performance still require runtime validation. Queue recovery deliberately waits for an empty socket boundary before accepting later packets.

## Planned lap review extension

The F8 full-session lap browser and X/Y distance comparison are specified in [LAP_REVIEW.md](LAP_REVIEW.md). This extends the original deferred lap recording/comparison scope. It is documentation only and does not change the current build or its settings.

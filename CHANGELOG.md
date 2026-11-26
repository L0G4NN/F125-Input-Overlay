# Changelog

## Unreleased

- Report UDP processing, live HUD drawing, F6 panel drawing and plot rebuild times in Swofty’s performance log when its profiling API is available.

- Populate recorder backing pages during addon load to reduce first-touch stalls during longer races. This increases initial working set.

- Protect race history from delayed Session packets after a reset and preserve local laps when the player changes telemetry sharing.
- Keep same-frame samples on a flashback target and prevent late pedals for a finished lap from clearing the next lap’s first sample.
- Require fresh garage evidence after a receive interruption. Avoid repeated recorder resets while a pause continues.
- Use distinguishable small-value axis labels and whole-gear ticks. Remove superseded review and build instructions.

## 0.5.1 — Lap retention and race results

- Ignore inactive car records before rewind detection. Preserve completed laps when a driver drops off the roster.
- Reuse trace slots from timing-only laps. Finalize known cars even if the participant roster expires, and fill missing lap times from late Lap Data or final Session History.
- Show isolated samples and bins that overlap the zoom boundary.
- Reject packages when the manifest version or DLL checksum disagrees with the native build metadata.

## 0.5.0 — Full-field race recording

- Retain up to 78 detailed completed laps and one active lap for each of 22 drivers. The session quota adjusts to track length and appears in review. Later laps retain timing and quality labels.
- Share the lap stamp across 32 bins and use compact gap links. The recorder fits within 192 MiB and preserves 2 m pedal extrema, 16 m extended extrema and player-only motion data.
- Ignore delayed car and roster packets after a same-session track change. Open review on the latest detailed lap when later laps have timing only.
- Use F6 by default and display the configured shortcut. Existing profiles keep their saved bindings.
- Correct the preview link, installation instructions and package version reporting. See [full-field replay results](docs/MULTI_DRIVER_REVIEW.md#050-full-field-replay).

## 0.4.x — Multi-driver lap review

- Restyle F8 with charcoal/slate surfaces, outlined controls, cyan/amber driver and lap accents, selected-tab underlines, lap-time summaries and labelled graph scales. Keep separate/shared views and all interaction behaviour.

- Replace X/Y driver cycling with paged dropdown lists; preserve independent selections and prevent clicks/scrolls reaching controls underneath.

- Remove paused F8 telemetry-freshness/latch and held-input/navigation blockers; retain explicit close, resume-on-track and focus/window-loss cleanup.

- Independent X/Y driver and completed-lap selection, retaining all five pages and distance controls.
- Player-only motion channels and restricted opponent ERS are unavailable rather than fabricated zero traces. Local HUD and PR #15 focus gates remain tied to the player.
- Participants decoder, 22-car/200-lap regressions, cross-driver panel tests, deterministic mutation test and allocation benchmark. Real-game validation remains pending.


## Earlier lap-review changes

- Restore F8 during pauses entered from a live session, keeping held-control checks.
- Roll back only abandoned lap data after rewinds, preserving earlier laps and prefix samples across all channels.

- Show a text-only Waiting for UDP status before telemetry arrives; do not draw an empty or stale graph.

- Remove the extra pixel below F8 extrema markers to avoid protruding trace edges without changing recorded values or geometry budgets.
- Add Bahrain F8 and live overlay previews to the README.

## 0.3.0 — Extended telemetry graphs

- Add dashed S2/S3 markers from validated session distances on every review page.
- Add speed/gear, instantaneous MGU-K power/cumulative energy, per-wheel slip ratio/angle and front/rear plank-height pages with shared X/Y selection and zoom.
- Add a fresh front-wheel steering angle in degrees to the live HUD; preserve unavailable states.
- Keep added channels in fixed 16 m extrema bins with independent exact-frame pairing. Total pool is 52.4 MiB for 128 detailed laps; only the selected page renders.
- Preserve raw slip-angle/plank-height values because their units are not explicit in the pinned field descriptions. No in-game validation claim.
- Add decoder, pairing, bounds, geometry, fuzz and allocation checks; retain garage-only focus safeguards.

## 0.2.2 — Review readability and flashback input mitigation

- Use compact two-line lap rows so quality labels cannot overlap selection buttons.
- Default to separate X/Y lanes per pedal, with an optional overlay view and distinct cyan/amber lap colours.
- Draw connected envelope traces using verified sample links, preserving extrema and real gaps. Keep the 24-byte bins and fixed 24.2 MiB pool; bound rendering to 192 columns per series.
- Restrict interactive review to fresh garage metadata while unpaused. Close on every pause, including potential replay/flashback, and refuse focus with other held controls. Also release focus when the drawable viewport disappears.
- This removes a risky capture path, but does not establish the reported stuck-throttle root cause. Actual game input and visual verification remain required.

## 0.2.1 — Setup naming visibility mitigation

- Hide the live HUD in garage, paused and stale/unknown contexts by default, keeping overlay drawing off game menus.
- Track lightweight visibility metadata even without the lap-review pool; add a native troubleshooting override.
- Release review focus when typing, non-review keyboard input or gamepad navigation starts.
- Add sanitizer regressions for visibility and input handoff. Issue #8 still needs in-game confirmation; no root-cause or visual-reproduction claim.

## 0.2.0 — Experimental session lap review

- Add startup-allocated, bounded memory-only lap recording with 2 m throttle/brake envelopes, 128 detailed traces and metadata-only overflow.
- Add validated lap/session/final-classification metadata and exact-frame pairing, gaps, invalid/partial labels and conservative flashback reset.
- Add garage/paused F8 menu, X/Y distance comparisons, zoom/pan, range cursor, key setting and focus cleanup.
- Keep closed-menu graph work and driving-state review input polling off; add lifecycle/focus/fuzz/benchmark checks and documented runtime limitations.
- Windows compilation and synthetic checks only; in-game operation and performance remain unverified.

## Packet validation hardening

- Reject invalid pedals before session/frame tracking changes; preserve gap indication only for newer packets from the established player.
- Decode each header and pedal pair once, with no new allocations or history storage.
- Ignore paused input before session tracking changes; add poisoning and recovery regressions.

## 0.1.4

- Preserve session identity and frame ordering across resets and player changes.
- Break traces on invalid current inputs and discard queued packets after interruptions.
- Add receiver recovery regression tests, stricter slider validation and `.swoftyaddon` packaging.
- Record Swofty reference conformance and remaining runtime validation.

## 0.1.3

- Added persistent native horizontal/vertical position settings alongside width, height and corner.
- Layout remains within screen bounds across resolutions.
- Added layout tests for corners, centres, limits and resolutions.

## 0.1.0 — Experimental import build

- Added Windows Swofty DLL, UDP 2025 receiver, green/red scrolling traces, bars and native settings.
- Added session/player resets, duplicate/reordered packet rejection, stale-data indication and pause/resume events.
- Added reproducible build/package scripts and install instructions.
- Compiled and inspected; real Swofty import and game testing remain pending.

## Initial groundwork

- Portable C++20 trace core with bounded history, validation and graph coordinate mapping.
- Deterministic synthetic core checks with address/undefined-behaviour sanitizers.

- Initial project scope, requirements and settings proposal.
- Architecture, implementation milestones and validation plan.
- Upstream source references and explicit compatibility unknowns.
- Contribution guidance and repository templates.

### Status

Trace core implemented; native integration and in-game validation pending.

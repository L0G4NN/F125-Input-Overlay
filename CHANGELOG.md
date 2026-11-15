# Changelog

## 0.2.0 — Experimental session lap review

- Add startup-allocated, bounded memory-only lap recording with 2 m throttle/brake envelopes, 128 detailed traces and metadata-only overflow.
- Add validated lap/session/final-classification metadata and exact-frame pairing, gaps, invalid/partial labels and conservative flashback reset.
- Add garage/paused F8 menu, X/Y distance comparisons, zoom/pan, range cursor, key setting and focus cleanup.
- Keep closed-menu graph work and driving-state review input polling off; add lifecycle/focus/fuzz/benchmark checks and documented runtime limitations.
- Windows compilation and synthetic checks only; in-game operation and performance remain unverified.

## Unreleased — packet validation hardening

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

## Unreleased

### Added

- Portable C++20 trace core with bounded history, validation and graph coordinate mapping.
- Deterministic synthetic core checks with address/undefined-behaviour sanitizers.

- Initial project scope, requirements and settings proposal.
- Architecture, implementation milestones and validation plan.
- Upstream source references and explicit compatibility unknowns.
- Contribution guidance and repository templates.

### Status

Trace core implemented; native integration and in-game validation pending.

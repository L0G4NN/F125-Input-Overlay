# Roadmap

## M0 — Repository groundwork

- [x] Define MVP and defaults.
- [x] Document architecture, data correctness and lifecycle requirements.
- [x] Document build strategy, test plan and upstream references.
- [x] Add contribution templates, editor settings and ignore rules.
- [x] Confirm private GitHub repository and publish this groundwork.

No addon code is part of M0.

## M1 — Compatibility and build spike

- [ ] Pin SDK revision, licences and toolchain.
- [ ] Build a minimal addon and verify its load/unload lifecycle.
- [ ] Confirm local-player inputs, units, source freshness and timing.
- [ ] Verify overlay drawing and the relevant game/launcher versions.
- [ ] Resolve pause/rewind detection and whether UDP is needed.

Exit: evidence supports the proposed data and rendering path.

## M2 — Trace core

- [x] Implement validated input samples, state transitions and bounded history.
- [x] Implement time-based graph geometry.
- [x] Add synthetic tests for edge cases and variable frame rates.

Exit: deterministic core behaviour without requiring F1 25.

## M3 — Usable overlay

- [ ] Integrate green/red traces, percentage labels and bars.
- [ ] Add settings and viewport-safe layout.
- [ ] Handle unavailable data and session discontinuities.
- [ ] Verify thread ownership and clean shutdown.

Exit: first end-to-end in-game demonstration.

## M4 — First tested release

- [ ] Run integration and manual checks.
- [ ] Measure overhead and fix any regression.
- [ ] Add verified build/install/troubleshooting instructions.
- [ ] Validate and package the addon with dependency notices.

Exit: a tagged release with exact supported versions and honest test evidence.

## Later ideas

User-selectable colours, steering trace, optional hotkeys, dragging, lap comparison and recording.
Keep them separate from the initial throttle/brake implementation.

The portable M2 core was implemented independently while M1 native source freshness and lifecycle questions remain open. M1 has not passed its exit gate.

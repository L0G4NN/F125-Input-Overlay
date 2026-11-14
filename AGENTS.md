# Agent instructions

## Scope

This is F1 25 Input Overlay, a private, documentation-first Swofty addon project.
Follow the user's current authorization. The initial request stops after repository groundwork; do not start addon implementation until requested.

## Before implementation

Read README.md and the requirements, architecture, development and test-plan documents.
Resolve M1 compatibility questions against a pinned upstream revision before assuming API behaviour.
Keep the core independent of game APIs. Use documented Swofty interfaces; do not invent signatures, offsets, units or freshness signals.

## Quality

Treat unavailable input differently from zero. Preserve real timing and input transitions.
Keep history bounded, callbacks nonblocking and cross-thread ownership explicit.
Add tests for changed behaviour, not tests that simply restate implementation.
Document build results separately from in-game validation.
Do not claim compatibility, low overhead or successful installation without evidence.

## Workflow

Use dev or a focused topic branch for implementation and PRs into main.
Keep README, compatibility records and changelog aligned with the actual state.
Avoid unnecessary dependencies and redundant GitHub Actions runs.
Do not add automatic publishing or upload captured telemetry.
Do not commit secrets, logs, saves, game assets or generated binaries.

# F1 25 Input Overlay

A planned Swofty Launcher addon for F1 25 that displays a scrolling throttle and brake trace, inspired by the input overlays used in sim racing.

**Status: initial trace core implemented and tested. Native addon integration is pending; no installable release exists yet.**

## Planned first version

- Green throttle and red brake traces on one 0–100% graph.
- Five seconds of history by default, with new input entering at the right.
- Live percentage labels and optional vertical input bars.
- Adjustable position, size, background opacity and history length.
- Clear handling of missing telemetry, pauses, flashbacks and session changes.
- Read-only display of game-reported inputs; no driving assistance or changes to car behaviour.

The purpose is to see braking application, trail-brake release and throttle pickup. These are game-reported values, not a direct measurement of pedal force or raw hardware input.

## Target and current limits

Initial target: Windows x64, F1 25, offline single-player through Swofty Launcher.
The preferred approach is a native C++ addon using Swofty's telemetry and overlay APIs.
Both APIs are documented as experimental. Exact launcher, game, SDK and toolchain versions must be pinned during the first implementation milestone.

No compatibility or performance claims have been validated in game. Linux/Proton, VR, split-screen, multiplayer and the 2026 Season Pack are outside the initial support claim.

Repository: [L0G4NN/F125-Input-Overlay](https://github.com/L0G4NN/F125-Input-Overlay) (private).

## Start here

| Document | Purpose |
| --- | --- |
| [Requirements](docs/REQUIREMENTS.md) | Display behaviour, settings and acceptance criteria |
| [Architecture](docs/ARCHITECTURE.md) | Components, timing, data ownership and failure handling |
| [Development](docs/DEVELOPMENT.md) | Build strategy and first implementation steps |
| [Test plan](docs/TEST_PLAN.md) | Unit, integration and in-game checks |
| [Roadmap](docs/ROADMAP.md) | Ordered milestones and completion gates |
| [Sources and compatibility](docs/SOURCES.md) | Upstream references and unresolved assumptions |
| [Contributing](CONTRIBUTING.md) | Branch and review workflow |
| [Agent instructions](AGENTS.md) | Guidance for future coding sessions |

## Repository contents

The repository includes a portable C++20 trace core in `src/core/trace.hpp`, deterministic checks in `tests/trace_test.cpp`, and `scripts/test.sh`. Run `bash scripts/test.sh` with GCC and Address/UndefinedBehavior sanitizers available.

The adapter must supply confirmed fresh timestamps and verified lifecycle signals. The core does not obtain game telemetry or detect pauses by itself. A native addon and its manifest will follow the compatibility spike.

There are no automated GitHub Actions workflows yet. Add focused checks when there is executable code to validate.

## Ownership

Private project maintained by L0G4NN. Original project material is reserved under [LICENSE](LICENSE).
No game assets or upstream SDK files are included. This project is not affiliated with EA, Codemasters, Formula 1, iRacing or Swofty.

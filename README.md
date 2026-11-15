# F1 25 Input Overlay

A Swofty Launcher addon for F1 25 that displays a scrolling throttle and brake trace, inspired by the input overlays used in sim racing.

**Status: experimental 0.3.0 under development. Native Windows overlay, bounded lap recorder and F8 review panel implemented; actual Swofty import and in-game validation remain pending.**

See [installation and telemetry settings](docs/INSTALL.md). The ZIP contains a compiled DLL; the GitHub source ZIP is not the install package.

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
The implementation uses Swofty's experimental overlay API and a loopback UDP receiver for EA's documented 2025 telemetry format. SDK revision and compiler are pinned in docs/SOURCES.md.

No compatibility or performance claims have been validated in game. Linux/Proton, VR, split-screen, multiplayer and the 2026 Season Pack are outside the initial support claim.

Repository: [L0G4NN/F125-Input-Overlay](https://github.com/L0G4NN/F125-Input-Overlay).

## Start here

| Document | Purpose |
| --- | --- |
| [Requirements](docs/REQUIREMENTS.md) | Display behaviour, settings and acceptance criteria |
| [Architecture](docs/ARCHITECTURE.md) | Components, timing, data ownership and failure handling |
| [Development](docs/DEVELOPMENT.md) | Build strategy and first implementation steps |
| [Test plan](docs/TEST_PLAN.md) | Unit, integration and in-game checks |
| [Lap review plan](docs/LAP_REVIEW.md) | F8 lap browser, X/Y comparison, limits and validation status |
| [Roadmap](docs/ROADMAP.md) | Ordered milestones and completion gates |
| [Sources and compatibility](docs/SOURCES.md) | Upstream references and unresolved assumptions |
| [Contributing](CONTRIBUTING.md) | Branch and review workflow |
| [Agent instructions](AGENTS.md) | Guidance for future coding sessions |

## Repository contents

The repository includes a portable C++20 trace core in `src/core/trace.hpp`, deterministic checks in `tests/trace_test.cpp`, and `scripts/test.sh`. Run `bash scripts/test.sh` with GCC and Address/UndefinedBehavior sanitizers available.

The native adapter receives fresh UDP packets, resolves the local player from the packet header, and detects discontinuities. Swofty pause/resume and session events are used when available.

Build: install `ziglang==0.14.1` or Zig 0.14.1, then run `ZIG=/path/to/zig bash scripts/build.sh`. Package with `python3 scripts/package.py` after the build.

There are no automated GitHub Actions workflows yet. Add focused checks when there is executable code to validate.

## Ownership

Project maintained by L0G4NN. Original project material is reserved under [LICENSE](LICENSE).
No game assets are included. The vendored SDK header retains its MIT licence. This project is not affiliated with EA, Codemasters, Formula 1, iRacing or Swofty.

Position and size are adjustable through ADDON SETTINGS: Width, Height, Corner, Horizontal position and Vertical position. See docs/INSTALL.md for the percentage-based placement controls.

See [reference conformance review](docs/CONFORMANCE.md) for checked API contracts and outstanding runtime checks.

F8 opens session lap review in the garage while the game is not paused on cores with overlay input v3. Select X/Y for distance-aligned pedal comparisons: separate lanes by default, optional overlay, cyan X and amber Y, zoom with the wheel and drag to pan. Up to 128 full traces are kept in a fixed pool; later laps retain timing only. See [lap review](docs/LAP_REVIEW.md) for the 24.2 MiB pool, conservative flashback reset and unverified game behaviour.

The live HUD is hidden in garage/paused/unknown contexts by default to avoid covering game-menu text. See [issue #8 validation](docs/SETUP_NAMING.md) for the mitigation, troubleshooting override and remaining in-game check.

Review closes on any pause (including replay/flashback) and refuses focus while keyboard/controller controls are held. The reported stuck-throttle behaviour still requires in-game verification; this addon does not write vehicle inputs.

F8 also includes Speed/Gear, ERS, per-wheel Slip and Aero Height pages, with dashed S2/S3 boundaries. The live HUD shows the reported front-wheel steering angle in degrees. Extended recording uses a fixed 52.4 MiB pool; see [extended telemetry](docs/EXTENDED_TELEMETRY.md) for units, 16 m resolution and validation limits.

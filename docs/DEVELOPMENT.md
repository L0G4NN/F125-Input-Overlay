# Development plan

## Current state

The portable trace core is implemented. Run `bash scripts/test.sh` using GCC with C++20 and sanitizer support.
In a ptrace-hosted environment where LeakSanitizer cannot run, use `ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh`; AddressSanitizer and UndefinedBehaviorSanitizer remain enabled.
Native code is in src/swofty/plugin.cpp; addon/swofty.json is the package manifest. Run scripts/build.sh with Zig 0.14.1 and scripts/package.py to create the import ZIP. See INSTALL.md.

## First coding milestone: compatibility spike

1. Inspect the current official Swofty addon template, headers and implementation.
2. Pin the exact upstream commit and record its licence and required notices.
3. Select and pin the compiler/build system after reproducing the official sample build.
4. Confirm the local game and launcher versions used for integration testing.
5. Prove local-player throttle and brake units, update cadence and freshness.
6. Prove pause, flashback, restart and session transition detection.
7. Draw one small test element through the overlay API, then unload it cleanly.
8. Record results and limitations in SOURCES.md before implementing the full graph.

The documented upstream sample uses C++ and a Zig cross-compile to a Windows DLL.
The native build uses Zig 0.14.1; it has been cross-compiled here.
Prefer the smallest reproducible toolchain supported by the pinned SDK.

## Planned layout after the spike

| Path | Intended contents |
| --- | --- |
| src/core/ | Input validation, history and plot geometry |
| src/swofty/ | Lifecycle, telemetry, overlay and settings adapters |
| tests/ | Deterministic tests and synthetic fixtures |
| addon/ | Validated manifest and package metadata |
| third_party/ | Only approved pinned dependencies with notices, if vendoring is needed |
| docs/ | Requirements and verified developer/user guidance |

Do not add a manifest that names a nonexistent DLL to the documentation-only repository.

## Build and CI policy

Start with local checks. Once code exists, add one focused workflow for core tests and an addon compile check where practical.
Avoid scheduled runs, redundant push-plus-PR jobs and broad build matrices.
Use read-only workflow permissions by default and cancel superseded runs.
A packaging job should be manual or release-driven, and must not publish an untested build automatically.

## Packaging later

A release package must include its manifest, the compiled DLL, applicable notices and installation instructions.
Validate the manifest and package with tools from the pinned launcher version.
Do not ship game binaries, game assets, local logs, saves or proprietary headers.
A version is not supported until the relevant in-game matrix has been completed.

## Evidence

Keep synthetic tests, compilation and real game validation explicitly separate.
Never report a successful build as proof that the addon loads or that pedal traces are correct.

## Implemented adapter decision

The pinned telemetry implementation exposes cached car values and a flow-driven clock, without per-input packet freshness. The addon therefore decodes EA 2025 UDP datagrams itself. Receipt and overall frame IDs establish freshness; header session timestamps provide plot time. Rendering and ingestion have one render-thread owner; settings and lifecycle notifications use atomic values. The socket is nonblocking with a 128-datagram per-frame budget. Socket and SDK callback lifetime follows the game process; changing enabled addons requires a game restart.

Current package outputs include both ZIP and `.swoftyaddon` archives. See CONFORMANCE.md for the reference review and queue recovery contract.

## Extended telemetry in 0.3.0

See [EXTENDED_TELEMETRY.md](EXTENDED_TELEMETRY.md) for the implemented sector markers, five F8 pages, per-wheel selector, front-wheel steering readout, verified EA offsets and unit limitations. The new total fixed pool is 52.4 MiB, superseding earlier pedal-only 24.2 MiB totals. Pedals retain 2 m bins; extra channels use 16 m extrema bins. Rendering is four 176-column arrays for the selected page only. Existing garage-only/unpaused focus safeguards and socket bounds remain in force. Game validation remains pending.

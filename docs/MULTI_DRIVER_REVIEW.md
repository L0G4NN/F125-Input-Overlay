# Multi-driver lap comparison (F6 default)

Version 0.5.1 supports independent X/Y driver and lap selections. In-game validation remains pending.

## User controls

The X and Y driver dropdowns independently select known participants. Each list shows eight drivers at a time; scroll or use Previous/Next to reach the rest. Escape or clicking outside dismisses the list without changing the selection. Each shows the car slot and a shortened driver name. Selecting a different driver clears that side's old lap selection; reselecting the same driver preserves it. The Laps button switches the list between the X driver's and Y driver's completed laps; Previous/Next and the list wheel paginate that driver's history. Press X or Y beside a lap to assign that exact driver/lap pair. Selection summaries show each driver's name and its own lap number.

Supported combinations:

- One driver's lap A against their lap B.
- Driver A's lap N against driver B's lap N.
- Driver A's lap N against driver B's lap M.

A lap number belongs to its driver; it does not refer to the leader's lap or a common wall-clock interval. Only recorded, completed laps are selectable. Missing samples remain gaps. Partial, invalid, in/out laps and timing-only laps retain their labels. Both plots use the same track-distance scale and view range. Pedals, Speed/Gear, ERS, Wheel Slip and Aero Height, sector markers, zoom, pan, cursor values and split/shared axes remain available. This feature does not add delta-time calculation, export, persistent recording or a native garage tab.

## Data availability

| Channel | Local player | Other recorded drivers |
| --- | --- | --- |
| Throttle/brake | Telemetry packet 6 | That car's packet 6 entry |
| Speed/gear | Telemetry packet 6 | That car's packet 6 entry |
| MGU-K power / lap energy | Status packet 7 | Only when participant telemetry is public; otherwise unavailable |
| Slip ratio/angle, aero heights | Motion Ex packet 13 | Unavailable: packet 13 is player-only |
| Live steering HUD | Local player's Motion Ex | Never switched by X/Y selection |

The header's local-player index remains distinct from the selected car-array index. The live pedal HUD, steering readout, garage visibility and pause/focus safety all follow the local player, even when an AI car is in its garage or selected for review. Multiplayer, split-screen, spectator mode and UDP format 2026 remain outside the supported scope.

Opponent recording starts only after a validated Participants packet establishes active slots. Participant packets older than ten seconds suspend opponent ingestion until refreshed; a gap is inserted. Existing completed traces remain readable, including after a driver disappears from the roster. Ordinary unchanged participant refreshes do not close F6 or reset selections. Identity changes and changes to an opponent’s telemetry permissions clear that car’s history, release its slots and invalidate selections. Packets preceding that identity change cannot populate the replacement occupant's history.

Names are copied from the bounded 32-byte field, terminated locally, stripped of control bytes and displayed with non-ASCII bytes replaced by `?` for the default font. Car slot numbers distinguish similar/truncated names. Missing names use a car-number fallback. No remote strings are used as format strings.

## Memory and race length

Each of the 22 driver slots can retain up to **78 completed detailed laps plus one active lap**. The validated Session packet supplies the track length. Each lap reserves one pedal bin per 2 m and one extended-channel bin per 16 m, rounding up to cover the finish. The local player reserves all 14 extended channels, and each opponent reserves four (speed, gear and two ERS channels). The float extrema and their gap links retain their existing precision.

The total recorder budget is **192 MiB**. On the 0.5.0 Linux x64 validation build, backing and recorder state total **200,160,168 bytes (190.89 MiB)**. Each block of 32 bins shares a lap stamp and a validity mask. A 16-bit link per bin holds the previous connected index and the discontinuity flag. Pedal and extended blocks occupy 584 and 328 bytes respectively. This preserves the original float extrema and gap semantics. The full 78-lap quota applies through 9,408 m, at least 70 laps fit through 10,496 m, and the maximum accepted 16,384 m layout retains 44 completed laps plus one active lap per driver. F6 displays the calculated quota. Laps with timing but no recorded samples release their trace slot. After a driver fills their quota, new active laps still have a trace slot. At completion those laps retain timing and quality metadata and appear as **NO TRACE**. Each driver can retain 1,024 timing entries.

Windows reserves and commits zero-backed virtual pages at addon load. The addon touches each backing page once at load so that recording new laps does not have to populate those pages for the first time on the render thread. This moves work and roughly 190 MiB of initial working set to startup. Windows may still trim pages under memory pressure. A track change repartitions the existing backing using track-sized regions, and session-wide stamps hide data from old regions without clearing the backing. Session reset, interrupted laps, participant changes and rewind return owned slots to their driver. Lap transitions use a fixed per-driver free-slot stack. There is no full-lap copy, bulk initialization or memory allocation in the paint callback. The 128-datagram receive budget remains in place, and F6 builds plots only for the selected laps and page.

## Rewind and F6 safeguards

Review opens in a fresh local-player garage or during a pause with the game window active. Held controls do not block opening. Escape dismisses an open driver dropdown first. The configured shortcut or Close releases focus. Resuming on track, window or focus loss, reset and disabling the addon close review. Close review before setup typing or flashback. Actual game input behaviour still needs validation.

A validated rewind in any recorded car invalidates future history across the field. Each car waits for its own valid lap metadata to locate its target lap and distance. Its history is hidden while that location is unknown. Earlier laps and the target prefix survive; later laps and the boundary/suffix bins are discarded. The boundary bin is conservatively dropped at 2 m for pedals and 16 m for extended channels. Reopened laps are marked partial. Freed slots may be reused without mixing old and new samples. Selections/cache generation resets and closes F6 on a rewind.

## Verified packet addition

[EA F1 25 UDP v3 specification](https://forums.ea.com/t5/s/tghpe58374/attachments/tghpe58374/f1-games-game-info-hub-en/61/4/Data%20Output%20from%20F1%2025%20v3.pdf), Participants packet 4, version 1, **1,284 bytes**: active count at packet +29, entries from +30 with 57-byte stride. Within each entry: AI +0, driver ID +1, network ID +2, team +3, race number +5, name[32] +7, telemetry visibility +39, online-name visibility +40, livery-colour count +44. All offsets use byte reads; no unaligned structure casts. Existing Session, Lap, Telemetry, Status, Motion Ex and Final Classification decoders retain their pinned sizes/version checks and now accept a separately validated target car index.

## Validation

`ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh` runs the existing regression suite plus multi-driver tests. Leak detection is disabled only for this host's tracing restrictions. ASan/UBSan remain enabled. Tests cover cross-driver values/timings, independent lap transitions, per-car frame pairing, unsupported Motion Ex, ERS restrictions, participant truncation and labels, malformed high-frame isolation, nonzero player safety, unchanged roster refresh, global rewinds, missing target metadata, 22 cars over 200 laps (4,378 completed metadata entries, 1,716 retained traces), an oversized track with equal reduced quotas, and reuse across track and player changes.

Additional repeatable commands:

```bash
g++ -std=c++20 -O1 -g -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -Isrc tests/multi_fuzz.cpp -o build/multi_fuzz
ASAN_OPTIONS=detect_leaks=0 ./build/multi_fuzz
g++ -std=c++20 -O2 -Isrc tests/multi_benchmark.cpp -o build/multi_benchmark
./build/multi_benchmark
ZIG=/path/to/zig-0.14.1 bash scripts/build.sh
```

The deterministic mutation test exercises 300,000 packets; it is not coverage-guided libFuzzer. Benchmarks measure synthetic CPU ingestion and plot generation, not game frame time or GPU cost. Actual AI data availability, names, final-lap timing, flashback controls, launcher import and CPU/GPU impact require the real game.

### 0.5.0 full-field replay

The dense replay fills every pedal and extended bin through synthetic UDP packets, verifies each retained lap for every car and checks the player-only aero channel. Each run used a fresh Linux process. Peak RSS includes the executable, fixtures and touched recorder pages. Windows commits recorder backing at startup and its in-game working set remains unmeasured.

| Track length | Previous peak RSS, 70 laps | 0.5.0 peak RSS, 70 laps | 0.5.0 peak RSS, 78 laps |
| ---: | ---: | ---: | ---: |
| 5,000 m | 123.4 MiB | 91.8 MiB | 101.9 MiB |
| 7,004 m | 171.8 MiB | 126.9 MiB | 141.1 MiB |
| 8,000 m | 195.9 MiB | 144.0 MiB | 160.3 MiB |

At 70 and 78 laps the replay verified 1,540 and 1,716 detailed traces respectively. The allocation benchmark recorded zero C++ allocations during packet ingestion and selected-plot rebuilds at 0, 78, 128 and 200 completed laps per car. To reproduce on Linux, compile once and run each layout and lap count in a separate process.

```sh
g++ -std=c++20 -O2 -Isrc tests/race_replay.cpp -o build/race_replay
/usr/bin/time -v build/race_replay 7004 78
```

### Previous track-sized recorder benchmark (PR #20)

The same GCC `-O2` synthetic fixture fed 60,000 four-packet iterations with 22 cars at 0, 78, 128 and 200 completed laps per car. Across two runs, recording took 3.0–3.5 µs per iteration with zero C++ allocations. The prior recorder took 3.4–3.6 µs while all cars had active traces. Its later scenarios cost less in some runs because the shared pool ran out of detail slots. The new recorder continues collecting the active lap after the completed-lap quota is full. These measurements cover packet ingestion only. Windows game callback frame time and working set still need validation.

### Previous shared-pool benchmark (16 November 2026)

GCC `-O2` on this Linux host, 60,000 four-packet iterations with 22 active cars per scenario; fixed fixtures prepared before timing. The pair of selected traces in the plot microbenchmark is sparse (0/32 m samples on a 1,000 m track), so these plot figures are not a worst-case dense rendering measurement.

| Completed laps per car | Four-packet ingestion | X/Y plot rebuild | New C++ allocations |
| --- | --- | --- | --- |
| 0 | 3.06 us | 0.11 us (empty) | 0 |
| 78 | 2.58 us | 1.57 us | 0 |
| 128 | 2.55 us | 1.62 us | 0 |
| 200 | 1.81 us | 1.18 us | 0 |

The later scenarios are timing-only for current laps after trace storage fills. The faster timings therefore do not imply increasing lap count improves performance. Dense single-player X/Y panel tests still cover all pages, sector lines and cursor readouts under the 16,384-vertex budget; multi-driver UI tests cover independent selectors and unavailable opponent channels. Windows AMD64 build and local package validation pass. No release has been published as part of this change.

### First-touch check

A Linux dense replay compared demand-zero backing with one write per OS page before recording. With 22 cars on a 7,004 m track, laps 1, 40 and 78 incurred 608, 545 and 540 minor faults respectively during recording. The pre-touched run recorded zero on those three laps. Both runs verified 1,716 detailed traces. These process-level counts isolate initial page population. In-game frame times and Windows working-set behaviour still require measurement.

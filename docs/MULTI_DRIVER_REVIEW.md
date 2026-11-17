# Multi-driver F8 lap comparison

Implemented on top of PR #15 and main `d7c6c56`. Synthetic validation and Windows compilation are separate from actual F1 25/Swofty validation, which remains pending.

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

Opponent recording starts only after a validated Participants packet establishes active slots. Participant packets older than ten seconds suspend opponent ingestion until refreshed; a gap is inserted. Existing completed traces remain readable. Ordinary unchanged participant refreshes do not close F8 or reset selections. Identity or telemetry-permission changes clear the affected car's history, release its slots and invalidate selections. Packets preceding that identity change cannot populate the replacement occupant's history.

Names are copied from the bounded 32-byte field, terminated locally, stripped of control bytes and displayed with non-ASCII bytes replaced by `?` for the default font. Car slot numbers distinguish similar/truncated names. Missing names use a car-number fallback. No remote strings are used as format strings.

## Memory and race length

There is **one shared 128-slot detailed-trace budget**, including active laps, across all recorded cars. It is not 128 laps per driver. Slots are allocated first-come at lap start, with the local car processed first on a common packet. With a full 22-car field this retains roughly the first five to six detailed laps per car; subsequent laps are explicitly **NO TRACE** with timing/quality metadata only. This is an intentional bounded first implementation, not a full-race detailed archive for every driver. No silent eviction or resampling is performed.

Both trace arrays remain the existing 129-element allocation (the extra legacy slot is unused). Combined pool storage is 54,952,068 bytes including the slot allocator. Each of the 22 drivers has a fixed 1,024-entry metadata list and separate bounded packet-pairing/order state. Total pools plus all recorder state: **55,790,756 bytes (53.21 MiB)** on the validation host, guarded below 64 MiB at compile time. Global monotonic stamps prevent a reused slot exposing another car's old samples. Session reset, discarded unfinished laps, identity changes and rewinds return owned slots to the shared allocator. No whole-trace copies occur at completion.

Normal ingestion visits at most 22 cars, regardless of lap count, and never allocates after initialization. Slot search is bounded to 128 entries and only occurs at lap start. One nonblocking socket and the 128-datagram-per-paint receive budget are unchanged. Only the two selected laps and selected page build/draw while F8 is open. No threads, locks or disk writes are added.

## Rewind and F8 safeguards

PR #15's opening conditions are preserved: an unpaused fresh garage, or a pause entered with fresh local-player lap metadata and a latched pre-pause context. Resume on track closes review. Held keyboard/controller controls prevent acquisition; typing, other navigation, focus loss and window deactivation release it. Opponent garage states never grant focus. Pauses still cannot reliably distinguish the flashback/replay screen; close F8 before operating flashback. Actual stuck-control behaviour remains an in-game test requirement.

A validated rewind in any recorded car invalidates future history across the field. Each car waits for its own valid lap metadata to locate its target lap and distance. Its history is hidden while that location is unknown. Earlier laps and the target prefix survive; later laps and the boundary/suffix bins are discarded. The boundary bin is conservatively dropped at 2 m for pedals and 16 m for extended channels. Reopened laps are marked partial. Freed slots may be reused without mixing old and new samples. Selections/cache generation resets and closes F8 on a rewind.

## Verified packet addition

[EA F1 25 UDP v3 specification](https://forums.ea.com/t5/s/tghpe58374/attachments/tghpe58374/f1-games-game-info-hub-en/61/4/Data%20Output%20from%20F1%2025%20v3.pdf), Participants packet 4, version 1, **1,284 bytes**: active count at packet +29, entries from +30 with 57-byte stride. Within each entry: AI +0, driver ID +1, network ID +2, team +3, race number +5, name[32] +7, telemetry visibility +39, online-name visibility +40, livery-colour count +44. All offsets use byte reads; no unaligned structure casts. Existing Session, Lap, Telemetry, Status, Motion Ex and Final Classification decoders retain their pinned sizes/version checks and now accept a separately validated target car index.

## Validation

`ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh` runs the existing regression suite plus multi-driver tests. Leak detection is disabled only for this host's tracing restrictions. ASan/UBSan remain enabled. Tests cover cross-driver values/timings, independent lap transitions, per-car frame pairing, unsupported Motion Ex, ERS restrictions, participant truncation and labels, malformed high-frame isolation, nonzero player safety, unchanged roster refresh, global rewinds, missing target metadata, and 22 cars over 200 laps (4,378 completed metadata entries, exactly 128 retained traces).

Additional repeatable commands:

```bash
g++ -std=c++20 -O1 -g -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -Isrc tests/multi_fuzz.cpp -o build/multi_fuzz
ASAN_OPTIONS=detect_leaks=0 ./build/multi_fuzz
g++ -std=c++20 -O2 -Isrc tests/multi_benchmark.cpp -o build/multi_benchmark
./build/multi_benchmark
ZIG=/path/to/zig-0.14.1 bash scripts/build.sh
```

The deterministic mutation test exercises 300,000 packets; it is not coverage-guided libFuzzer. Benchmarks measure synthetic CPU ingestion and plot generation, not game frame time or GPU cost. Actual AI data availability, names, final-lap timing, flashback controls, launcher import and CPU/GPU impact require the real game.

### Synthetic performance result (16 November 2026)

GCC `-O2` on this Linux host, 60,000 four-packet iterations with 22 active cars per scenario; fixed fixtures prepared before timing. The pair of selected traces in the plot microbenchmark is sparse (0/32 m samples on a 1,000 m track), so these plot figures are not a worst-case dense rendering measurement.

| Completed laps per car | Four-packet ingestion | X/Y plot rebuild | New C++ allocations |
| --- | --- | --- | --- |
| 0 | 3.06 us | 0.11 us (empty) | 0 |
| 78 | 2.58 us | 1.57 us | 0 |
| 128 | 2.55 us | 1.62 us | 0 |
| 200 | 1.81 us | 1.18 us | 0 |

The later scenarios are timing-only for current laps after trace storage fills. The faster timings therefore do not imply increasing lap count improves performance. Dense single-player X/Y panel tests still cover all pages, sector lines and cursor readouts under the 16,384-vertex budget; multi-driver UI tests cover independent selectors and unavailable opponent channels. Windows AMD64 build and local package validation pass. No release has been published as part of this change.


## F8 pause access update

F8 now opens whenever Swofty reports a pause, without a fresh-UDP or pre-pause latch requirement. Held keys, controller buttons, triggers, stick drift and other navigation no longer block opening or automatically close review. F8, Escape and Close still close it; resuming on track, window/focus loss, reset and disable still release focus. Unpaused access remains garage-only. Close review manually before setup typing or flashback controls. This supersedes earlier freshness/held-control restrictions above; in-game behaviour remains to be verified.

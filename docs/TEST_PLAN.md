# Test plan

Run `bash scripts/test.sh` for the portable decoder, trace, layout, receive gate, recorder and panel checks. The panel test drives F6, both driver selectors and all five review pages with synthetic input. The [full-field validation guide](MULTI_DRIVER_REVIEW.md#validation) includes the long race replay, mutation test and allocation benchmark. [Development](DEVELOPMENT.md) has the Windows build and package commands.

## Manual game matrix

Run on the exact versions recorded in SOURCES.md:

- Grand Prix practice and an offline race; assess Time Trial only within the launcher's supported mode.
- Controller and wheel/pedals where available.
- Zero, half and full input; simultaneous pedals; constant input for several seconds.
- Pause/menu, flashback, restart, return to menu and load another session.
- Telemetry source loss and recovery, including whether UDP must be enabled.
- 1080p and 1440p; resize and DPI scaling; HDR/frame generation if claiming support.
- Settings changes and persistence after restart.
- Clean launch with only this addon, then coexistence with another HUD addon.
- Disable/re-enable and normal game exit without leaked callbacks or crashes.

## Performance target

Measure baseline versus enabled overlay under matching conditions.
Initial targets, not guarantees: bounded memory with no growth during a 30-minute session, no blocking work in render callbacks, and no material frame-time regression.
Record CPU/GPU, resolution, frame generation, sample rate, settings and measurement method.
Set a numerical release budget once a representative baseline exists.

## Late-race stutter capture

Use one offline 22-car race with recording enabled. At laps 5 and 65, capture one minute driving with F6 closed, then one minute paused with F6 open. Use the same camera, HUD history, comparison page, zoom and number of selected laps at each checkpoint. During each paused sample, first leave the graph still, then change the selection or zoom several times. Note the wall-clock time and lap at each boundary.

On a core with the profiler exports, the addon registers these CPU timings under `l0g4nn.input-overlay` in the `== Hooks` section of `swofty/perf.log`:

| Probe | Work measured |
| --- | --- |
| UDP receive and record | One socket drain, including decoding and recorder updates, even when the queue is empty |
| Live HUD draw | Preparing and submitting the visible on-track graph and its labels |
| F6 panel draw | Painting the open panel, including any plot rebuild within that paint |
| F6 plot rebuild | Rebuilding the selected traces after a selection, page or view change |

The core refreshes the report every 15 seconds. Save a copy at each window's start and end after a refresh. For each probe, divide the change in total milliseconds by the change in calls to get milliseconds per call for that window. A zero call difference means the path did not run. Compare the early and late windows in the same state. The rebuild is included in panel draw time. Check the matching pacing and hitch entries for frame spikes, GPU load and other contributors. Probe times cover CPU work issuing draw commands. The game's GPU work appears in its separate pacing data.

[Upstream profiler documentation](https://f1.swofty.net/reference/std/logging.html#timing-your-own-work)

## Release gate

Do not release until core tests pass, the pinned native build succeeds, package validation passes and a real in-game smoke test verifies both traces.
Record missing matrix coverage as unsupported or untested.


Also verify F6 rebinding, late race results, all five pages with a player/AI comparison, and flashback with a held pedal. Use the separate [setup naming matrix](SETUP_NAMING.md#manual-acceptance-matrix) for the reported invisible text.

# Extended telemetry review (0.3.0)

F8 retains the unpaused, fresh-garage-only input gate from 0.2.2. Choose X/Y laps and a page; the selection, zoom and distance range stay shared. Only the visible page builds/draws graphs. The default view has separate X/Y lanes; the View button switches to shared axes.

| Page | Upper graph | Lower graph |
| --- | --- | --- |
| Pedals | Throttle (%) | Brake (%) |
| Speed / Gear | Speed (km/h) | Gear: reverse -1, neutral 0, forward 1–8 |
| ERS | Instantaneous MGU-K power (kW) | Cumulative energy deployed in this lap (MJ) |
| Wheel slip | Signed slip ratio | Slip angle, raw reported value |
| Aero height | Front plank-edge height, raw reported value | Rear plank-edge height, raw reported value |

The Wheel button cycles rear left, rear right, front left, front right; each wheel's two channels are recorded independently. These are the requested wheel-slip metrics, not an automatic lock-up/understeer diagnosis. Aero height is plank-edge height, not the suspension setup slider.

The official Motion Ex field comments explicitly specify radians for the front wheels' steering angle. The live HUD converts this to signed degrees, labels it **Front-wheel angle**, and shows `--` after 500 ms without a valid update or after a discontinuity. It is not physical steering-wheel rotation. It works independently of lap recording and does not require the review pool.

The pinned EA field comments do not explicitly state units for wheel slip angle or front/rear aero height. Those values are deliberately retained and labelled raw; no unverified degrees/millimetres conversion is applied. Real-game validation is needed before assigning display units. Negative values are retained rather than clipped away.

## Sectors

Dashed vertical lines show the starts of S2 and S3 from the Session packet's distances in metres. They align across both graphs and X/Y lanes and follow zoom/pan. Missing, nonfinite, reversed or out-of-track boundaries suppress markers; sectors are never guessed by splitting track length into thirds. Start/finish is the lap plot's 0/track-length extent.

## Recording and bounds

The original pedal pool remains 2 m min/max bins. Fourteen added channels use 16 m min/max bins: speed, gear, MGU-K power, deployed energy, four wheel slip ratios, four wheel slip angles and two plank heights. This retains short extrema within each bin, but cannot retain their exact order/location within 16 m. Zoom cannot recover that discarded resolution. Gear connections are stepped; other connections follow the upper envelopes, without smoothing.

Both pools are allocated once at startup. Together they reserve **54,951,936 bytes (52.4 MiB)**; recorder metadata/pending packets add about 38 KB on the test host. A compile-time guard keeps these below 64 MiB. There are 128 retained completed traces plus the existing active-slot allowance; later laps retain timing only. Stamps avoid per-lap clearing/copies, and neither memory nor ingestion loops grow with completed lap count. Recording off retains the pools until process exit.

Status/Motion Ex/Telemetry values pair independently with validated Lap Data using exact overall frame IDs, with at most 100 ms source-time and receipt-time separation. An eight-slot fixed pending ring tolerates packet arrival order without borrowing a previous frame's distance. Missing packets are unavailable, never filled with cached values or zero. Completed laps are sealed against late writes so cached plots stay consistent. Channel predecessor links break after gaps; a discontinuity within one bin conservatively hides that bin. Session, identity, disable and flashback resets clear pairing/live steering and invalidate traces using the existing generation policy.

Each visible page aggregates two channels for X/Y into four fixed 176-column arrays. At most 1,408 trace line calls (8,448 vertices) leave room for controls, labels, cursor and sector dashes within the core's 16,384-vertex budget. No graphs build/draw while review is closed. The nonblocking socket and 128-datagram-per-paint limit are unchanged; no second socket, worker thread, telemetry upload or game input writes are added.

## Verified format and offsets

EA F1 25 UDP specification v3, format 2025, packet version 1, 29-byte header:

- Session 1, 753 bytes: sector 2/3 distance at packet offsets 745/749.
- Telemetry 6, 1352 bytes: 60-byte per-car stride; speed at car +0 (uint16 km/h), gear at +15 (int8).
- Status 7, 1239 bytes: 55-byte stride; MGU-K power at car +33 (W), deployed energy at +50 (J).
- Motion Ex 13, 273 bytes, player only: four slip ratios at packet +93, four slip angles at +109, front wheels angle at +197 (rad), front/rear plank height at +217/+221. Wheel arrays use RL/RR/FL/FR order.

Exact lengths, format/version/player/splitscreen, finite values, defensive magnitude limits and per-stream order are checked before indexing/storing. Defensive bounds are malformed-input limits, not claimed physical ranges. Unsupported packets are ignored. This implementation does not add 2026 UDP format support.

## Validation

`bash scripts/test.sh` includes extended decoding, truncation, signed gear, nonzero-player stride, independent packet order, exact-frame pairing, stale steering, missing status, invalid sector, poisoning and storage checks. Panel tests populate every channel and exercise all pages with X/Y, sector lines and cursor, checking the geometry budget. The deterministic fuzzer includes Status and Motion Ex and all extra plot channel pairs; the benchmark feeds all four data packet types at 0/78/128/200 completed laps.

These are synthetic checks. Windows cross-compilation is not evidence that fields are available in every session or that game input/performance is unaffected. On a gaming PC check S2/S3 against track timing lines, speed/gear against the HUD, ERS power versus cumulative deployment, each selected wheel, raw height/angle values, signed steering centre/full lock, UDP loss, flashback, full races and disabled/enabled frame-time comparisons.

## Paused review and rewind rollback (current)

This supersedes earlier garage-only and whole-session rewind-reset policies. F8 is allowed in an unpaused fresh garage or a pause entered with fresh lap metadata; that context is latched because paused UDP can stop. Resume on track releases focus. Held controls and non-review navigation still release/refuse focus. Pause masks do not identify replay/flashback reliably: close F8 before flashback controls; the previous stuck-input report remains an in-game validation risk.

A newer overall frame with backwards source time or lap number starts rollback. Recording waits for validated lap metadata, drops pending pairs and rejects older frames. Completed laps before the target survive; the target lap reopens with its retained prefix, and later laps are removed. Discard the entire boundary bin and suffix (2 m pedals, 16 m extended channels), not just extrema, to avoid mixing attempts. Missing target history starts partial. Rewritten laps are conservatively partial and retain conservative quality flags. Selections/cache generation resets. No allocation: rollback scans at most 1,024 lap records plus one pedal slot and 14 extended channel suffixes; normal recording cost and fixed pool size are unchanged.


## Multi-driver review (current)

[Multi-driver review](MULTI_DRIVER_REVIEW.md) supersedes the player-only recording and trace-count descriptions above. X/Y select independent drivers and completed laps. One 128-slot detailed pool is shared across all cars (including active laps), with 1,024 timing entries per car and a measured 53.21 MiB total reservation. Local HUD and the PR #15 pause/focus gates still use only local-player state. Opponent slip/aero data and restricted ERS remain unavailable. See that document for controls, retention limits, packet offsets, rewind handling and validation evidence.

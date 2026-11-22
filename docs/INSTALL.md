# Installation and user guide

F1 25 Input Overlay is an experimental Windows x64 addon for offline single-player F1 25. The current source version is 0.5.0. Actual Swofty import and in-game behaviour remain unverified.

## Download and install

1. Close F1 25.
2. Use Swofty Launcher 1.3.7 or newer and permit its experimental overlay feature. Lap review additionally requires a Swofty core with overlay input v3.
3. Check [Releases](https://github.com/L0G4NN/F125-Input-Overlay/releases) for a compiled `.swoftyaddon`. For version 0.5.0, the package name is `F125-Input-Overlay-0.5.0.swoftyaddon`. Use the version supplied with your chosen release. GitHub's **Source code** archives do not contain the compiled addon. If no compiled package is available, see [Development](DEVELOPMENT.md) to build one.
4. Import the `.swoftyaddon` from Swofty's **Addons** page or drag it onto the launcher window. Leave it archived.
5. Enable **F1 25 Input Overlay** in your offline profile and launch the game through Swofty.

The addon ID is `l0g4nn.input-overlay`. Remove previous installations with a different ID before importing, then reapply settings.

## Set up telemetry

In F1 25's Telemetry Settings, set:

| Setting | Value |
| --- | --- |
| UDP Telemetry | On |
| UDP Broadcast Mode | Off |
| UDP IP Address | `127.0.0.1` |
| UDP Port | `20777` |
| UDP Format | `2025` |
| UDP Send Rate | 60 Hz (30 Hz is also accepted) |

Start an offline practice session and leave the garage. Green is throttle; red is brake. Both share a 0–100% graph, with new samples entering at the right. Values are game-reported inputs, not pedal force or raw hardware readings.

The live HUD is hidden in garage, paused and stale/unknown telemetry contexts by default to keep menus and setup naming visible. Recording continues under its own settings while the HUD is hidden.

Port 20777 is fixed. Close other telemetry listeners using it before starting the game. The addon does not forward packets. Format 2026 and split-screen packets are rejected.

## Settings

Open **ADDON SETTINGS → F1 25 Input Overlay**. Settings use your Swofty profile and are intended to persist across normal restarts; in-game persistence still needs validation.

| Setting | Purpose and default |
| --- | --- |
| Enable input overlay and lap review | Master toggle, on by default. Disabling stops receiving/recording; re-enabling starts a new session store. |
| Show input bars | Vertical pedal bars; on by default. |
| History (seconds) | 2–15 seconds; default 5. |
| Width / Height | Default 420 × 160; ranges 280–900 wide and 120–360 high. |
| Background opacity (%) | 0 transparent to 100 opaque; default 25. |
| Corner | 0 bottom-left, 1 bottom-right, 2 top-left, 3 top-right; default 0. |
| Horizontal / Vertical position | Move inward from the selected corner: 0% at that edge, 50% centred, 100% at the opposite edge. |
| Record session laps in memory | On by default. Turning it off preserves completed laps but discards the partial active lap. |
| Lap review key | F6–F12; default F6. Saved settings in older profiles may still use F8. Avoid bindings used by the game or other addons. |
| Hide live HUD in garage and menus | On by default. Turn off temporarily to see HUD status messages when diagnosing telemetry. |

The full live panel stays inside the screen. Mouse dragging to reposition it is not implemented.

## Review and compare laps

1. Keep the master toggle and **Record session laps in memory** on. Enable UDP before driving the laps you want to review.
2. Complete laps, then **pause mid-session**, or return to the garage unpaused.
3. Keep the game window active and press **F6** or your configured review key.
4. Open the **X v** and **Y v** dropdowns to select drivers independently. Switch **Laps: X driver / Y driver** to browse either driver, then assign **X** or **Y** beside a completed lap. X is cyan and Y is amber. Each pedal has separate X/Y lanes by default; **View** switches to shared-axis overlay comparisons.

| Control | Action |
| --- | --- |
| Mouse wheel over graphs | Zoom |
| Drag on graphs | Pan |
| Reset view | Fit the full lap |
| Clear comparison | Remove Y |
| Escape, Close or configured review key | Close review and release focus |

Graphs use distance in metres. Cursor values show min/max inputs within a 2 m bin. Gaps indicate missing or unmatched data and are not smoothed over.

Lap labels include **INVALID**, **PARTIAL**, **OUT LAP**, **IN LAP** and **NO TRACE**. Partial or incomplete laps may not support a full comparison. **NO TRACE** means timing metadata is retained without detailed pedal data. A final lap can have an unknown time if no authoritative timing transition arrived.

The live HUD hides while review is open. Review closes when resuming on track, Alt-Tab, session replacement, master disable or another addon taking focus. Close review before naming a setup or operating flashback controls so the game receives keyboard and mouse input.

Review focus does not block controllers or wheels, and DirectInput wheels may not appear in its controller snapshot. These restrictions mitigate input capture problems; the reported stuck-throttle behaviour still needs in-game verification. The addon does not write vehicle inputs. See [setup visibility notes](SETUP_NAMING.md) and [lap-review validation](LAP_REVIEW.md).

## Recording limits and data

- Laps stay in memory only. There is no disk telemetry recording, export or upload.
- Exiting the game or a session reset clears recordings. Rewinds clear X/Y selections, retain earlier laps and the affected lap prefix, and delete the abandoned suffix and later laps. The boundary bin is discarded; rewritten laps are marked partial.
- Each of up to 22 drivers retains up to 78 detailed completed laps plus an active lap. The track length determines the space per trace and can reduce the quota equally for all drivers. F6 shows the session quota. Later laps retain timing and show **NO TRACE**, up to 1,024 metadata entries per driver.
- The recorder reserves startup backing within a 192 MiB budget. The validated track length determines the storage used by each driver. See [multi-driver review](MULTI_DRIVER_REVIEW.md#memory-and-race-length) for examples.
- Unknown track IDs and tracks longer than 16,384 m are not recorded.

## Troubleshooting

| Problem | What to check |
| --- | --- |
| No panel while driving | Check the addon is enabled in the profile used to launch the game, the master toggle is on and the experimental overlay feature is permitted. Check all UDP settings above. Restart after changing enabled addons. |
| No panel in garage/menus | Expected with **Hide live HUD in garage and menus** on. Leave the garage for the live graph; use F6 for garage review. |
| Waiting for UDP 2025 | Check address, port, format and UDP Telemetry above. There is no native-input fallback. Temporarily turn off **Hide live HUD in garage and menus** to see status messages. |
| UDP port 20777 unavailable | Close other apps listening on that port and restart F1 25. The port cannot be changed in this version. |
| F6 does nothing | Pause the game or return to the garage with fresh UDP lap data. Keep the game window active. Check recording, the master toggle and configured key. Older cores without overlay input v3 cannot open review. |
| No completed laps | Enable recording and UDP before driving, then complete a lap. Session resets clear recordings; rewinds remove only the superseded section. |
| Frozen trace or gaps | Pause events freeze the trace. Missing data is unavailable, not zero. Queued packets are discarded after interruptions, which can briefly show waiting. |
| History disappears after flashback/restart | Session changes clear history; rewinds remove the affected suffix and later laps. |
| Review closes unexpectedly | Resuming on track, session changes and loss of window or addon focus close review. |

Inspect `swofty/loader.log` for entries containing `[input-overlay]`. For a [bug report](https://github.com/L0G4NN/F125-Input-Overlay/issues), include addon, launcher/core and game versions, telemetry settings, steps to reproduce and relevant log lines. Check logs for private information before sharing.

## Disable or remove

Exit F1 25, then disable the addon in your Swofty profile. Use launcher's addon management to remove the package if desired. No save changes are performed. Recorded laps are lost when the game exits.

## Validation status

Windows AMD64 compilation, addon exports, system DLL imports, package structure and synthetic packet/trace checks are documented. These checks do not establish successful installation or in-game compatibility.

Actual import/load, keyboard/mouse/controller/wheel behaviour, pause/flashback handling, setup naming, race finalization, settings persistence, coexistence and performance still need game testing. Linux/Proton, VR, HDR/frame generation, multiplayer, split-screen and the 2026 format/Season Pack are untested or outside the initial target. See [conformance](CONFORMANCE.md) and the [test plan](TEST_PLAN.md) for details.

# F1 25 Input Overlay

See how you apply throttle and brake while driving in F1 25, then compare recorded laps back in the garage. This addon runs through Swofty Launcher on Windows x64.

- Live green throttle and red brake traces, percentage labels and optional input bars.
- Five seconds of history by default, adjustable from 2 to 15 seconds.
- Adjustable panel size, position and background opacity.
- Session lap review with two-lap comparisons, distance-based graphs, zoom and pan.

The display uses game-reported inputs, not pedal force or raw hardware readings. It does not provide driving assistance or change car behaviour.

**Experimental: current source version 0.2.2. Actual Swofty import and in-game behaviour still need validation.** The initial target is offline single-player. Multiplayer, VR, Linux/Proton, split-screen and the 2026 Season Pack are outside the current support claim.

## Install and start driving

You need F1 25 on Windows x64 and Swofty Launcher 1.3.7 or newer, with its experimental overlay feature permitted. Lap review also requires a Swofty core with overlay input v3.

1. Check [Releases](https://github.com/L0G4NN/F125-Input-Overlay/releases) for a compiled `.swoftyaddon` package. GitHub's **Source code** ZIP is not an installable addon. If no compiled package is available, see the [developer build instructions](docs/DEVELOPMENT.md).
2. Close F1 25, then import the `.swoftyaddon` using Swofty's **Addons** page or drag it onto the launcher. Leave the file archived.
3. Enable **F1 25 Input Overlay** in your offline profile and launch the game through Swofty.
4. In F1 25's Telemetry Settings, use:

   | Setting | Value |
   | --- | --- |
   | UDP Telemetry | On |
   | UDP Broadcast Mode | Off |
   | UDP IP Address | `127.0.0.1` |
   | UDP Port | `20777` |
   | UDP Format | `2025` |
   | UDP Send Rate | 60 Hz (30 Hz is also accepted) |

5. Start an offline practice session and leave the garage. The live panel is hidden in the garage and menus by default. New input appears at the right of the graph.

Another telemetry app cannot listen on port 20777 at the same time. The addon uses a fixed port and does not forward packets.

## Adjust the display

Open **ADDON SETTINGS → F1 25 Input Overlay** in Swofty to change history, input bars, size, opacity and position.

Choose a corner, then move inward with **Horizontal position** and **Vertical position**: 0% stays at the selected corner, 50% centres that axis, and 100% reaches the opposite edge. The panel stays inside the screen. Mouse dragging is not available.

## Compare your laps

Leave **Record session laps in memory** on while driving. Return to the garage with the game unpaused, release held controls, and press **F8** while the game window is active.

Select **X** and **Y** beside completed laps. X is cyan and Y is amber, with separate comparison graphs for throttle and brake. Scroll over a graph to zoom, drag to pan, and use **Reset view** to fit the lap. Press **Escape**, **Close**, or your review key to return to the game. The review key can be changed to F6–F12.

Recorded laps stay in memory only. Exiting the game, a session reset or a flashback/rewind clears them. There is no save or export feature. See the [full user guide](docs/INSTALL.md#review-and-compare-laps) for controls and recording limits.

## Need help?

- **No panel while driving:** check the addon is enabled, the overlay feature is permitted, and UDP matches the table above. Missing or stale telemetry hides the panel by default.
- **Waiting for UDP:** check format **2025**, address `127.0.0.1` and port `20777`. Close other apps listening on that port and restart the game.
- **F8 does nothing:** return to the garage, unpause, release held controls and keep the game window active. Check recording and the master toggle are on. Older cores can support the live HUD without lap review.

See [installation, settings and troubleshooting](docs/INSTALL.md), or [report a problem](https://github.com/L0G4NN/F125-Input-Overlay/issues).

## More information

- [User guide](docs/INSTALL.md) · [Changelog](CHANGELOG.md)
- [Development and building](docs/DEVELOPMENT.md) · [Contributing](CONTRIBUTING.md)
- [Requirements](docs/REQUIREMENTS.md) · [Architecture](docs/ARCHITECTURE.md) · [Test plan](docs/TEST_PLAN.md)
- [Sources and compatibility](docs/SOURCES.md) · [Runtime validation notes](docs/CONFORMANCE.md)

Maintained by L0G4NN. Original project material is reserved under [LICENSE](LICENSE); the vendored Swofty SDK retains its MIT licence. No game assets are included. This project is not affiliated with EA, Codemasters, Formula 1, iRacing or Swofty.

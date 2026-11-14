# Install the first test build

This is an experimental Windows x64 build. It is packaged for Swofty import, but has not been loaded in a real F1 25 session here.

1. Close F1 25. Use a current Swofty Launcher (the overlay feature requires at least 1.3.7).
2. Drag `F125-Input-Overlay-0.1.0.zip` onto the launcher window. It is an **addon**, with `swofty.json` at the archive root; leave it zipped.
3. Enable **F1 25 Input Overlay** in your offline profile, then launch the game through Swofty.
4. In F1 25's Telemetry Settings, set:

| Setting | Value |
| --- | --- |
| UDP Telemetry | On |
| UDP Broadcast Mode | Off |
| UDP IP Address | 127.0.0.1 |
| UDP Port | 20777 |
| UDP Format | 2025 |
| UDP Send Rate | 60 Hz (30 Hz also works) |

5. Start an offline practice session. Green is throttle; red is brake. New samples enter at the right, with five seconds visible by default.
6. In ADDON SETTINGS, adjust history, width, height, background opacity, corner or input bars. Corner values: 0 bottom-left, 1 bottom-right, 2 top-left, 3 top-right.

## Troubleshooting

- **Waiting for UDP 2025**: check every telemetry setting above. Format 2026 and split-screen packets are intentionally rejected. Native inputs are not used as a fallback.
- **UDP port unavailable**: another telemetry app may own port 20777. Close that listener and restart F1 25. This version has a fixed port and does not forward packets to other apps.
- **Nothing appears**: check the addon is enabled, the overlay feature is permitted, and Show overlay is on. Inspect `swofty/loader.log` for `[input-overlay]`. Restart the game after changing enabled addons.
- **Paused**: the core's pause/resume events freeze the trace. If those events are unavailable on your game build, silence is shown as unavailable instead; it is not guessed to mean pause.
- **Flashback/restart**: history clears when a new session/player is seen or a newer overall frame carries a lower session timestamp. Older datagrams are discarded.

To remove it, exit the game and disable the addon in your profile. No save changes, telemetry recording or uploads are performed.

## Validation status

Verified here: Windows AMD64 DLL compilation, exported addon entry points, system DLL imports, manifest/package structure, synthetic packet and trace tests with AddressSanitizer/UndefinedBehaviorSanitizer.
Pending on a Windows gaming PC: actual Swofty import, load, live display, pause/flashback behaviour, coexistence and performance. Linux/Proton, VR, HDR/frame generation and the 2026 UDP format are untested or unsupported in this first build.

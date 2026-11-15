# Install the first test build

This is an experimental Windows x64 build. It is packaged for Swofty import, but has not been loaded in a real F1 25 session here.

1. Close F1 25. Use a current Swofty Launcher (the overlay feature requires at least 1.3.7).
2. Install the `.swoftyaddon` from the Addons page, or drag `F125-Input-Overlay-0.2.1.swoftyaddon` onto the launcher window. It is an **addon**, with `swofty.json` at the archive root; leave it archived.
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

To remove it, exit the game and disable the addon in your profile. No save changes, disk telemetry recording or uploads are performed. Session lap recording stays in memory.

## Validation status

Verified here: Windows AMD64 DLL compilation, exported addon entry points, system DLL imports, manifest/package structure, synthetic packet and trace tests with AddressSanitizer/UndefinedBehaviorSanitizer.
Pending on a Windows gaming PC: actual Swofty import, load, live display, pause/flashback behaviour, coexistence and performance. Linux/Proton, VR, HDR/frame generation and the 2026 UDP format are untested or unsupported in this first build.

## Position and size settings (0.1.3)

Open ADDON SETTINGS → F1 25 Input Overlay. Width and Height resize the panel. Corner picks the starting corner. Horizontal and Vertical position move inward from that corner: 0% stays at the corner, 50% centres that axis, 100% reaches the opposite edge. The full panel stays inside the screen. Values use native profile settings and survive a normal restart. Mouse dragging is not implemented.

The addon ID is `l0g4nn.input-overlay`. Remove previous installations with a different ID before importing, then reapply settings. After reset, resume, a render gap over 500 ms, or a full receive budget, queued packets are discarded until the socket is empty. The next receive cycle resumes with a broken trace segment. This can briefly show waiting.

## Experimental F8 lap review (0.2.0)

Keep the master overlay enabled and **Record session laps in memory** on. UDP format 2025 must be enabled before driving the laps you want to review. Return to the garage (fresh lap metadata required) with the game unpaused, then press **F8**. The game window must be foreground. On older cores without overlay input v3 the live trace works but review is unavailable; check the loader log.

Choose X and Y beside completed laps. X is cyan and Y is amber. Each pedal has separate X/Y lanes by default; View toggles a shared-axis overlay. Both graphs use distance in metres. Wheel over the graphs zooms, dragging pans, Reset view fits the lap, and Clear comparison removes Y. Cursor values show min/max ranges in a 2 m bin. Escape, Close or the configured function key releases focus without changing pause state. Native settings allow **F6–F12**, default F8. Avoid keys already bound by the game/other addons.

Review focus does **not** block controllers or wheels. Opening is deliberately restricted to fresh garage while unpaused; leaving that state, Alt-Tab, session replacement, master disable or another addon taking focus closes it. The live HUD is hidden while review is open. Closing review keeps recording; the master toggle stops the socket/recording and re-enabling starts a new session store. Turning only recording off preserves completed laps but discards the partial active lap.

The pool is allocated/zeroed once during addon startup, not each lap or frame: 25,362,432 bytes (24.19 MiB), plus small fixed state/caches. At most 128 detailed traces are kept; later completed laps are labelled NO TRACE and retain timing metadata. There is a hard 1,024-entry metadata guard (the UDP lap-number field itself is only 8-bit). No silent eviction or increasing memory use over long races. The pool remains allocated until game exit even if recording is off.

Unmatched frames, missing data, partial first laps and invalid laps are labelled or drawn with gaps. Flashback or lap-number rewind conservatively clears **all recorded laps** and X/Y selections; this first implementation does not reconstruct pre-flashback history. Final classification can confirm the final lap with time unknown if no authoritative lap-time transition arrived. No session-history time backfill is implemented. Unknown track IDs and tracks longer than 16,384 m are not recorded.

Real game checks remain pending: packet pairing/garage transitions on your exact game build, keyboard/mouse focus, setting persistence, race finalization and CPU/GPU performance. The local tests and synthetic profiles are not an in-game validation.

## Setup-name visibility (0.2.1)

**Hide live HUD in garage and menus** defaults to on. With it on, the small HUD draws only with fresh non-garage lap metadata and while not paused. No/stale UDP means the HUD stays hidden; diagnose UDP/port errors in the loader log. This also works on older cores without overlay input v3. Session recording continues under its existing settings.

The F8 review panel remains available in the garage while unpaused. Close it before naming a setup. Non-review keys, typed characters or gamepad navigation now close it and release focus; characters already captured during that frame are not replayed, so retype the first character if you began typing with review open. DirectInput wheels are not guaranteed to appear in the overlay's XInput snapshot.

Turning **Hide live HUD in garage and menus** off restores the previous always-visible HUD for troubleshooting; leave it on for the default mitigation. This is an addon-side mitigation for issue #8; actual setup-name rendering/entry still needs verification in F1 25. See SETUP_NAMING.md.

## 0.2.2 behaviour update

Interactive F8 review is now **garage-only and unpaused**, superseding earlier paused-review descriptions. Any pause closes it, even with fresh garage metadata; held keyboard/controller controls prevent focus acquisition. Test entering flashback with throttle held/released, both with review closed and after leaving the garage review, on keyboard, controller and wheel. Compare addon-disabled and Swofty-core-only runs if the fault persists: the addon never writes throttle or injects controls, and no exact replay/flashback query is available in the pinned public API. This is an input-capture mitigation, not a confirmed game-level fix.

Lap rows use separate timing and quality lines. X (cyan) and Y (amber) use separate lanes by default; “View: overlay” puts them on shared pedal axes. Connected upper-envelope lines plus min/max markers replace isolated dots. A connection requires a recorded predecessor link, including sparse bins at racing speed; gaps remain disconnected and no smoothing is applied. Zoom to inspect detail. The bin pool remains 24.2 MiB; four 192-column caches and at most 1,536 trace-line calls bound open-menu work independently of lap count.

## Extended telemetry in 0.3.0

See [EXTENDED_TELEMETRY.md](EXTENDED_TELEMETRY.md) for the implemented sector markers, five F8 pages, per-wheel selector, front-wheel steering readout, verified EA offsets and unit limitations. The new total fixed pool is 52.4 MiB, superseding earlier pedal-only 24.2 MiB totals. Pedals retain 2 m bins; extra channels use 16 m extrema bins. Rendering is four 176-column arrays for the selected page only. Existing garage-only/unpaused focus safeguards and socket bounds remain in force. Game validation remains pending.

# Setup naming: issue #8

Report: https://github.com/L0G4NN/F125-Input-Overlay/issues/8 — setup-name text is not visible while creating a new setup. The report has no game/core/addon versions, screenshot, log, or indication whether the final saved name is correct. It predates the interactive F8 implementation. The older HUD does not take keyboard focus or modify the game's setup/font data.

## Implemented mitigation (0.2.1)

- Hide live HUD drawing unless fresh lap metadata says the car is outside the garage and the game is not paused. Unknown/stale states also hide it. This removes our background/text primitives from the likely setup-naming context rather than changing game fonts or guessing offsets.
- Use existing validated session/lap data without requiring a 24 MiB recorder allocation. Unsupported review cores/allocation failure can still track visibility; recording guards prevent any unallocated-pool access.
- Add the native **Hide live HUD in garage and menus** toggle, default true. Turning it off temporarily restores previous behaviour for isolation. Normal UDP failure diagnostics remain in the loader log rather than drawing a waiting panel over a menu.
- While review owns focus, hand it back on text/non-review keys or XInput navigation. Closing review never replays WM_CHAR/key events into the game. The first input received during handoff may already have been captured by Swofty; close review before naming a setup or retype that character. Wheels using DirectInput may not be present in the XInput snapshot.
- Preserve existing explicit close, Escape, state loss, other-addon focus, master disable and session-reset cleanup. Closed review never calls focus-on.

The gate is constant-time per paint and reuses existing metadata; no extra sockets, allocations, worker threads, recording buffers or per-frame logs are added. The current public SDK has no verified exact setup-name-dialog signal; no unsupported private UI/font hooks were introduced.

## Evidence and limitation

Policy, freshness and unallocated-pool tests pass with ASan/UBSan. Fake-overlay tests verify focus handoff for text, Enter, controller buttons and axes. Windows AMD64 compilation and package checks pass. No affected Windows/F1 25 installation was available, so this is a mitigation awaiting visual confirmation, not an established root-cause fix. Do not automatically close issue #8 on merge.

## Manual acceptance matrix

Record exact game, Swofty/core and addon version. In the setup save/name dialog enter a distinctive non-personal test name. Check visibility while typing and the saved name afterwards; an accurate saved name with invisible editing text distinguishes rendering from missing character input.

| Run | Configuration | Check |
| --- | --- | --- |
| A | Stock game | Baseline typing, deletion, cancel and saved-name visibility |
| B | Swofty with this addon disabled | Whether the core/another addon alone reproduces it |
| C | Addon enabled, default hide-in-menus on, F8 never opened | No live HUD primitives over the garage dialog; name visible and saved correctly |
| D | Open F8, close using Close/Escape/hotkey, then name a setup | Input delivered normally; no lingering focus |
| E | Navigate the game with a controller or start typing while review is open | Review closes; no persistent capture; retype a captured first character if necessary |
| F | Temporarily turn hide-in-menus off | Whether live drawing is specifically associated with the symptom |

Repeat after leaving/re-entering the garage, pause/resume, Alt-Tab, recording on/off and another overlay taking focus. Verify the live HUD returns on track when fresh metadata resumes. Confirm UDP-off/port-conflict behaviour is diagnosed through logs without an obstructing HUD.

If B also reproduces the problem, investigate Swofty core or other addons; avoid claiming an addon-only fix. If C still reproduces while this addon issues no draw calls and owns no focus, collect the loader log and a screenshot, and isolate the core renderer/input path with a minimal addon. Do not patch font colours or renderer state without evidence.

# Setup naming: issue #8

Report: https://github.com/L0G4NN/F125-Input-Overlay/issues/8 — setup-name text is not visible while creating a new setup. The report has no game/core/addon versions, screenshot, log, or indication whether the final saved name is correct. It predates the interactive F8 implementation. The older HUD does not take keyboard focus or modify the game's setup/font data.

## Current behaviour

The default live HUD hides in a known garage. Use F6 to review laps there or while paused. Close review before typing into the game. Swofty routes keyboard and mouse input to the focused panel while controllers and wheels can still reach the game. The exact setup-name-dialog state is unavailable to the addon.

## Manual acceptance matrix

Record exact game, Swofty/core and addon version. In the setup save/name dialog enter a distinctive non-personal test name. Check visibility while typing and the saved name afterwards; an accurate saved name with invisible editing text distinguishes rendering from missing character input.

| Run | Configuration | Check |
| --- | --- | --- |
| A | Stock game | Baseline typing, deletion, cancel and saved-name visibility |
| B | Swofty with this addon disabled | Whether the core/another addon alone reproduces it |
| C | Addon enabled, default hide-in-menus on, F6 never opened | No live HUD primitives over the garage dialog; name visible and saved correctly |
| D | Open F6, close using Close/Escape/hotkey, then name a setup | Input delivered normally; no lingering focus |
| E | Navigate the game with a controller or start typing while review is open | Review stays open. Close it, then verify ordinary typing and control return |
| F | Temporarily turn hide-in-menus off | Whether live drawing is specifically associated with the symptom |

Repeat after leaving/re-entering the garage, pause/resume, Alt-Tab, recording on/off and another overlay taking focus. Verify the live HUD returns on track when fresh metadata resumes. Confirm UDP-off/port-conflict behaviour is diagnosed through logs without an obstructing HUD.

If B also reproduces the problem, investigate Swofty core or other addons; avoid claiming an addon-only fix. If C still reproduces while this addon issues no draw calls and owns no focus, collect the loader log and a screenshot, and isolate the core renderer/input path with a minimal addon. Do not patch font colours or renderer state without evidence.

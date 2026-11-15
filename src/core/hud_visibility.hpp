#pragma once
namespace input_trace {
// The noninteractive HUD must not cover the game's own garage/menu text.
// Unknown/stale telemetry is not evidence that it is safe to draw over menus.
inline bool show_live_hud(bool hide_in_menus,bool paused,bool fresh_on_track) {
    return !hide_in_menus || (!paused && fresh_on_track);
}
}

namespace input_trace {
// A pause is not proof of a safe menu: it can be replay/flashback. Never
// acquire review focus there, including when garage metadata is still fresh.
inline bool review_interactive(bool paused,bool fresh_garage) {
    return !paused && fresh_garage;
}
}

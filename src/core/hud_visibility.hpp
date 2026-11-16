#pragma once
namespace input_trace {
// A compact waiting label is allowed outside a known garage, but never
// during pause/flashback. It does not acquire focus or draw graph primitives.
inline bool show_waiting_hud(bool paused,bool fresh_garage) {
    return !paused && !fresh_garage;
}
// The noninteractive HUD must not cover the game's own garage/menu text.
// Unknown/stale telemetry is not evidence that it is safe to draw over menus.
inline bool show_live_hud(bool hide_in_menus,bool paused,bool fresh_on_track) {
    return !hide_in_menus || (!paused && fresh_on_track);
}
}

namespace input_trace {
// Pause review does not depend on UDP freshness: telemetry may stop in menus.
inline bool review_interactive(bool paused,bool fresh_garage) {
    return paused || fresh_garage;
}
}

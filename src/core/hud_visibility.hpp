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
// Latch fresh session context at pause entry: UDP may stop during the menu.
class PauseReviewGate {
    bool was_paused_{}, allowed_{};
public:
    void reset() {was_paused_=allowed_=false;}
    bool update(bool paused,bool fresh_session) {
        if(paused && !was_paused_) allowed_=fresh_session;
        if(!paused) allowed_=false;
        was_paused_=paused;return paused && allowed_;
    }
};
inline bool review_interactive(bool paused,bool fresh_garage,bool paused_session=false) {
    return paused?paused_session:fresh_garage;
}
}

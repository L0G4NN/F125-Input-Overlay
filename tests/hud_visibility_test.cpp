#include "core/hud_visibility.hpp"
#include "review_fixture.hpp"
#include <cassert>
#include <limits>
using namespace fixture;
using namespace input_trace;
int main() {
    assert(show_waiting_hud(false,false)); // no session/UDP yet
    assert(!show_waiting_hud(false,true)); // protect garage text entry
    assert(!show_waiting_hud(true,false)); // pause/flashback
    assert(!show_waiting_hud(true,true));

    // F8 works throughout pause, including absent/stale UDP and initial menu entry.
    assert(review_interactive(true,false));
    assert(review_interactive(true,true));
    assert(review_interactive(false,true));
    assert(!review_interactive(false,false)); // resume on track still closes review

    assert(!show_live_hud(true,false,false)); // no metadata: no menu obstruction
    assert(!show_live_hud(true,true,true));   // pause hides even if driving metadata was fresh
    assert(show_live_hud(true,false,true));
    assert(show_live_hud(false,true,false)); // explicit troubleshooting override
    // Visibility metadata works without the 24 MiB review pool (older cores or allocation failure).
    review::Recorder r; r.accept(session(),0);
    assert(!r.on_track(0) && !r.garage(0)); // session packet alone is not lap evidence
    r.accept(meta(1,10,1,.01f,0),1);
    assert(r.garage(1) && !r.on_track(1));
    assert(!show_live_hud(true,false,r.on_track(1)));
    r.accept(meta(1,11,2,.02f,1),1.01);
    assert(r.on_track(1.01) && !r.garage(1.01));
    assert(!r.on_track(2)); // freshness timeout hides the HUD, not guess that the car is driving
    assert(!r.on_track(std::numeric_limits<double>::quiet_NaN()));
    // Invalid metadata cannot make an unknown/garage state visible.
    auto bad=meta(1,12,3,.03f,255);r.accept(bad,2);
    assert(!r.on_track(2));
    r.reset();assert(!r.on_track(2) && !r.garage(2));
    // No pool -> packet processing must never dereference a recording slot.
    r.accept(session(),0);pair(r,1,0,1,.01f);pair(r,2,0,2,.02f);
    assert(!r.available() && r.size()==0 && r.on_track(.02));
}

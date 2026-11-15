#pragma once
#include <cmath>
namespace input_trace {
// One render-thread owner. After an interruption discard the socket queue up
// to would-block; do not call queued packets fresh because they were read now.
class ReceiveGate {
public:
    bool begin(double now, bool reset, bool paused) {
        if(!seen_ || reset || paused || paused_ != paused ||
           !std::isfinite(now) || now-last_ > .5 || now < last_) dropping_=true;
        seen_=true; last_=now; paused_=paused;
        return dropping_;
    }
    bool accept() const { return !dropping_; }
    void drained() { dropping_=false; }
    void exhausted() { dropping_=true; }
private:
    double last_{};
    bool seen_{}, paused_{}, dropping_{true};
};
}

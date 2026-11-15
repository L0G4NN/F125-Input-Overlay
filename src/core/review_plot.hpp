#pragma once
#include "lap_review.hpp"
namespace input_trace::review {
// Fixed-size per-pixel envelopes. No geometry allocation, no work for unselected laps.
struct Column { float lo{}, hi{}; bool valid{}, broken{}; };
struct Plot {
    static constexpr unsigned columns=512;
    Column series[4][columns]{};
    void build(const Recorder& r,int x,int y,float begin,float end) {
        *this={};
        if(!std::isfinite(begin) || !std::isfinite(end) || end<=begin) return;
        for(int selection=0;selection<2;++selection) {
            int idx=selection?y:x;
            if(idx<0 || std::size_t(idx)>=r.size()) continue;
            const auto& lap=r.lap(std::size_t(idx));
            auto first=std::size_t(std::clamp(begin/2,0.f,float(bins_per_lap-1)));
            auto last=std::size_t(std::clamp(end/2,0.f,float(bins_per_lap-1)));
            for(auto i=first;i<=last;++i) {
                float d=float(i)*2;
                if(d<begin || d>end) continue;
                auto col=std::min(unsigned((d-begin)/(end-begin)*columns),columns-1);
                const auto* b=r.bin(lap,i);
                // Draw envelopes only, never joining columns across missing data.
                for(unsigned channel=0;channel<2;++channel) {
                    auto& c=series[selection*2+channel][col];
                    if(!b) continue;
                    float lo=channel?b->brake_min:b->throttle_min, hi=channel?b->brake_max:b->throttle_max;
                    if(!c.valid) { c.lo=lo; c.hi=hi; c.valid=true; }
                    else { c.lo=std::min(c.lo,lo); c.hi=std::max(c.hi,hi); }
                }
            }
        }
    }
};
}

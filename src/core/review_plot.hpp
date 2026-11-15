#pragma once
#include "lap_review.hpp"
namespace input_trace::review {
// Fixed envelopes plus connections proved by consecutive recorded samples.
// Empty 2 m bins are normal at racing speed; only explicit predecessor links
// bridge them. Telemetry gaps never gain an invented connecting line.
struct Column { float lo{}, hi{}, first{}, last{}; int previous{-1}; bool valid{}, broken{}; };
struct Plot {
    static constexpr unsigned columns=192;
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
            int previous_bin=-1,previous_col=-1;
            for(auto i=first;i<=last;++i) {
                float d=float(i)*2;
                if(d<begin || d>end) continue;
                const auto* b=r.bin(lap,i);
                if(!b) continue;
                auto col=std::min(unsigned((d-begin)/(end-begin)*columns),columns-1);
                bool joined=previous_bin>=0 && (b->broken>>1)==unsigned(previous_bin+1);
                for(unsigned channel=0;channel<2;++channel) {
                    auto& c=series[selection*2+channel][col];
                    float lo=channel?b->brake_min:b->throttle_min, hi=channel?b->brake_max:b->throttle_max;
                    if(!c.valid) {
                        c.lo=lo; c.hi=hi; c.first=hi; c.valid=true;
                        c.previous=joined?previous_col:-1;
                    } else {
                        c.lo=std::min(c.lo,lo); c.hi=std::max(c.hi,hi);
                        if(!joined) { c.broken=true; c.previous=-1; }
                    }
                    c.last=hi;
                }
                previous_bin=int(i); previous_col=int(col);
            }
        }
    }
};
}

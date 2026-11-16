#pragma once
#include "lap_review.hpp"
namespace input_trace::review {
struct Column { float lo{}, hi{}, first{}, last{}; int previous{-1}; bool valid{}, broken{}; };
struct Plot {
    static constexpr unsigned columns=176; // reserve geometry for sector dashes and page controls
    Column series[4][columns]{};
    float low[2]{0,0},high[2]{1,1};
    void build(const Recorder& r,int x,int y,float begin,float end,int channel0=-1,int channel1=-1) {
        build(r,x,r,y,begin,end,channel0,channel1);
    }
    void build(const Recorder& rx,int x,const Recorder& ry,int y,float begin,float end,int channel0=-1,int channel1=-1) {
        *this={};
        if(!std::isfinite(begin) || !std::isfinite(end) || end<=begin) return;
        bool ext=channel0>=0;
        if(ext && (channel0>=int(extra_channels) || channel1<0 || channel1>=int(extra_channels))) return;
        const float spacing=ext?16.f:2.f;
        const auto count=ext?extra_bins:bins_per_lap;
        bool have[2]{};
        for(int selection=0;selection<2;++selection) {
            const auto& r=selection?ry:rx;
            int idx=selection?y:x;
            if(idx<0 || std::size_t(idx)>=r.size()) continue;
            const auto& lap=r.lap(std::size_t(idx));
            auto first=std::size_t(std::clamp(begin/spacing,0.f,float(count-1)));
            auto last=std::size_t(std::clamp(end/spacing,0.f,float(count-1)));
            int previous_bin[2]{-1,-1},previous_col[2]{-1,-1};
            for(auto i=first;i<=last;++i) {
                float d=float(i)*spacing;
                if(d<begin || d>end) continue;
                auto col=std::min(unsigned((d-begin)/(end-begin)*columns),columns-1);
                for(unsigned channel=0;channel<2;++channel) {
                    float lo,hi;unsigned link;
                    if(ext) {
                        auto b=r.extra(lap,unsigned(channel?channel1:channel0),unsigned(i));
                        if(!b) continue;
                        lo=b->lo;hi=b->hi;link=b->link;
                    } else {
                        auto b=r.bin(lap,i);if(!b) continue;
                        lo=channel?b->brake_min:b->throttle_min;hi=channel?b->brake_max:b->throttle_max;link=b->broken;
                    }
                    bool joined=previous_bin[channel]>=0 && (link>>1)==unsigned(previous_bin[channel]+1);
                    auto& c=series[selection*2+channel][col];
                    if(!c.valid) { c.lo=lo;c.hi=hi;c.first=hi;c.valid=true;c.previous=joined?previous_col[channel]:-1; }
                    else { c.lo=std::min(c.lo,lo);c.hi=std::max(c.hi,hi);if(!joined) {c.broken=true;c.previous=-1;} }
                    c.last=hi;previous_bin[channel]=int(i);previous_col[channel]=int(col);
                    if(ext) {
                        if(!have[channel]) {
                            bool height=(channel?channel1:channel0)>=12;
                            low[channel]=height?lo:std::min(0.f,lo);high[channel]=height?hi:std::max(0.f,hi);have[channel]=true;
                        }
                        else {low[channel]=std::min(low[channel],lo);high[channel]=std::max(high[channel],hi);}
                    }
                }
            }
        }
        for(unsigned c=0;c<2;++c) {
            if(ext && (c?channel1:channel0)==1) {low[c]=-1;high[c]=8;}
            if(ext && (c?channel1:channel0)>=12 && have[c]) {
                float magnitude=std::max(std::abs(low[c]),std::abs(high[c]));
                float margin=std::max({.0001f,magnitude*.00001f,(high[c]-low[c])*.1f});
                low[c]-=margin;high[c]+=margin;
            } else if(high[c]-low[c]<.001f) high[c]=low[c]+1;
        }
    }
};
}

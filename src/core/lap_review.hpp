#pragma once
#include "udp.hpp"
#include <algorithm>
#include <memory>
#include <new>

namespace input_trace::review {
constexpr std::size_t bins_per_lap=8192, trace_limit=128, lap_limit=1024;
struct Bin {
    float throttle_min{}, throttle_max{}, brake_min{}, brake_max{};
    std::uint32_t stamp{}, broken{};
};
static_assert(sizeof(Bin)==24);
struct Lap {
    unsigned number{}, time_ms{}, slot{}, stamp{};
    bool invalid{}, partial{true}, limited{}, out{}, in{};
    float first{}, last{};
};
struct Meta {
    Pedals h{};
    float distance{};
    unsigned lap{}, last_ms{}, driver{};
    bool invalid{};
};
// One render owner. Allocate/zero the pool once in ON_LOAD, never in paint.
// Generation stamps make lap starts, session resets and finalization constant-time.
class Recorder {
    struct Pool { Bin bins[trace_limit+1][bins_per_lap]{}; };
    struct Pair { Meta meta{}; Pedals pedals{}; double mw{}, pw{}; bool m{}, p{}; };
public:
    bool initialize() { if(!pool_) pool_.reset(new(std::nothrow) Pool); return bool(pool_); }
    void reset() {
        count_=stored_=0; active_=false; session_=false; garage_=false; rewound_=false; finished_=false;
        pairs_={}; seen_={}; broken_=true; have_sample_=false; ++generation_; // pool stamps are never reused
    }
    void gap() { broken_=true; if(active_) current_.partial=true; pairs_={}; }
    void set_recording(bool on) { if(recording_!=on) { recording_=on; active_=false; pairs_={}; } }
    std::size_t size() const { return count_; }
    const Lap& lap(std::size_t i) const { return laps_[i]; }
    const Bin* bin(const Lap& l,std::size_t i) const {
        if(!pool_ || l.limited || l.slot>trace_limit || i>=bins_per_lap) return nullptr;
        const auto& b=pool_->bins[l.slot][i];
        return b.stamp==l.stamp && !b.broken ? &b : nullptr;
    }
    bool recording() const { return recording_; }
    bool session_ready() const { return session_; }
    bool available() const { return bool(pool_); }
    unsigned generation() const { return generation_; }
    float length() const { return length_; }
    bool garage(double now) const { return session_ && garage_ && now>=lap_wall_ && now-lap_wall_<=.5; }
    bool metadata_limit() const { return count_==lap_limit; }
    static constexpr std::size_t storage_bytes() { return sizeof(Pool); }
    // Fully validate each packet before committing identity, ordering or lap state.
    void accept(std::span<const unsigned char> p,double wall) {
        if(!pool_ || !std::isfinite(wall) || p.size()<29) return;
        Pedals h{}; unsigned stream=0;
        if(p[6]==1) {
            if(!decode_common(p,h,1,753)) return;
            unsigned len=unsigned(p[33]) | unsigned(p[34])<<8;
            if(len<100 || len>bins_per_lap*2 || p[36]>127 || p[44]!=0 || p[154]!=0) return;
            for(auto old:retired_) if(old && old==h.session) return;
            if(session_ && h.session==uid_ && !newer(0,h.frame)) return;
            if(session_ && (h.session!=uid_ || h.player!=player_ || track_!=p[36] || length_!=len)) {
                if(h.session!=uid_) retired_[retire_pos_++%retired_.size()]=uid_;
                reset();
            }
            if(!session_) { uid_=h.session; player_=h.player; track_=p[36]; length_=float(len); session_=true; }
            remember(0,h); return;
        }
        if(p[6]==8) {
            if(!decode_common(p,h,8,1042) || !session_ || h.session!=uid_ || h.player!=player_ ||
               p[29]>22 || h.player>=p[29] || !newer(3,h.frame)) return;
            const auto* c=p.data()+30+46*h.player;
            if(c[5]>7 || c[1]==0) return;
            if(rewound_ && h.frame!=rewind_frame_ && std::uint32_t(h.frame-rewind_frame_)>=0x80000000u) return;
            remember(3,h);
            if(active_ && c[1]==current_.number) { finish(0); active_=false; }
            finished_=true; return;
        }
        Meta m{};
        if(p[6]==2) {
            if(!decode_common(p,h,2,1285)) return;
            const auto* c=p.data()+29+57*h.player;
            float d=f32(c+20);
            if(!std::isfinite(d) || d < -2000 || d>length_+100 || c[33]==0 || c[37]>1 || c[44]>4 || u32(c)>3600000) return;
            m={h,d,c[33],u32(c),c[44],c[37]!=0}; stream=1;
        } else if(p[6]==6) {
            if(!decode_header(p,h)) return;
            if(!decode_pedals(p,h)) {
                if(session_ && h.session==uid_ && h.player==player_ && newer(2,h.frame)) gap();
                return;
            }
            stream=2;
        } else return;
        if(!session_ || h.session!=uid_ || h.player!=player_ || !newer(stream,h.frame)) return;
        if(rewound_ && h.frame!=rewind_frame_ && std::uint32_t(h.frame-rewind_frame_)>=0x80000000u) return;
        if((seen_[stream] && h.time<times_[stream]) || (stream==1 && active_ && m.lap<current_.number)) {
            // Conservative rewind policy: discard this generation, including completed laps.
            // Do not combine abandoned and replacement attempts or retain stale selections.
            count_=stored_=0; active_=false; have_sample_=false; pairs_={}; ++generation_; finished_=false; gap();
            for(auto& t:times_) t=h.time;
            rewind_frame_=h.frame; rewound_=true;
            // Keep all stream frame counters: delayed pre-rewind packets remain ordered.
        }
        remember(stream,h);
        if(stream==1) {
            garage_=m.driver==0; lap_wall_=wall;
            if(!recording_ || finished_) return;
            if(active_ && m.lap!=current_.number) {
                if(m.lap==current_.number+1) finish(m.last_ms); // skipped transitions are never invented as completed laps
                active_=false;
            }
            if(!active_) {
                // Epoch wrap is practically unreachable; stop recording rather than reuse stamps.
                if(stamp_==UINT32_MAX) { recording_=false; return; }
                current_={}; current_.number=m.lap; current_.stamp=++stamp_;
                current_.slot=unsigned(stored_); current_.limited=stored_>=trace_limit;
                current_.partial=m.distance>20 || m.distance<0;
                current_.first=m.distance; current_.last=m.distance;
                active_=true; broken_=true; have_sample_=false;
            }
            current_.invalid|=m.invalid; current_.out|=m.driver==3; current_.in|=m.driver==2;
        }
        if(!recording_) return;
        // Match exact overall frames regardless of packet arrival order. No cached-distance interpolation.
        auto& pair=pairs_[h.frame%pairs_.size()];
        if((pair.m && pair.meta.h.frame!=h.frame) || (pair.p && pair.pedals.frame!=h.frame)) pair={};
        if(stream==1) { pair.meta=m; pair.mw=wall; pair.m=true; }
        else { pair.pedals=h; pair.pw=wall; pair.p=true; }
        if(pair.m && pair.p) {
            if(std::abs(pair.meta.h.time-pair.pedals.time)<=.1 && std::abs(pair.mw-pair.pw)<=.1)
                sample(pair.meta,pair.pedals);
            else gap();
            pair={};
        }
    }
private:
    void finish(unsigned ms) {
        if(count_==lap_limit) return;
        current_.partial|=!have_sample_ || current_.last<length_-100;
        current_.time_ms=ms; laps_[count_++]=current_;
        if(!current_.limited) ++stored_;
    }
    bool newer(unsigned i,std::uint32_t frame) const {
        return !seen_[i] || (frame!=frames_[i] && std::uint32_t(frame-frames_[i])<0x80000000u);
    }
    void remember(unsigned i,const Pedals& h) { frames_[i]=h.frame; times_[i]=h.time; seen_[i]=true; }
    void sample(const Meta& m,const Pedals& p) {
        if(!active_ || m.lap!=current_.number || m.driver==0 || m.distance<0 || m.distance>=length_) { gap(); return; }
        if(m.distance+2<current_.last) { gap(); return; }
        const auto i=std::size_t(m.distance/2);
        if(i>=bins_per_lap) { gap(); return; }
        bool interrupted=broken_ || (have_sample_ && (p.time-last_time_>.1 || p.time<=last_time_ || m.distance-current_.last>100));
        if(interrupted && have_sample_) current_.partial=true;
        if(!current_.limited) {
            auto& b=pool_->bins[current_.slot][i];
            if(b.stamp!=current_.stamp) b={p.throttle,p.throttle,p.brake,p.brake,current_.stamp,0};
            else {
                if(interrupted) b.broken=1;
                b.throttle_min=std::min(b.throttle_min,p.throttle); b.throttle_max=std::max(b.throttle_max,p.throttle);
                b.brake_min=std::min(b.brake_min,p.brake); b.brake_max=std::max(b.brake_max,p.brake);
            }
        }
        current_.last=m.distance; last_time_=p.time; have_sample_=true; broken_=false;
    }
    std::unique_ptr<Pool> pool_;
    std::array<Lap,lap_limit> laps_{};
    std::array<Pair,8> pairs_{};
    std::array<std::uint64_t,64> retired_{};
    std::array<std::uint32_t,4> frames_{};
    std::array<double,4> times_{};
    std::array<bool,4> seen_{};
    Lap current_{};
    std::size_t count_{}, stored_{}, retire_pos_{};
    std::uint64_t uid_{};
    unsigned player_{}, track_{}, stamp_{}, generation_{};
    std::uint32_t rewind_frame_{};
    bool rewound_{}, finished_{};
    float length_{};
    double last_time_{}, lap_wall_{};
    bool active_{}, session_{}, garage_{}, recording_{true}, broken_{true}, have_sample_{};
};
static_assert(Recorder::storage_bytes()+sizeof(Recorder)<32u*1024*1024);
}

#pragma once
#include "udp.hpp"
#include "extended.hpp"
#include <algorithm>
#include <memory>
#include <new>

namespace input_trace::review {
constexpr std::size_t bins_per_lap=8192, trace_limit=128, lap_limit=1024;
struct Bin {
    float throttle_min{}, throttle_max{}, brake_min{}, brake_max{};
    // link: bit 0 invalidates a mixed/discontinuous bin; upper bits hold
    // the preceding connected bin index + 1 (zero means no connection).
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
// One shared allocator for all cars. Stamps never repeat, even across slot reuse.
struct TracePool {
    Bin bins[trace_limit+1][bins_per_lap]{};
    std::array<bool,trace_limit> used{};
    unsigned stamp{};
    unsigned acquire() {
        for(unsigned i=0;i<trace_limit;++i) if(!used[i]) {used[i]=true;return i;}
        return unsigned(trace_limit);
    }
    void release(const Lap& lap) { if(!lap.limited && lap.slot<trace_limit) used[lap.slot]=false; }
};
// One render owner. Allocate/zero the pool once in ON_LOAD, never in paint.
// Generation stamps make lap starts, session resets and finalization constant-time.
class Recorder {
    struct Pair { Meta meta{}; Pedals pedals{}; double mw{}, pw{}; bool m{}, p{}; };
public:
    bool initialize() { if(!pool_) { owned_.reset(new(std::nothrow) TracePool); pool_=owned_.get(); } return bool(pool_) && extras_.initialize(); }
    void attach(TracePool& pool,ExtraPool& extra) {pool_=&pool;extras_.attach(extra);}
    void reset() {
        release_all();
        count_=0; active_=false; session_=false; garage_=false; lap_seen_=false; rewound_=false; finished_=false; rewind_pending_=false;
        extras_.gap(); sectors_={}; pairs_={}; seen_={}; broken_=true; have_sample_=false; ++generation_; // pool stamps are never reused
    }
    void gap() { extras_.gap(); broken_=true; if(active_) current_.partial=true; pairs_={}; }
    void set_recording(bool on) { if(recording_!=on) { recording_=on; release_current(); active_=false; pairs_={}; extras_.gap(); } }
    std::size_t size() const { return rewind_pending_?0:count_; }
    const Lap& lap(std::size_t i) const { return laps_[i]; }
    const Bin* bin(const Lap& l,std::size_t i) const {
        if(!pool_ || l.limited || l.slot>trace_limit || i>=bins_per_lap) return nullptr;
        const auto& b=pool_->bins[l.slot][i];
        return b.stamp==l.stamp && !(b.broken&1) ? &b : nullptr;
    }
    unsigned driver_count() const {return 1;}
    unsigned driver_index(unsigned) const {return 0;}
    unsigned player_index() const {return 0;}
    const Recorder& driver(unsigned) const {return *this;}
    const char* driver_name(unsigned) const {return "Player";}
    const char* driver_hint(unsigned,int) const {return "";}
    bool recording() const { return recording_; }
    bool session_ready() const { return session_; }
    bool available() const { return bool(pool_); }
    unsigned generation() const { return generation_; }
    float length() const { return length_; }
    bool fresh_lap(double now) const { return session_ && lap_seen_ && now>=lap_wall_ && now-lap_wall_<=.5; }
    bool garage(double now) const { return fresh_lap(now) && garage_; }
    bool on_track(double now) const { return fresh_lap(now) && !garage_; }
    bool metadata_limit() const { return count_==lap_limit; }
    static constexpr std::size_t storage_bytes() { return sizeof(TracePool)+ExtraStore::storage_bytes(); }
    float sector(unsigned i) const { return i<2?sectors_[i]:0; }
    bool steering(double now,float& degrees) const { return extras_.steering(now,degrees); }
    const ExtraBin* extra(const Lap& lap,unsigned channel,unsigned index) const {
        return lap.limited?nullptr:extras_.bin(lap.slot,lap.stamp,channel,index);
    }
    // A valid rewind from another car invalidates this car's future too. Hide
    // its history until its own valid lap metadata locates the retained prefix.
    void request_rewind(const Pedals& h) {
        rewind_pending_=true;pairs_={};extras_.gap();++generation_;finished_=false;
        for(auto& t:times_) t=h.time;
        rewind_frame_=h.frame;rewound_=true;
    }
    // Fully validate each packet before committing identity, ordering or lap state.
    void accept(std::span<const unsigned char> p,double wall,unsigned car=22) {
        if(car>22) return;
        auto header=[&](Pedals& h,unsigned id,std::size_t size) {
            if(!decode_common(p,h,id,size)) return false;
            if(car<22) h.player=car;
            return true;
        };
        if(!std::isfinite(wall) || p.size()<29) return;
        Pedals h{}; unsigned stream=0;
        if(p[6]==1) {
            if(!header(h,1,753)) return;
            unsigned len=unsigned(p[33]) | unsigned(p[34])<<8;
            if(len<100 || len>bins_per_lap*2 || p[36]>127 || p[44]!=0 || p[154]!=0) return;
            for(auto old:retired_) if(old && old==h.session) return;
            if(session_ && h.session==uid_ && !newer(0,h.frame)) return;
            if(session_ && (h.session!=uid_ || h.player!=player_ || track_!=p[36] || length_!=len)) {
                if(h.session!=uid_) retired_[retire_pos_++%retired_.size()]=uid_;
                reset();
            }
            if(!session_) { uid_=h.session; player_=h.player; track_=p[36]; length_=float(len); session_=true; }
            float s2=f32(p.data()+745),s3=f32(p.data()+749);
            sectors_={};
            if(std::isfinite(s2) && std::isfinite(s3) && s2>0 && s3>s2 && s3<len) sectors_={s2,s3};
            remember(0,h); return;
        }
        if(p[6]==8) {
            if(rewind_pending_) return;
            if(!header(h,8,1042) || !session_ || h.session!=uid_ || h.player!=player_ ||
               p[29]>22 || h.player>=p[29] || !newer(3,h.frame)) return;
            const auto* c=p.data()+30+46*h.player;
            if(c[5]>7 || c[1]==0) return;
            if(rewound_ && h.frame!=rewind_frame_ && std::uint32_t(h.frame-rewind_frame_)>=0x80000000u) return;
            remember(3,h);
            if(active_ && c[1]==current_.number) { finish(0); active_=false; }
            finished_=true; return;
        }
        Meta m{}; ExtraPacket extended{}; bool have_extra=false;
        if(p[6]==2) {
            if(!header(h,2,1285)) return;
            const auto* c=p.data()+29+57*h.player;
            float d=f32(c+20);
            if(!std::isfinite(d) || d < -2000 || d>length_+100 || c[33]==0 || c[37]>1 || c[44]>4 || u32(c)>3600000) return;
            m={h,d,c[33],u32(c),c[44],c[37]!=0}; stream=1;
        } else if(p[6]==6) {
            if(!header(h,6,1352)) return;
            if(!decode_pedals(p,h)) {
                if(session_ && h.session==uid_ && h.player==player_ && newer(2,h.frame)) gap();
                return;
            }
            stream=2; have_extra=decode_extra(p,extended,car);
        } else if(p[6]==7 || p[6]==13) {
            if(!decode_extra(p,extended,car)) return;
            h=extended.h;stream=p[6]==7?4:5;have_extra=true;
        } else return;
        if(!session_ || h.session!=uid_ || h.player!=player_ || !newer(stream,h.frame)) return;
        if(rewound_ && h.frame!=rewind_frame_ && std::uint32_t(h.frame-rewind_frame_)>=0x80000000u) return;
        if(!rewind_pending_ && ((seen_[stream] && h.time<times_[stream]) || (stream==1 && active_ && m.lap<current_.number))) {
            request_rewind(h);
        }
        remember(stream,h);
        if(rewind_pending_) {
            if(stream!=1) return;
            truncate(m); rewind_pending_=false;
        }
        if(stream==1) {
            garage_=m.driver==0; lap_seen_=true; lap_wall_=wall;
            if(!recording_ || !pool_ || finished_) return;
            if(active_ && m.lap!=current_.number) {
                if(m.lap==current_.number+1) finish(m.last_ms); // skipped transitions are never invented as completed laps
                else release_current();
                active_=false;
            }
            if(!active_) {
                // Epoch wrap is practically unreachable; stop recording rather than reuse stamps.
                if(pool_->stamp==UINT32_MAX) { recording_=false; return; }
                current_={}; current_.number=m.lap; current_.stamp=++pool_->stamp;
                current_.slot=pool_->acquire(); current_.limited=current_.slot>=trace_limit;
                current_.partial=m.distance>20 || m.distance<0;
                current_.first=m.distance; current_.last=m.distance;
                active_=true; broken_=true; have_sample_=false;
            }
            current_.invalid|=m.invalid; current_.out|=m.driver==3; current_.in|=m.driver==2;
        }
        if(have_extra) extras_.packet(extended,wall,recording_ && pool_ && !finished_);
        if(!recording_ || !pool_) return;
        if(stream==1 && active_ && !current_.limited && m.driver!=0)
            extras_.metadata(h.frame,current_.slot,current_.stamp,m.distance,length_,h.time,wall);
        if(stream>2) return;
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
    void truncate(const Meta& m) {
        Lap retained{}; bool found=active_ && current_.number==m.lap;
        if(found) retained=current_;
        std::size_t keep=0;
        while(keep<count_ && laps_[keep].number<m.lap) ++keep;
        if(!found && keep<count_ && laps_[keep].number==m.lap) {retained=laps_[keep];found=true;}
        if(active_ && (!found || current_.stamp!=retained.stamp)) release_current();
        if(pool_) for(std::size_t i=keep;i<count_;++i)
            if(!found || laps_[i].stamp!=retained.stamp) pool_->release(laps_[i]);
        count_=keep;
        active_=found; have_sample_=false;broken_=true;pairs_={};extras_.gap();
        if(!found) return;
        current_=retained;current_.time_ms=0;current_.partial=true;
        current_.last=m.distance;
        if(!current_.limited && pool_) {
            const auto first=std::size_t(std::clamp(m.distance/2.f,0.f,float(bins_per_lap)));
            for(auto i=first;i<bins_per_lap;++i)
                if(pool_->bins[current_.slot][i].stamp==current_.stamp) pool_->bins[current_.slot][i]={};
            extras_.truncate(current_.slot,current_.stamp,m.distance);
        }
    }
    void finish(unsigned ms) {
        extras_.seal(current_.stamp);
        if(count_==lap_limit) {release_current();return;}
        current_.partial|=!have_sample_ || current_.last<length_-100;
        current_.time_ms=ms; laps_[count_++]=current_;
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
            if(b.stamp!=current_.stamp) {
                const auto previous=std::size_t(std::max(0.f,current_.last)/2);
                const auto link=!interrupted && have_sample_ && previous<i ? std::uint32_t((previous+1)<<1) : 0u;
                b={p.throttle,p.throttle,p.brake,p.brake,current_.stamp,link};
            }
            else {
                if(interrupted) b.broken|=1;
                b.throttle_min=std::min(b.throttle_min,p.throttle); b.throttle_max=std::max(b.throttle_max,p.throttle);
                b.brake_min=std::min(b.brake_min,p.brake); b.brake_max=std::max(b.brake_max,p.brake);
            }
        }
        current_.last=m.distance; last_time_=p.time; have_sample_=true; broken_=false;
    }
    void release_current() { if(pool_ && active_) pool_->release(current_); }
    void release_all() {
        release_current();
        if(pool_) for(std::size_t i=0;i<count_;++i) pool_->release(laps_[i]);
    }
    std::unique_ptr<TracePool> owned_;
    TracePool* pool_{};
    ExtraStore extras_;
    std::array<float,2> sectors_{};
    std::array<Lap,lap_limit> laps_{};
    std::array<Pair,8> pairs_{};
    std::array<std::uint64_t,64> retired_{};
    std::array<std::uint32_t,6> frames_{};
    std::array<double,6> times_{};
    std::array<bool,6> seen_{};
    Lap current_{};
    std::size_t count_{}, retire_pos_{};
    std::uint64_t uid_{};
    unsigned player_{}, track_{}, generation_{};
    std::uint32_t rewind_frame_{};
    bool rewound_{}, finished_{}, rewind_pending_{};
    float length_{};
    double last_time_{}, lap_wall_{};
    bool active_{}, session_{}, garage_{}, lap_seen_{}, recording_{true}, broken_{true}, have_sample_{};
};
static_assert(Recorder::storage_bytes()+sizeof(Recorder)<64u*1024*1024);
}

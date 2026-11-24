#pragma once
#include "udp.hpp"
#include "extended.hpp"
#include <algorithm>
#include <memory>
#include <new>

namespace input_trace::review {
constexpr std::size_t bins_per_lap=8192, trace_limit=128, lap_limit=1024;
struct PedalExtrema {
    float throttle_min, throttle_max, brake_min, brake_max;
    void include(PedalExtrema sample) {
        throttle_min=std::min(throttle_min,sample.throttle_min);
        throttle_max=std::max(throttle_max,sample.throttle_max);
        brake_min=std::min(brake_min,sample.brake_min);
        brake_max=std::max(brake_max,sample.brake_max);
    }
};
using PedalBlock = BinBlock<PedalExtrema>;
static_assert(sizeof(PedalBlock)==584 && 2*bins_per_lap<=UINT16_MAX);
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
// One render owner. Slot views are configured before recording a session.
class Recorder {
    struct Pair { Meta meta{}; Pedals pedals{}; double mw{}, pw{}; bool m{}, p{}; };
public:
    bool initialize() {
        if(bins_) return true;
        if(!owned_) owned_.reset(new(std::nothrow) PedalBlock[trace_limit*block_count(bins_per_lap)]{});
        if(!owned_extra_) owned_extra_.reset(new(std::nothrow) ExtraBlock[trace_limit*extra_channels*block_count(extra_bins)]{});
        if(!owned_ || !owned_extra_) return false;
        attach(owned_.get(),owned_extra_.get(),owned_stamp_,trace_limit,trace_limit,
               extra_channels,bins_per_lap,extra_bins);
        return true;
    }
    void attach(PedalBlock* bins,ExtraBlock* extra,unsigned& stamp,unsigned slots,unsigned retained,
                unsigned channels,unsigned pedal_bins,unsigned extended_bins) {
        bins_=bins;stamp_=&stamp;slots_=slots;retention_=retained;distance_bins_=pedal_bins;blocks_per_slot_=block_count(pedal_bins);
        extras_.attach(extra,slots,channels,extended_bins);
        free_count_=slots_;
        for(unsigned i=0;i<slots_;++i) free_slots_[i]=slots_-1-i;
    }
    void reset() {
        release_all();
        count_=0; retained_=0; active_=false; session_=false; garage_=false; lap_seen_=false; rewound_=false; finished_=false; rewind_pending_=false;
        extras_.gap(); length_=0; sectors_={}; pairs_={}; seen_={}; broken_=true; have_sample_=false; ++generation_; // pool stamps are never reused
    }
    void transport_gap() { lap_seen_=false; gap(); }
    void gap() { extras_.gap(); broken_=true; if(active_) current_.partial=true; pairs_={}; }
    void set_recording(bool on) { if(recording_!=on) { recording_=on; release_current(); active_=false; pairs_={}; extras_.gap(); } }
    std::size_t size() const { return rewind_pending_?0:count_; }
    const Lap& lap(std::size_t i) const { return laps_[i]; }
    std::optional<PedalBlock::Sample> bin(const Lap& l,std::size_t i) const {
        if(!bins_ || l.limited || l.slot>=slots_ || i>=distance_bins_) return std::nullopt;
        return slot_blocks(l.slot)[i/block_width].read(l.stamp,i);
    }
    unsigned driver_count() const {return 1;}
    unsigned driver_index(unsigned) const {return 0;}
    unsigned player_index() const {return 0;}
    const Recorder& driver(unsigned) const {return *this;}
    const char* driver_name(unsigned) const {return "Player";}
    const char* driver_hint(unsigned,int) const {return "";}
    bool recording() const { return recording_; }
    bool session_ready() const { return session_; }
    bool available() const { return bool(bins_); }
    unsigned generation() const { return generation_; }
    float length() const { return length_; }
    bool fresh_lap(double now) const { return session_ && lap_seen_ && now>=lap_wall_ && now-lap_wall_<=.5; }
    bool garage(double now) const { return fresh_lap(now) && garage_; }
    bool on_track(double now) const { return fresh_lap(now) && !garage_; }
    bool metadata_limit() const { return count_==lap_limit; }
    unsigned detailed_lap_capacity() const { return retention_; }
    static constexpr std::size_t storage_bytes() {
        return trace_limit*(block_count(bins_per_lap)*sizeof(PedalBlock)+extra_channels*block_count(extra_bins)*sizeof(ExtraBlock));
    }
    float sector(unsigned i) const { return i<2?sectors_[i]:0; }
    bool steering(double now,float& degrees) const { return extras_.steering(now,degrees); }
    std::optional<ExtraBlock::Sample> extra(const Lap& lap,unsigned channel,unsigned index) const {
        return lap.limited?std::nullopt:extras_.bin(lap.slot,lap.stamp,channel,index);
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
            if(retired_.contains(h.session)) return;
            if(session_ && h.session==uid_ && !newer(0,h.frame)) return;
            if(session_ && (h.session!=uid_ || h.player!=player_ || track_!=p[36] || length_!=len)) {
                if(h.session!=uid_) retired_.add(uid_);
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
            if(c[5]<3 || c[5]>7) return;
            if(rewound_ && h.frame!=rewind_frame_ && std::uint32_t(h.frame-rewind_frame_)>=0x80000000u) return;
            remember(3,h);
            if(active_) {
                if(c[1]==current_.number) finish(0);
                else release_current();
                active_=false;
            }
            classification_frame_=h.frame;
            finished_=true;
            backfill_history();
            return;
        }
        if(p[6]==11) {
            if(!header(h,11,1460) || !session_ || h.session!=uid_ || p[29]!=player_ ||
               p[30]>history_times_.size() || !newer(6,h.frame) ||
               (rewound_ && precedes(h.frame,rewind_frame_))) return;
            remember(6,h);
            history_count_=p[30];
            for(unsigned i=0;i<history_count_;++i)
                history_times_[i]=u32(p.data()+36+14*i);
            if(finished_) backfill_history();
            return;
        }
        Meta m{}; ExtraPacket extended{}; bool have_extra=false;
        if(p[6]==2) {
            if(!header(h,2,1285)) return;
            const auto* c=p.data()+29+57*h.player;
            if(c[45]<2 || c[45]>7) return; // Invalid or inactive car entry.
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
        // A reordered final-lap timing packet can arrive after classification.
        if(finished_ && stream==1 && !precedes(classification_frame_,h.frame)) {
            backfill_lap_time(m);
            return;
        }
        if(!rewind_pending_ && ((seen_[stream] && h.time<times_[stream]) || (stream==1 && active_ && m.lap<current_.number))) {
            request_rewind(h);
        }
        remember(stream,h);
        // Queue target-frame telemetry as usual. The rewind barrier already
        // removed pre-rewind pairs, and only Lap Data can supply a distance.
        if(rewind_pending_ && stream==1) {
            truncate(m); rewind_pending_=false;
        }
        if(stream==1) {
            garage_=m.driver==0; lap_seen_=true; lap_wall_=wall;
            if(finished_) { backfill_lap_time(m); return; }
            if(!recording_ || !bins_) return;
            if(active_ && m.lap!=current_.number) {
                if(m.lap==current_.number+1) finish(m.last_ms); // skipped transitions are never invented as completed laps
                else release_current();
                active_=false;
            }
            if(!active_) {
                // Epoch wrap is practically unreachable; stop recording rather than reuse stamps.
                if(*stamp_==UINT32_MAX) { recording_=false; return; }
                current_={}; current_.number=m.lap; current_.stamp=++*stamp_;
                current_.limited=!free_count_;
                if(!current_.limited) current_.slot=free_slots_[--free_count_];
                current_.partial=m.distance>20 || m.distance<0;
                current_.first=m.distance; current_.last=m.distance;
                active_=true; broken_=true; have_sample_=false; pedal_samples_=0;
            }
            current_.invalid|=m.invalid; current_.out|=m.driver==3; current_.in|=m.driver==2;
        }
        if(have_extra) extras_.packet(extended,wall,recording_ && bins_ && !finished_);
        if(!recording_ || !bins_) return;
        if(stream==1 && active_ && !current_.limited && m.driver!=0)
            extras_.metadata(h.frame,current_.slot,current_.stamp,m.distance,length_,h.time,wall);
        if(stream>2) return;
        // Match exact overall frames regardless of packet arrival order. No cached-distance interpolation.
        auto& pair=pairs_[h.frame%pairs_.size()];
        if((pair.m && pair.meta.h.frame!=h.frame) || (pair.p && pair.pedals.frame!=h.frame)) pair={};
        if(stream==1) { pair.meta=m; pair.mw=wall; pair.m=true; }
        else { pair.pedals=h; pair.pw=wall; pair.p=true; }
        if(pair.m && pair.p) {
            if(!active_ || pair.meta.lap!=current_.number) { pair={}; return; }
            if(std::abs(pair.meta.h.time-pair.pedals.time)<=.1 && std::abs(pair.mw-pair.pw)<=.1)
                sample(pair.meta,pair.pedals);
            else gap();
            pair={};
        }
    }
private:
    static bool precedes(std::uint32_t a,std::uint32_t b) {
        return a!=b && std::uint32_t(b-a)<0x80000000u;
    }
    void backfill_history() {
        if(!seen_[6] || precedes(frames_[6],classification_frame_)) return;
        for(std::size_t i=0;i<count_;++i) {
            auto& lap=laps_[i];
            if(lap.number>history_count_) break;
            const unsigned ms=history_times_[lap.number-1];
            if(!lap.time_ms && ms && ms<=3600000) lap.time_ms=ms;
        }
    }
    void backfill_lap_time(const Meta& m) {
        if(count_ && !laps_[count_-1].time_ms && m.lap==laps_[count_-1].number+1)
            laps_[count_-1].time_ms=m.last_ms;
    }
    void truncate(const Meta& m) {
        Lap retained{}; bool found=active_ && current_.number==m.lap;
        if(found) retained=current_;
        std::size_t keep=0;
        while(keep<count_ && laps_[keep].number<m.lap) ++keep;
        if(!found && keep<count_ && laps_[keep].number==m.lap) {retained=laps_[keep];found=true;}
        if(active_ && (!found || current_.stamp!=retained.stamp)) release_current();
        for(std::size_t i=keep;i<count_;++i) {
            if(!laps_[i].limited) --retained_;
            if(!found || laps_[i].stamp!=retained.stamp) release(laps_[i]);
        }
        count_=keep;
        active_=found; have_sample_=false; pedal_samples_=0;broken_=true;
        if(!found) return;
        current_=retained;
        if(!recording_) { release_current(); active_=false; return; }
        current_.time_ms=0;current_.partial=true;
        current_.last=m.distance;
        if(!current_.limited && bins_) {
            const auto first=std::size_t(std::clamp(m.distance/2.f,0.f,float(distance_bins_)));
            auto* blocks=slot_blocks(current_.slot);
            PedalBlock::truncate(blocks,blocks_per_slot_,current_.stamp,first);
            for(std::size_t i=0;i<blocks_per_slot_;++i)
                pedal_samples_+=blocks[i].sample_count(current_.stamp);
            extras_.truncate(current_.slot,current_.stamp,m.distance);
        }
    }
    void finish(unsigned ms) {
        extras_.seal(current_.stamp);
        if(count_==lap_limit) {release_current();return;}
        current_.partial|=!have_sample_ || current_.last<length_-100;
        if(!current_.limited) {
            if(retained_==retention_ || (!pedal_samples_ && !extras_.has_samples(current_.stamp))) {
                release_current();
                current_.limited=true;
            } else ++retained_;
        }
        current_.time_ms=ms; laps_[count_++]=current_;
    }
    bool newer(unsigned i,std::uint32_t frame) const {
        return !seen_[i] || (frame!=frames_[i] && std::uint32_t(frame-frames_[i])<0x80000000u);
    }
    void remember(unsigned i,const Pedals& h) { frames_[i]=h.frame; times_[i]=h.time; seen_[i]=true; }
    void sample(const Meta& m,const Pedals& p) {
        if(m.driver==0 || m.distance<0 || m.distance>=length_) { gap(); return; }
        if(m.distance+2<current_.last) { gap(); return; }
        const auto i=std::size_t(m.distance/2);
        if(i>=distance_bins_) { gap(); return; }
        bool interrupted=broken_ || (have_sample_ && (p.time-last_time_>.1 || p.time<=last_time_ || m.distance-current_.last>100));
        if(interrupted && have_sample_) current_.partial=true;
        if(!current_.limited) {
            const auto previous=std::size_t(std::max(0.f,current_.last)/2);
            const unsigned link=!interrupted && have_sample_ && previous<i ? unsigned((previous+1)<<1) : 0;
            auto& block=slot_blocks(current_.slot)[i/block_width];
            pedal_samples_+=block.record(current_.stamp,i,{p.throttle,p.throttle,p.brake,p.brake},link,interrupted);
        }
        current_.last=m.distance; last_time_=p.time; have_sample_=true; broken_=false;
    }
    PedalBlock* slot_blocks(unsigned slot) const {
        return bins_+slot*blocks_per_slot_;
    }
    void release(const Lap& lap) {
        if(!lap.limited) free_slots_[free_count_++]=lap.slot;
    }
    void release_current() {
        if(active_) {
            extras_.seal(current_.stamp);
            release(current_);
        }
    }
    void release_all() {
        release_current();
        for(std::size_t i=0;i<count_;++i) release(laps_[i]);
    }
    std::unique_ptr<PedalBlock[]> owned_;
    std::unique_ptr<ExtraBlock[]> owned_extra_;
    PedalBlock* bins_{};
    std::size_t blocks_per_slot_{};
    unsigned* stamp_{};
    unsigned owned_stamp_{},slots_{},retention_{},distance_bins_{},retained_{},free_count_{};
    std::array<unsigned,trace_limit> free_slots_{};
    ExtraStore extras_;
    std::array<float,2> sectors_{};
    std::array<Lap,lap_limit> laps_{};
    std::array<Pair,8> pairs_{};
    RetiredSessions retired_;
    std::array<std::uint32_t,7> frames_{};
    std::array<double,7> times_{};
    std::array<bool,7> seen_{};
    Lap current_{};
    std::array<unsigned,100> history_times_{};
    unsigned history_count_{};
    std::size_t count_{};
    std::uint64_t uid_{};
    unsigned player_{}, track_{}, generation_{};
    std::uint32_t rewind_frame_{}, classification_frame_{};
    bool rewound_{}, finished_{}, rewind_pending_{};
    float length_{};
    double last_time_{}, lap_wall_{};
    int pedal_samples_{};
    bool active_{}, session_{}, garage_{}, lap_seen_{}, recording_{true}, broken_{true}, have_sample_{};
};
static_assert(Recorder::storage_bytes()+sizeof(Recorder)<64u*1024*1024);
}

#pragma once
#include "lap_review.hpp"
#include <cstdio>
#include <cstring>

namespace input_trace::review {
struct Participant {
    bool active{},ai{},public_{};
    unsigned driver{},network{},team{},number{};
    char name[33]{};
    bool same_identity(const Participant& p) const {
        return active==p.active && ai==p.ai && driver==p.driver && network==p.network &&
               team==p.team && number==p.number && std::strcmp(name,p.name)==0;
    }
};
// EA 2025 v3: packet 4, 30-byte prefix + 22 x 57-byte participant entries.
inline bool decode_participants(std::span<const unsigned char> p,Pedals& h,
                                std::array<Participant,22>& out) {
    if(!decode_common(p,h,4,1284) || p[29]>22) return false;
    out={};
    for(unsigned i=0;i<p[29];++i) {
        const auto* c=p.data()+30+57*i;
        if(c[0]>1 || c[39]>1 || c[40]>1 || c[44]>4) return false;
        auto& d=out[i];d.active=true;d.ai=c[0]!=0;d.public_=c[39]!=0;
        d.driver=c[1];d.network=c[2];d.team=c[3];d.number=c[5];
        // Bound untrusted labels and remove controls. The SDK's default font is ASCII.
        unsigned n=0;
        for(;n<32 && c[7+n];++n) d.name[n]=c[7+n]>=32 && c[7+n]<127?char(c[7+n]):'?';
        d.name[n]=0;
        if(!n) std::snprintf(d.name,sizeof d.name,"Car %u",i+1);
    }
    return true;
}
// One shared trace/extra pool. Each car owns its pairing, ordering, lap history
// and rewind state; selecting a different driver never changes the live player.
class MultiRecorder {
public:
    bool initialize() {
        if(!pool_) pool_.reset(new(std::nothrow) TracePool);
        if(!extra_) extra_.reset(new(std::nothrow) ExtraPool);
        if(!pool_ || !extra_) return false;
        for(auto& r:cars_) r.attach(*pool_,*extra_);
        return true;
    }
    void reset() {
        for(auto& r:cars_) r.reset();
        roster_={};roster_seen_=false;session_=false;opponents_fresh_=false;
        identity_known_={};
        ++generation_;
    }
    void gap() {for(auto& r:cars_) r.gap();}
    void set_recording(bool on) {for(auto& r:cars_) r.set_recording(on);}
    bool available() const {return bool(pool_) && bool(extra_);}
    bool recording() const {return local().recording();}
    bool session_ready() const {return local().session_ready();}
    bool fresh_lap(double now) const {return local().fresh_lap(now);}
    bool garage(double now) const {return local().garage(now);}
    bool on_track(double now) const {return local().on_track(now);}
    bool steering(double now,float& value) const {return local().steering(now,value);}
    float length() const {return local().length();}
    float sector(unsigned i) const {return local().sector(i);}
    unsigned generation() const {return generation_;}
    unsigned player_index() const {return player_;}
    const Recorder& driver(unsigned i) const {return cars_[i<22?i:player_];}
    unsigned driver_count() const {
        unsigned n=0;for(unsigned i=0;i<22;++i) if(i==player_ || roster_[i].active) ++n;
        return n;
    }
    unsigned driver_index(unsigned ordinal) const {
        for(unsigned i=0;i<22;++i) if(i==player_ || roster_[i].active) {
            if(!ordinal) return i;
            --ordinal;
        }
        return player_;
    }
    const char* driver_name(unsigned i) const {
        return i<22 && roster_[i].active?roster_[i].name:i==player_?"Player":"Unknown";
    }
    const char* driver_hint(unsigned i,int page) const {
        if(i>=22) return "Unknown driver.";
        if(i==player_) return "";
        if(page>=3) return "Player-only channel: unavailable for this car.";
        if(!roster_seen_ || !opponents_fresh_) return "Participant data stale: recording suspended.";
        if(page==2 && !roster_[i].public_) return "ERS restricted: unavailable for this car.";
        return "";
    }
    bool metadata_limit() const {
        for(const auto& r:cars_) if(r.metadata_limit()) return true;
        return false;
    }
    static constexpr std::size_t storage_bytes() {return sizeof(TracePool)+sizeof(ExtraPool);}
    void accept(std::span<const unsigned char> p,double wall) {
        if(p.size()<29 || !std::isfinite(wall)) return;
        Pedals h{};
        if(p[6]==1) {
            if(!decode_common(p,h,1,753)) return;
            const unsigned len=unsigned(p[33])|unsigned(p[34])<<8;
            if(len<100 || len>bins_per_lap*2 || p[36]>127 || p[44]!=0 || p[154]!=0) return;
            for(auto uid:retired_) if(uid && uid==h.session) return;
            if(session_ && h.session==uid_ && !newer(h.frame,session_frame_)) return;
            if(session_ && (h.session!=uid_ || h.player!=player_ || p[36]!=track_ || len!=length_)) {
                if(h.session!=uid_) retired_[retire_pos_++%retired_.size()]=uid_;
                reset();
            }
            if(!session_) ++generation_;
            session_=true;uid_=h.session;player_=h.player;track_=p[36];length_=len;session_frame_=h.frame;
            std::copy(p.begin(),p.end(),session_packet_.begin());
            for(unsigned i=0;i<22;++i) cars_[i].accept(p,wall,i);
            return;
        }
        if(!session_ || p[27]!=player_) return;
        if(p[6]==4) {
            std::array<Participant,22> next{};
            if(!decode_participants(p,h,next) || h.session!=uid_ ||
               (roster_seen_ && !newer(h.frame,roster_frame_))) return;
            bool changed=!roster_seen_;
            for(unsigned i=0;i<22;++i) {
                // Never join two occupants of the same car slot or retain private ERS.
                if(roster_seen_ && (!next[i].same_identity(roster_[i]) || next[i].public_!=roster_[i].public_)) {
                    cars_[i].reset();cars_[i].accept(session_packet_,wall,i);changed=true;identity_known_[i]=true;identity_frame_[i]=h.frame;
                }
            }
            if(!roster_seen_) for(unsigned i=0;i<22;++i) if(i!=player_ && next[i].active) {
                identity_known_[i]=true;identity_frame_[i]=h.frame;
            }
            roster_=next;roster_seen_=true;roster_frame_=h.frame;roster_wall_=wall;opponents_fresh_=true;
            if(changed) ++generation_;
            return;
        }
        const bool fresh=roster_seen_ && wall>=roster_wall_ && wall-roster_wall_<=10;
        if(!fresh && opponents_fresh_) for(unsigned i=0;i<22;++i) if(i!=player_) cars_[i].gap();
        opponents_fresh_=fresh;
        // Player first gives it priority when simultaneous starts consume the last slots.
        dispatch(player_,p,wall);
        if(!fresh) return;
        for(unsigned i=0;i<22;++i) if(i!=player_ && roster_[i].active) {
            if(p[6]==13 || (p[6]==7 && !roster_[i].public_)) continue;
            dispatch(i,p,wall);
        }
    }
private:
    static bool newer(std::uint32_t a,std::uint32_t b) {return a!=b && std::uint32_t(a-b)<0x80000000u;}
    const Recorder& local() const {return cars_[player_];}
    void dispatch(unsigned i,std::span<const unsigned char> p,double wall) {
        const auto frame=u32(p.data()+23);
        if(identity_known_[i] && frame!=identity_frame_[i] && !newer(frame,identity_frame_[i])) return;
        auto before=cars_[i].generation();cars_[i].accept(p,wall,i);
        if(cars_[i].generation()!=before) {
            // Per-car validation has established a real rewind. All other cars
            // must await their own target metadata, including missing/inactive cars.
            Pedals rewind{};rewind.frame=frame;rewind.time=f32(p.data()+15);
            for(unsigned j=0;j<22;++j) if(j!=i) cars_[j].request_rewind(rewind);
            ++generation_;
        }
    }
    std::unique_ptr<TracePool> pool_;
    std::unique_ptr<ExtraPool> extra_;
    std::array<Recorder,22> cars_{};
    std::array<std::uint32_t,22> identity_frame_{};
    std::array<bool,22> identity_known_{};
    std::array<Participant,22> roster_{};
    std::array<unsigned char,753> session_packet_{};
    std::array<std::uint64_t,64> retired_{};
    std::size_t retire_pos_{};
    std::uint64_t uid_{};
    std::uint32_t session_frame_{},roster_frame_{};
    unsigned player_{},generation_{},track_{},length_{};
    double roster_wall_{};
    bool session_{},roster_seen_{},opponents_fresh_{};
};
static_assert(MultiRecorder::storage_bytes()+sizeof(MultiRecorder)<64u*1024*1024);
}

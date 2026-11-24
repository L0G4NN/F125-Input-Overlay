#pragma once
#include "trace.hpp"
#include <algorithm>
#include <bit>
#include <span>
namespace input_trace {
inline std::uint32_t u32(const unsigned char* p) {
    return std::uint32_t(p[0]) | std::uint32_t(p[1])<<8 | std::uint32_t(p[2])<<16 | std::uint32_t(p[3])<<24;
}
inline float f32(const unsigned char* p) { return std::bit_cast<float>(u32(p)); }
struct Pedals {
    std::uint64_t session;
    std::uint32_t frame;
    unsigned player;
    double time;
    float throttle, brake;
};
// EA F1 25 v3: packed little-endian header 29 bytes, packet 6 v1,
// 22 car entries of 60 bytes, 3 trailing panel/gear bytes.
inline bool decode_common(std::span<const unsigned char> p, Pedals& out, unsigned id, std::size_t size) {
    if(p.size()!=size || p.size()<29 || (unsigned(p[0]) | unsigned(p[1])<<8)!=2025 ||
       p[2]!=25 || p[5]!=1 || p[6]!=id || p[27]>=22 || p[28]!=255) return false;
    const float t=f32(p.data()+15);
    if(!std::isfinite(t) || t<0) return false;
    out={std::uint64_t(u32(p.data()+7)) | std::uint64_t(u32(p.data()+11))<<32,
         u32(p.data()+23), p[27], t, 0, 0};
    return true;
}
inline bool decode_header(std::span<const unsigned char> p, Pedals& out) {
    return decode_common(p,out,6,1352);
}
// Header validation guarantees the stride and offsets used here.
inline bool decode_pedals(std::span<const unsigned char> p, Pedals& out) {
    const auto* car=p.data()+29+60*out.player;
    out.throttle=f32(car+2); out.brake=f32(car+10);
    return std::isfinite(out.throttle) && std::isfinite(out.brake) &&
           out.throttle>=0 && out.throttle<=1 && out.brake>=0 && out.brake<=1;
}
inline bool decode(std::span<const unsigned char> p, Pedals& out) {
    return decode_header(p,out) && decode_pedals(p,out);
}
// Session IDs are opaque, including zero. Only populated slots participate.
class RetiredSessions {
public:
    bool contains(std::uint64_t uid) const {
        const auto end=ids_.begin()+count_;
        return std::find(ids_.begin(),end,uid)!=end;
    }
    void add(std::uint64_t uid) {
        ids_[next_]=uid;
        next_=(next_+1)%ids_.size();
        count_=std::min(count_+1,ids_.size());
    }
private:
    std::array<std::uint64_t,64> ids_{};
    std::size_t next_{},count_{};
};
class Feed {
public:
    Trace<> trace;
    bool accept(std::span<const unsigned char> bytes, double wall) {
        Pedals next{};
        if(!decode_header(bytes,next) || !std::isfinite(wall)) return false;
        if(trace.state()==State::paused) return false;
        // Validate once, before committing any identity or ordering state.
        // Only a newer packet for the established player can signal a gap.
        if(!decode_pedals(bytes,next)) {
            if(have_ && next.session==session_ && next.player==player_ &&
               next.frame!=frame_ && std::uint32_t(next.frame-frame_)<0x80000000u)
                trace.unavailable();
            return false;
        }
        if(retired_.contains(next.session)) return false;
        if(have_ && next.session==session_ &&
           (next.frame==frame_ || std::uint32_t(next.frame-frame_)>=0x80000000u)) return false;
        if(have_ && next.session!=session_) { retired_.add(session_); }
        if(!have_ || next.session!=session_ || next.player!=player_) {
            trace.clear(); have_=false;
        }
        // Overall frame counter does not rewind during flashbacks. Drop old or
        // duplicate datagrams; unsigned arithmetic also handles counter wrap.
        session_=next.session; player_=next.player; frame_=next.frame; have_=true;
        if(have_ && wall-last_>.5) trace.unavailable();
        if(trace.append(next.time,next.throttle,next.brake)) { last_=wall; return true; }
        return false;
    }
    void tick(double wall) { if(have_ && trace.state()!=State::paused && wall-last_>.5) trace.unavailable(); }
    // Keep source identity and ordering across lifecycle resets. A restart with
    // the same UID can rewind time while its overall frame continues forward.
    void clear() { trace.clear(); }
private:
    RetiredSessions retired_;
    bool have_{};
    std::uint64_t session_{};
    unsigned player_{};
    std::uint32_t frame_{};
    double last_{};
};
}

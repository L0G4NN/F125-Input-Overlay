#pragma once
#include "trace.hpp"
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
inline bool decode(std::span<const unsigned char> p, Pedals& out) {
    if(p.size()!=1352 || (unsigned(p[0]) | unsigned(p[1])<<8)!=2025 ||
       p[2]!=25 || p[5]!=1 || p[6]!=6 || p[27]>=22 || p[28]!=255) return false;
    const auto* car=p.data()+29+60*p[27];
    const float t=f32(p.data()+15), throttle=f32(car+2), brake=f32(car+10);
    if(!std::isfinite(t) || t<0 || !std::isfinite(throttle) || !std::isfinite(brake) ||
       throttle<0 || throttle>1 || brake<0 || brake>1) return false;
    out={std::uint64_t(u32(p.data()+7)) | std::uint64_t(u32(p.data()+11))<<32,
         u32(p.data()+23), p[27], t, throttle, brake};
    return true;
}
class Feed {
public:
    Trace<> trace;
    bool accept(std::span<const unsigned char> bytes, double wall) {
        Pedals next{};
        if(!decode(bytes,next)) return false;
        if(!have_ || next.session!=session_ || next.player!=player_) {
            trace.clear(); have_=false;
        }
        // Overall frame counter does not rewind during flashbacks. Drop old or
        // duplicate datagrams; unsigned arithmetic also handles counter wrap.
        if(have_ && (next.frame==frame_ || std::uint32_t(next.frame-frame_)>=0x80000000u)) return false;
        session_=next.session; player_=next.player; frame_=next.frame; have_=true;
        if(trace.state()==State::paused) return false;
        if(trace.append(next.time,next.throttle,next.brake)) { last_=wall; return true; }
        return false;
    }
    void tick(double wall) { if(have_ && trace.state()!=State::paused && wall-last_>.5) trace.unavailable(); }
    void clear() { trace.clear(); have_=false; }
private:
    bool have_{};
    std::uint64_t session_{};
    unsigned player_{};
    std::uint32_t frame_{};
    double last_{};
};
}

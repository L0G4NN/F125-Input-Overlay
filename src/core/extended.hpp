#pragma once
#include "udp.hpp"
#include "bin_block.hpp"
#include <algorithm>
namespace input_trace::review {
constexpr unsigned extra_channels=14, extra_bins=1024;
struct ExtraExtrema {
    float lo,hi;
    void include(ExtraExtrema sample) {
        lo=std::min(lo,sample.lo);hi=std::max(hi,sample.hi);
    }
};
using ExtraBlock = BinBlock<ExtraExtrema>;
static_assert(sizeof(ExtraBlock)==328 && 2*extra_bins<=UINT16_MAX);
struct ExtraPacket { Pedals h{}; float values[extra_channels]{}; unsigned mask{},group{}; float steering{}; bool steering_valid{}; };
// EA 2025 v3 packed offsets. No casts to potentially unaligned structures.
inline bool decode_extra(std::span<const unsigned char> p,ExtraPacket& out,unsigned car=22) {
    if(p.size()<29 || car>22) return false;
    out={};
    if(p[6]==6) {
        if(!decode_common(p,out.h,6,1352)) return false;
        if(car<22) out.h.player=car;
        auto c=p.data()+29+60*out.h.player;
        out.values[0]=float(unsigned(c[0])|unsigned(c[1])<<8);
        out.values[1]=float(std::bit_cast<std::int8_t>(c[15]));
        if(out.values[0]>600 || out.values[1]<-1 || out.values[1]>8) return false;
        out.mask=3;out.group=0;
    } else if(p[6]==7) {
        if(!decode_common(p,out.h,7,1239)) return false;
        if(car<22) out.h.player=car;
        auto c=p.data()+29+55*out.h.player;
        out.values[2]=f32(c+33)/1000.f; // MGU-K power W -> kW
        out.values[3]=f32(c+50)/1000000.f; // cumulative deployed energy J -> MJ
        if(!std::isfinite(out.values[2]) || std::abs(out.values[2])>1000 ||
           !std::isfinite(out.values[3]) || out.values[3]<0 || out.values[3]>100) return false;
        out.mask=12;out.group=1;
    } else if(p[6]==13) {
        if(!decode_common(p,out.h,13,273) || (car<22 && car!=out.h.player)) return false;
        for(unsigned i=0;i<4;++i) {
            out.values[4+i]=f32(p.data()+93+4*i); // signed slip ratio
            out.values[8+i]=f32(p.data()+109+4*i);
        }
        out.values[12]=f32(p.data()+217); // preserve raw height units: EA field comment gives no unit
        out.values[13]=f32(p.data()+221);
        for(unsigned i=4;i<14;++i)
            if(!std::isfinite(out.values[i]) || std::abs(out.values[i])>(i<8?1000.f:i<12?360.f:10000.f)) return false;
        out.steering=f32(p.data()+197)*57.295779513f;
        out.steering_valid=std::isfinite(out.steering) && std::abs(out.steering)<=180;
        out.mask=0x3ff0;out.group=2;
    } else return false;
    return true;
}
// Extra channels use 16 m extrema bins; the existing pedals retain 2 m bins.
class ExtraStore {
    struct Meta { unsigned slot{},stamp{}; float distance{},length{}; double time{},wall{}; bool valid{}; };
    struct Pending { std::uint32_t frame{}; bool used{}; Meta meta{}; ExtraPacket packets[3]{}; double wall[3]{}; bool have[3]{},done[3]{}; };
    struct Previous { unsigned stamp{},slot{},bin{}; double time{}; float distance{}; bool valid{}; };
public:
    void attach(ExtraBlock* bins,unsigned slots,unsigned channels,unsigned distance_bins) {
        bins_=bins;slots_=slots;channels_=channels;distance_bins_=distance_bins;blocks_per_channel_=block_count(distance_bins);
    }
    bool has_samples(unsigned stamp) const { return last_sample_stamp_==stamp && visible_samples_>0; }
    void gap() { pending_={}; previous_={}; steering_valid_=false; }
    void truncate(unsigned slot,unsigned stamp,float distance) {
        gap();
        if(!bins_ || slot>=slots_) return;
        last_sample_stamp_=stamp;
        visible_samples_=0;
        const auto first=unsigned(std::clamp(distance/16.f,0.f,float(distance_bins_)));
        for(unsigned c=0;c<channels_;++c) {
            auto* blocks=channel_blocks(slot,c);
            ExtraBlock::truncate(blocks,blocks_per_channel_,stamp,first);
            for(std::size_t i=0;i<blocks_per_channel_;++i)
                visible_samples_+=blocks[i].sample_count(stamp);
        }
    }
    // Completed laps are immutable so the cached review cannot miss late writes.
    void seal(unsigned stamp) {
        for(auto& q:pending_) if(q.meta.valid && q.meta.stamp<=stamp) q={};
    }
    void metadata(std::uint32_t frame,unsigned slot,unsigned stamp,float distance,float length,double time,double wall) {
        auto& q=pending(frame);q.meta={slot,stamp,distance,length,time,wall,true};flush(q);
    }
    void packet(const ExtraPacket& p,double wall,bool record) {
        if(p.group==2) {steering_=p.steering;steering_valid_=p.steering_valid;steering_wall_=wall;}
        if(!record) return;
        auto& q=pending(p.h.frame);q.packets[p.group]=p;q.wall[p.group]=wall;q.have[p.group]=true;flush(q);
    }
    bool steering(double now,float& degrees) const {
        if(!std::isfinite(now) || !steering_valid_ || now<steering_wall_ || now-steering_wall_>.5) return false;
        degrees=steering_;return true;
    }
    std::optional<ExtraBlock::Sample> bin(unsigned slot,unsigned stamp,unsigned channel,unsigned index) const {
        if(!bins_ || slot>=slots_ || channel>=channels_ || index>=distance_bins_) return std::nullopt;
        return channel_blocks(slot,channel)[index/block_width].read(stamp,index);
    }
private:
    ExtraBlock* channel_blocks(unsigned slot,unsigned channel) const {
        return bins_+(std::size_t(slot)*channels_+channel)*blocks_per_channel_;
    }
    Pending& pending(std::uint32_t frame) { auto& q=pending_[frame%8];if(!q.used || q.frame!=frame) {q={};q.frame=frame;q.used=true;}return q; }
    void flush(Pending& q) {
        const auto m=q.meta;
        if(!bins_ || !m.valid || m.slot>=slots_ || m.distance<0 || m.distance>=m.length || m.distance>=16384) return;
        for(unsigned g=0;g<3;++g) {
            if(!q.have[g] || q.done[g]) continue;
            q.done[g]=true;const auto& p=q.packets[g];
            if(std::abs(p.h.time-m.time)>.1 || std::abs(q.wall[g]-m.wall)>.1) continue;
            const unsigned index=unsigned(m.distance/16);
            for(unsigned c=0;c<channels_;++c) if(p.mask&(1u<<c)) {
                auto& prev=previous_[c];
                // Late metadata/packets must not overwrite newer samples or an older completed lap.
                if(prev.valid && (m.stamp<prev.stamp || (m.stamp==prev.stamp && m.time<=prev.time))) continue;
                bool linked=prev.valid && prev.slot==m.slot && prev.stamp==m.stamp &&
                    m.time-prev.time<=.1 && m.distance>=prev.distance && m.distance-prev.distance<=100;
                auto& block=channel_blocks(m.slot,c)[index/block_width];
                const unsigned link=linked && prev.bin<index?(prev.bin+1)<<1:0;
                if(last_sample_stamp_!=m.stamp) {
                    last_sample_stamp_=m.stamp;
                    visible_samples_=0;
                }
                visible_samples_+=block.record(m.stamp,index,{p.values[c],p.values[c]},link,!linked);
                prev={m.stamp,m.slot,index,m.time,m.distance,true};
            }
        }
    }
    unsigned last_sample_stamp_{};
    int visible_samples_{};
    ExtraBlock* bins_{};
    std::size_t blocks_per_channel_{};
    unsigned slots_{},channels_{},distance_bins_{};
    std::array<Pending,8> pending_{};
    std::array<Previous,extra_channels> previous_{};
    double steering_wall_{};float steering_{};bool steering_valid_{};
};
}

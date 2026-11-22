#pragma once
#include "core/lap_review.hpp"
#include <vector>
namespace fixture {
using Bytes=std::vector<unsigned char>;
inline void put(Bytes& p,std::size_t o,std::uint32_t v) { for(int i=0;i<4;++i) p[o+i]=v>>(8*i); }
inline void fp(Bytes& p,std::size_t o,float v) { put(p,o,std::bit_cast<std::uint32_t>(v)); }
inline Bytes header(unsigned id,unsigned size,std::uint32_t frame,float time,unsigned player=0,std::uint64_t uid=42) {
    Bytes p(size); p[0]=2025&255; p[1]=2025>>8; p[2]=25; p[5]=1; p[6]=id;
    put(p,7,uid); put(p,11,uid>>32); put(p,23,frame); fp(p,15,time); p[27]=player; p[28]=255; return p;
}
inline Bytes session(std::uint32_t frame=0,float time=0,unsigned length=1000,std::uint64_t uid=42) {
    auto p=header(1,753,frame,time,0,uid); p[33]=length&255; p[34]=length>>8; p[36]=1; return p;
}
inline Bytes meta(unsigned lap,float distance,std::uint32_t frame,float time,unsigned driver=1,unsigned last_ms=90000,bool invalid=false) {
    auto p=header(2,1285,frame,time); put(p,29,last_ms); fp(p,49,distance); p[62]=lap; p[66]=invalid; p[73]=driver; p[74]=2; return p;
}
inline Bytes pedals(std::uint32_t frame,float time,float throttle=.75f,float brake=.25f) {
    auto p=header(6,1352,frame,time); fp(p,31,throttle); fp(p,39,brake); return p;
}
inline Bytes motion_extra(std::uint32_t frame,float time,float value=.1f) {
    auto p=header(13,273,frame,time);
    for(unsigned wheel=0;wheel<4;++wheel) { fp(p,93+4*wheel,value*(wheel+1));fp(p,109+4*wheel,-value*.1f*(wheel+1)); }
    fp(p,197,value);fp(p,217,.03f+value*.01f);fp(p,221,.05f+value*.01f);return p;
}
inline Bytes car_status(std::uint32_t frame,float time,float power=120000,float energy=2500000) {
    auto p=header(7,1239,frame,time);fp(p,62,power);fp(p,79,energy);return p;
}
inline void pair(input_trace::review::Recorder& r,unsigned lap,float distance,std::uint32_t frame,float time,bool reverse=false,float t=.75f,float b=.25f) {
    auto m=meta(lap,distance,frame,time),p=pedals(frame,time,t,b);
    if(reverse) { r.accept(p,time); r.accept(m,time); } else {r.accept(m,time); r.accept(p,time);}
}
}

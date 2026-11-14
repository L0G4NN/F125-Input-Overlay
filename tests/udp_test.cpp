#include "core/udp.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace input_trace;
void check(bool ok) { if(!ok) std::abort(); }
int main() {
    std::array<unsigned char,1352> p{};
    auto put=[&](int pos,std::uint32_t v){for(int i=0;i<4;++i)p[pos+i]=v>>(8*i);};
    auto floating=[&](int pos,float v){put(pos,std::bit_cast<std::uint32_t>(v));};
    p[0]=2025&255; p[1]=2025>>8; p[2]=25; p[5]=1; p[6]=6; p[27]=3; p[28]=255;
    put(7,42); put(23,1); floating(15,1); floating(29+3*60+2,.75f); floating(29+3*60+10,.25f);
    Pedals d{};
    check(decode(p,d) && d.player==3 && d.throttle==.75f && d.brake==.25f);
    check(!decode({p.data(),100},d));
    p[5]=2; check(!decode(p,d)); p[5]=1;
    p[27]=22; check(!decode(p,d)); p[27]=3;
    p[28]=0; check(!decode(p,d)); p[28]=255;
    Feed f; check(f.accept(p,10)); check(!f.accept(p,10.1));
    f.tick(10.6); check(f.trace.state()==State::unavailable);
    put(23,2); floating(15,1.1f); check(f.accept(p,10.7)); check(!f.trace.latest()->connected);
    put(23,1); floating(15,1.2f); check(!f.accept(p,10.8)); // old packet
    put(23,3); floating(15,.5f); check(f.accept(p,10.9)); check(f.trace.size()==1); // flashback
    put(7,99); put(23,0); floating(15,0); check(f.accept(p,11)); check(f.trace.size()==1); // new session
    floating(29+3*60+2,std::numeric_limits<float>::infinity()); check(!decode(p,d));
    std::cout<<"UDP decoding and source lifecycle checks passed\n";
}

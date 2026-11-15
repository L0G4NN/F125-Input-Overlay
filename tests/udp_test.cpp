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
    put(7,42); put(23,10); floating(15,2); check(!f.accept(p,12)); // retired session
    put(7,99); put(23,2); floating(15,.1f); check(f.accept(p,13));
    check(!f.trace.latest()->connected); // wall-clock gap
    floating(29+3*60+2,std::numeric_limits<float>::infinity()); check(!decode(p,d));
    // Lifecycle reset must preserve the identity needed to reject old sessions.
    Feed reset; put(7,1); put(23,100); floating(15,10); floating(29+3*60+2,.5f);
    check(reset.accept(p,1)); reset.clear();
    put(7,2); put(23,1); floating(15,0); check(reset.accept(p,2));
    put(7,1); put(23,101); floating(15,10.1f); check(!reset.accept(p,2.1));
    put(7,2); put(23,2); floating(15,.1f); check(reset.accept(p,2.2));
    // Same-session restart: time can rewind but overall frame ordering remains.
    reset.clear(); put(23,1); check(!reset.accept(p,2.3));
    put(23,3); floating(15,0); check(reset.accept(p,2.4));
    // An old player cannot bypass ordering by changing the index.
    p[27]=4; put(23,4); floating(15,.1f); check(reset.accept(p,2.5));
    p[27]=3; put(23,3); check(!reset.accept(p,2.6));
    // Invalid values from a current packet break the next segment.
    p[27]=4; put(23,5); floating(15,.2f);
    floating(29+4*60+2,std::numeric_limits<float>::quiet_NaN());
    check(!reset.accept(p,2.7)); check(reset.trace.state()==State::unavailable);
    floating(29+4*60+2,.5f); put(23,6); floating(15,.3f);
    check(reset.accept(p,2.8)); check(!reset.trace.latest()->connected);
    // Counter wrap is forward progress.
    Feed wrap; put(7,3); put(23,0xffffffffu); check(wrap.accept(p,3));
    put(23,0); floating(15,.4f); check(wrap.accept(p,3.1));
    std::cout<<"UDP decoding and source lifecycle checks passed\n";
}

#pragma once
#include "core/multi_review.hpp"
#include "review_fixture.hpp"
namespace multi_fixture {
using namespace fixture;
using namespace input_trace::review;
inline Bytes participants(unsigned frame=1,float time=.01f,unsigned count=3,unsigned player=0) {
    auto p=header(4,1284,frame,time,player);p[29]=count;
    for(unsigned i=0;i<count;++i) {
        const unsigned o=30+57*i;p[o]=i!=player;p[o+1]=i;p[o+5]=i+1;p[o+39]=1;
        std::snprintf(reinterpret_cast<char*>(p.data()+o+7),32,"Driver %u",i+1);
    }
    return p;
}
inline void car_meta(Bytes& p,unsigned car,unsigned lap,float distance,unsigned ms=90000,unsigned status=1) {
    const unsigned o=29+57*car;put(p,o,ms);fp(p,o+20,distance);p[o+33]=lap;p[o+44]=status;p[o+45]=2;
}
inline void car_pedals(Bytes& p,unsigned car,float throttle,float brake=.25f) {
    const unsigned o=29+60*car;fp(p,o+2,throttle);fp(p,o+10,brake);p[o]=200-car;p[o+15]=5;
}
inline void pair_grid(MultiRecorder& r,unsigned frame,float time,unsigned lap,float distance,unsigned count=3,bool reverse=false) {
    auto m=header(2,1285,frame,time),p=header(6,1352,frame,time);
    for(unsigned i=0;i<count;++i) {car_meta(m,i,lap,distance,90000+i*1000);car_pedals(p,i,float(i+1)/23);}
    if(reverse) {r.accept(p,time);r.accept(m,time);} else {r.accept(m,time);r.accept(p,time);}
}
}

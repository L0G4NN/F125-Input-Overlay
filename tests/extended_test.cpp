#include "review_fixture.hpp"
#include "core/review_plot.hpp"
#include <cassert>
#include <limits>
using namespace fixture;
using namespace input_trace::review;
Bytes motion(unsigned frame,float time) {
    auto p=header(13,273,frame,time);
    for(unsigned i=0;i<4;++i) {fp(p,93+4*i,.1f*(i+1));fp(p,109+4*i,-.01f*(i+1));}
    fp(p,197,.1f);fp(p,217,.03f);fp(p,221,.05f);return p;
}
Bytes status(unsigned frame,float time) {
    auto p=header(7,1239,frame,time);fp(p,29+33,120000);fp(p,29+50,2500000);return p;
}
int main() {
    ExtraPacket decoded;
    auto p=motion(1,.01f);assert(decode_extra(p,decoded));assert(decoded.mask==0x3ff0);
    assert(decoded.values[4]==.1f && decoded.values[7]==.4f && decoded.values[12]==.03f);
    assert(decoded.steering>5.72f && decoded.steering<5.74f);
    p=status(1,.01f);assert(decode_extra(p,decoded));assert(decoded.values[2]==120 && decoded.values[3]==2.5f);
    // Nonzero player offsets and signed reverse gear.
    p=pedals(1,.01f);p[27]=3;p[29+3*60]=250;p[29+3*60+15]=255;
    assert(decode_extra(p,decoded) && decoded.values[0]==250 && decoded.values[1]==-1);
    p=status(1,.01f);p[27]=3;fp(p,29+55*3+33,90000);assert(decode_extra(p,decoded) && decoded.values[2]==90);
    for(unsigned id:{6u,7u,13u}) {
        auto good=id==6?pedals(1,.01f):id==7?status(1,.01f):motion(1,.01f);
        for(unsigned n=0;n<good.size();++n) assert(!decode_extra(std::span(good.data(),n),decoded));
        good.push_back(0);assert(!decode_extra(good,decoded));
    }
    p=motion(1,.01f);fp(p,93,std::numeric_limits<float>::quiet_NaN());assert(!decode_extra(p,decoded));
    Recorder r;assert(r.initialize());p=session();fp(p,745,300);fp(p,749,700);r.accept(p,0);
    assert(r.sector(0)==300 && r.sector(1)==700);
    // Motion/status before metadata; speed after metadata. Every channel is paired independently.
    r.accept(motion(1,.01f),.01);r.accept(status(1,.01f),.01);r.accept(meta(1,0,1,.01f),.01);
    p=pedals(1,.01f);p[29]=200;p[44]=5;r.accept(p,.01);
    r.accept(meta(1,32,2,.02f),.02);r.accept(motion(2,.02f),.02);r.accept(status(2,.02f),.02);r.accept(pedals(2,.02f),.02);
    float steering=0;assert(r.steering(.1,steering));assert(!r.steering(1,steering));
    r.gap();r.accept(meta(1,64,3,.03f),.03);r.accept(motion(3,.03f),.03);
    r.accept(meta(2,0,4,.04f),.04);
    assert(r.size()==1);
    auto& lap=r.lap(0);
    assert(r.extra(lap,0,0)->hi==200 && r.extra(lap,1,0)->hi==5);
    assert(r.extra(lap,2,0)->hi==120 && r.extra(lap,3,0)->hi==2.5f);
    for(unsigned c=4;c<14;++c) {assert(r.extra(lap,c,0));assert(r.extra(lap,c,2)->link==2);assert(r.extra(lap,c,4)->link==0);}
    r.accept(status(3,.03f),.04); // late data cannot mutate a finalized/cached lap
    assert(!r.extra(lap,2,4)); // Missing status is unavailable, not cached power.
    Plot plot;plot.build(r,0,-1,0,1000,0,1);assert(plot.high[0]>=200 && plot.low[1]==-1 && plot.high[1]==8);
    // Valid newer metadata cannot consume an older frame's motion value.
    r.accept(motion(5,.05f),.05);r.accept(meta(2,160,6,.06f),.06);r.accept(meta(3,0,7,.07f),.07);
    assert(!r.extra(r.lap(1),4,10));
    // Paired packets beyond receipt tolerance do not produce a sample.
    r.accept(motion(8,.08f),.08);r.accept(meta(3,160,8,.08f),.3);r.accept(meta(4,0,9,.09f),.31);
    assert(!r.extra(r.lap(2),4,10));
    // Malformed high-frame data must not poison ordering or clear saved laps.
    p=status(10000,10);fp(p,62,std::numeric_limits<float>::infinity());r.accept(p,.4);
    assert(r.size()==3);r.accept(status(10,.1f),.4);
    r.set_recording(false);r.accept(motion(10,.1f),.4);assert(r.steering(.4,steering));
    r.reset();assert(!r.steering(.4,steering) && !r.sector(0));
    p=session();fp(p,745,700);fp(p,749,300);r.accept(p,0);assert(!r.sector(0));
    // Constant heights, even at defensive limits, must retain a nonzero axis range.
    r.set_recording(true);p=motion(1,.01f);fp(p,217,10000);fp(p,221,10000);r.accept(p,.01);
    r.accept(meta(1,0,1,.01f),.01);r.accept(meta(2,0,2,.02f),.02);
    plot.build(r,0,-1,0,1000,12,13);
    assert(plot.high[0]>plot.low[0] && std::isfinite(1/(plot.high[0]-plot.low[0])));
    assert(Recorder::storage_bytes()+sizeof r<64u*1024*1024);
}

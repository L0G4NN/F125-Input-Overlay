#include "review_fixture.hpp"
#include "core/review_plot.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace input_trace::review;
using namespace fixture;
void check(bool ok,const char* why) { if(!ok) {std::cerr<<why<<'\n'; std::abort();} }
int main() {
    Recorder r; check(r.initialize(),"pool init"); r.accept(session(),0);
    pair(r,1,0,1,.01f); pair(r,1,1,2,.02f,true,.1f,1); pair(r,1,1.5f,3,.03f,false,1,0);
    // Advance all the way to the line without a source gap.
    for(unsigned i=4;i<=502;++i) pair(r,1,float(i-3)*2, i,float(i)*.01f);
    r.accept(meta(2,0,504,5.04f),5.04);
    check(r.size()==1,"one completed lap"); auto l=r.lap(0);
    check(l.number==1 && l.time_ms==90000 && !l.partial,"lap timing/full coverage");
    auto bin=r.bin(l,0); check(bin && bin->throttle_min==.1f && bin->throttle_max==1 && bin->brake_max==1,"retain spike and overlap");
    r.accept(meta(2,0,504,5.04f),5.04); check(r.size()==1,"no duplicate completion");
    Plot plot; plot.build(r,0,-1,0,1000); check(plot.series[0][0].valid,"plot data");
    // Sparse 2 m bins at racing speed still form a connected trace, but a
    // real telemetry gap cannot be bridged.
    {
        Recorder sparse; check(sparse.initialize(),"sparse init"); sparse.accept(session(),0);
        pair(sparse,1,0,1,.01f); pair(sparse,1,10,2,.02f,false,.6f,.4f);
        sparse.gap(); pair(sparse,1,20,3,.03f); pair(sparse,1,30,4,.04f);
        sparse.accept(meta(2,0,5,.05f),.05);
        Plot q; q.build(sparse,0,-1,0,100);
        const unsigned c10=unsigned(10.f/100*Plot::columns),c20=unsigned(20.f/100*Plot::columns),c30=unsigned(30.f/100*Plot::columns);
        check(q.series[0][c10].previous==0,"sparse samples connect");
        check(q.series[0][c20].previous==-1,"gap not connected");
        check(q.series[0][c30].previous==int(c20),"connection resumes after gap");
        check(q.series[2][0].valid==false,"unselected laps not drawn");
        q.build(sparse,0,-1,0,16384); // gap folds into one pixel: no through-line
        check(q.series[0][0].broken,"subpixel discontinuity retained");
        q.build(sparse,0,-1,10,30);
        check(q.series[0][0].previous==-1,"zoom edge cannot reference offscreen data");
    }
    // Garage gating expires and is never inferred from silence.
    r.accept(meta(2,10,505,5.05f,0),5.05); check(r.garage(5.1),"fresh garage"); check(!r.garage(6),"stale garage");
    // Exact-frame pairing only; unmatched and delayed values never inherit cached distance.
    r.accept(meta(2,100,506,5.06f),5.06); r.accept(pedals(507,5.07f),5.07);
    r.accept(meta(3,0,508,5.08f),5.08); check(r.size()==2 && r.lap(1).partial,"missing pairs marked partial");
    check(!r.bin(r.lap(1),50),"no invented pedal sample");
    // Rewind invalidates the generation and all comparison handles; delayed old stream rejected.
    auto gen=r.generation(); r.accept(meta(2,500,509,3),5.09);
    check(r.size()==1 && r.generation()!=gen,"rewind retains earlier lap");
    r.accept(pedals(508,5.08f),5.1); r.accept(pedals(509,3),5.1);
    check(r.size()==1,"delayed frame cannot restore laps");
    // Truncation and hostile fields do not change tracked identity or finalize laps.
    auto bad=session(510,3,1000,99); bad.resize(100); r.accept(bad,5.2);
    bad=meta(255,std::numeric_limits<float>::quiet_NaN(),511,3.1f); r.accept(bad,5.3);
    bad=session(512,3.2f,65535,99); r.accept(bad,5.4); check(r.size()==1,"reject malformed metadata");
    // Rewind into a completed lap: keep its prefix and earlier laps, replace
    // the boundary bin and all later data, even if pedals arrive before metadata.
    {
        Recorder t;check(t.initialize(),"rewind init");t.accept(session(),0);
        pair(t,1,0,1,.01f);pair(t,1,998,2,.02f);
        pair(t,2,0,3,.03f,false,.2f,0);pair(t,2,100,4,.04f,false,.3f,0);
        t.accept(motion_extra(5,.05f,.9f),.05);pair(t,2,200,5,.05f,false,.9f,1);
        pair(t,3,0,6,.06f);pair(t,3,500,7,.07f);
        check(t.size()==2,"two complete before rewind");auto old=t.lap(1);
        check(t.extra(old,4,12).has_value(),"extended abandoned sample exists before rollback");
        t.accept(pedals(8,.035f,.4f,0),.08); // rewind without distance yet
        t.accept(meta(2,100,8,.035f),.08);
        check(t.size()==1 && t.bin(t.lap(0),0),"earlier lap preserved");
        check(t.bin(old,0) && !t.bin(old,50) && !t.bin(old,100),"prefix only; boundary discarded");
        check(!t.extra(old,4,12),"extended future discarded");
        pair(t,2,100,9,.045f,false,.4f,0);pair(t,2,200,10,.055f,false,.5f,0);
        pair(t,3,0,11,.065f);
        check(t.size()==2 && t.lap(1).partial,"rewritten lap finalized once");
        auto b=t.bin(t.lap(1),100);check(b && b->throttle_max==.5f && b->brake_max==0,"old peak absent");
        pair(t,2,0,12,.02f);check(t.size()==1,"second rewind removes completed suffix");
        pair(t,3,0,13,.03f);check(t.size()==2,"slot reusable after second rewind");
    }
    // Fill all 128 trace slots, then retain metadata without touching selected traces.
    r.reset(); r.accept(session(),0); std::uint32_t frame=1; float time=0;
    for(unsigned lap=1;lap<=201;++lap) { time+=.02f; pair(r,lap,0,frame++,time); }
    check(r.size()==200,"high lap count metadata");
    check(!r.lap(127).limited && r.lap(128).limited,"128 detail limit");
    check(r.bin(r.lap(0),0) && r.bin(r.lap(127),0) && !r.bin(r.lap(128),0),"stable retained storage");
    std::cout<<"Review lifecycle/bounds checks passed; pool="<<Recorder::storage_bytes()<<" bytes, recorder="<<sizeof(Recorder)<<" bytes\n";
    r.set_recording(false); pair(r,202,0,frame++,time+=.02f); check(r.size()==200,"recording disabled");
    r.set_recording(true); pair(r,202,0,frame++,time+=.02f); pair(r,203,0,frame++,time+=.02f);
    check(r.size()==201,"recording resumed");
    // Final classification confirms a final lap even if Lap Data never increments.
    auto final=header(8,1042,frame++,time+=.02f); final[29]=22;final[31]=203;final[35]=3;
    r.accept(final,time);check(r.size()==202 && r.lap(201).number==203 && r.lap(201).time_ms==0,"final lap with unknown time");
    r.accept(final,time);check(r.size()==202,"no duplicate classification");
    auto oldest=r.lap(0);
    pair(r,128,0,frame++,1.f);
    check(r.size()==127 && r.bin(oldest,0),"cap rollback retains older slots");
    pair(r,128,100,frame++,1.02f,false,.25f,0);
    pair(r,129,0,frame++,1.04f);
    check(r.size()==128 && !r.lap(127).limited,"reclaimed detailed slot");
    auto rebuilt=r.bin(r.lap(127),50);check(rebuilt && rebuilt->throttle_max==.25f,"reclaimed slot fresh values");
    // A changed session clears selections/data; an old retired session cannot take over.
    auto oldsession=session(1,0);r.accept(session(frame++,time,1000,99),time);
    check(r.size()==0,"new session resets");r.accept(oldsession,time+.01);check(r.size()==0,"retired source ignored");
    // Invalid packets for other sessions must not change generation.
    auto generation=r.generation();auto other=session(frame++,time,0,123);r.accept(other,time);
    check(r.generation()==generation,"invalid session cannot change generation");
    r.reset(); check(r.size()==0 && !r.garage(time),"session cleanup");
}

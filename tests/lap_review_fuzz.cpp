#include "review_fixture.hpp"
#include "core/review_plot.hpp"
#include <random>
#include <iostream>
#include <cstdlib>
using namespace fixture;
using namespace input_trace::review;
int main() {
    Recorder r; if(!r.initialize()) std::abort(); std::mt19937_64 rng(20251115);
    Plot plot;
    for(unsigned n=0;n<300000;++n) {
        if(n%5000==0) {r.reset();r.accept(session(),0);}
        Bytes p;
        switch(n%8) {
        case 0: p=session(n,float(n)/60);break;
        case 1: p=meta(1+n/5000,float(n%1000),n,float(n)/60);break;
        case 2: p=pedals(n,float(n)/60);break;
        case 3: p=header(8,1042,n,float(n)/60);p[29]=22;break;
        case 4: p=header(7,1239,n,float(n)/60);break;
        case 5: p=header(13,273,n,float(n)/60);break;
        default:p=Bytes(rng()%2049);break;
        }
        if(n%8<6 && n%7==0) for(unsigned j=0;j<1+rng()%8;++j) p[rng()%p.size()]=rng();
        r.accept(p,double(n)/60);
        if(n%43==0) r.gap();
        if(n%601==0) r.set_recording(rng()%2);
        if(r.size()>lap_limit || Recorder::storage_bytes()+sizeof(Recorder)>64u*1024*1024) std::abort();
        if(n%1000==0) {
            plot.build(r,r.size()?int(r.size()-1):-1,-1,0,r.length());
            for(auto& s:plot.series) for(auto c:s) if(c.valid && (!std::isfinite(c.lo)||c.lo<0||c.hi>1||c.lo>c.hi)) std::abort();
            for(int channel=0;channel<14;channel+=2) {
                plot.build(r,r.size()?int(r.size()-1):-1,-1,0,r.length(),channel,channel+1);
                for(auto& series:plot.series) for(auto c:series)
                    if(c.valid && (!std::isfinite(c.lo) || !std::isfinite(c.hi) || c.lo>c.hi)) std::abort();
            }
        }
    }
    std::cout<<"300000 mutated review packet/state cases passed\n";
}

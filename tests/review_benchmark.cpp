#include "review_fixture.hpp"
#include "core/review_plot.hpp"
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <new>
// Count C++ allocations after fixture/pool preparation; recording and Plot must not allocate.
static std::size_t allocations=0;
void* operator new(std::size_t n) {++allocations;if(auto p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
using namespace input_trace::review;
using namespace fixture;
using Clock=std::chrono::steady_clock;
int main() {
    volatile float sink=0;
    for(unsigned completed:{0u,78u,128u,200u}) {
        Recorder r;if(!r.initialize())return 1;r.accept(session(),0);
        unsigned frame=1;float time=0;
        for(unsigned l=1;l<=completed+1;++l)pair(r,l,0,frame++,time+=.02f);
        auto m=meta(completed+1,10,frame,time),p=pedals(frame,time),motion=motion_extra(frame,time),status=car_status(frame,time);
        auto before=allocations;auto start=Clock::now();
        constexpr unsigned iterations=300000;
        for(unsigned i=0;i<iterations;++i){++frame;time+=.016f;put(m,23,frame);put(p,23,frame);fp(m,15,time);fp(p,15,time);put(motion,23,frame);fp(motion,15,time);put(status,23,frame);fp(status,15,time);r.accept(m,time);r.accept(p,time);r.accept(motion,time);r.accept(status,time);}
        auto elapsed=std::chrono::duration<double,std::micro>(Clock::now()-start).count()/iterations;
        auto growth=allocations-before;
        std::cout<<completed<<" completed laps: four-packet ingest "<<elapsed<<" us, allocations="<<growth<<" bytes reserved="<<Recorder::storage_bytes()+sizeof r<<'\n';
        if(growth)return 2;
        Plot plot;before=allocations;start=Clock::now();
        for(unsigned i=0;i<1000;++i){plot.build(r,r.size()?0:-1,r.size()>1?1:-1,0,1000,4,8);sink=plot.series[0][0].hi;}
        std::cout<<"  selection rebuild "<<std::chrono::duration<double,std::micro>(Clock::now()-start).count()/1000<<" us, allocations="<<allocations-before<<'\n';
        if(allocations!=before)return 3;
    }
    (void)sink;
}

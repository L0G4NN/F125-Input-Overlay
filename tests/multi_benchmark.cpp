#include "multi_fixture.hpp"
#include "core/review_plot.hpp"
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <new>
static std::size_t allocations=0;
void* operator new(std::size_t n) {++allocations;if(auto p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
using namespace multi_fixture;
using Clock=std::chrono::steady_clock;
int main() {
    volatile float sink=0;
    auto r=std::make_unique<MultiRecorder>();if(!r->initialize())return 1;
    for(unsigned completed:{0u,78u,128u,200u}) {
        r->reset();r->accept(session(),0);r->accept(participants(1,.01f,22),.01);
        unsigned frame=2;float time=.02f;
        for(unsigned lap=1;lap<=completed+1;++lap) {
            pair_grid(*r,frame++,time,lap,0,22);time+=.01f;
            pair_grid(*r,frame++,time,lap,32,22);time+=.01f;
        }
        auto m=header(2,1285,frame,time),p=header(6,1352,frame,time),status=header(7,1239,frame,time);
        auto motion=motion_extra(frame,time),roster=participants(frame,time,22);
        for(unsigned i=0;i<22;++i) {car_meta(m,i,completed+1,34);car_pedals(p,i,.75f);fp(status,29+55*i+33,120000);}
        const auto before=allocations;const auto start=Clock::now();
        constexpr unsigned iterations=60000;
        for(unsigned j=0;j<iterations;++j) {
            ++frame;time+=.016f;
            for(auto b:{&m,&p,&status,&motion,&roster}) {put(*b,23,frame);fp(*b,15,time);}
            if(j%300==0)r->accept(roster,time);
            r->accept(m,time);r->accept(p,time);r->accept(status,time);r->accept(motion,time);
        }
        auto us=std::chrono::duration<double,std::micro>(Clock::now()-start).count()/iterations;
        auto growth=allocations-before;
        std::cout<<completed<<" completed laps/car: 22-car four-packet ingest "<<us<<" us; allocations "<<growth<<'\n';
        if(growth)return 2;
        Plot plot;auto start_plot=Clock::now();auto before_plot=allocations;
        for(unsigned j=0;j<1000;++j) {plot.build(r->driver(0),0,r->driver(1),0,0,1000);sink=plot.series[j%4][j%Plot::columns].hi;}
        std::cout<<"  X/Y plot rebuild "<<std::chrono::duration<double,std::micro>(Clock::now()-start_plot).count()/1000<<" us; allocations "<<allocations-before_plot<<'\n';
        if(allocations!=before_plot)return 3;
    }
    (void)sink;
    std::cout<<"Reserved including all recorder metadata: "<<MultiRecorder::storage_bytes()+sizeof(MultiRecorder)<<" bytes\n";
}

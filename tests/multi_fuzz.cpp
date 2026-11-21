#include "multi_fixture.hpp"
#include "core/review_plot.hpp"
#include <random>
#include <iostream>
using namespace multi_fixture;
int main() {
    auto r=std::make_unique<MultiRecorder>();if(!r->initialize())return 1;
    std::mt19937 random(12522);Plot plot;
    for(unsigned i=0;i<300000;++i) {
        if(i%1000==0) {r->reset();r->accept(session(),0);r->accept(participants(1,.01f,22),.01);}
        const unsigned frame=i%1000+2;const float t=frame*.01f;
        Bytes p;
        switch(random()%7) {
        case 0:p=participants(frame,t,22);break;
        case 1:p=header(2,1285,frame,t);for(unsigned c=0;c<22;++c)car_meta(p,c,1+frame/100,float(frame%100)*9);break;
        case 2:p=header(6,1352,frame,t);for(unsigned c=0;c<22;++c)car_pedals(p,c,.5f);break;
        case 3:p=header(7,1239,frame,t);break;
        case 4:p=motion_extra(frame,t);break;
        case 5:p=session(frame,t);break;
        default:p=header(8,1042,frame,t);p[29]=22;break;
        }
        if(random()%2) for(unsigned j=0,n=1+random()%6;j<n;++j)p[random()%p.size()]=random();
        if(random()%10==0)p.resize(random()%(p.size()+20));
        r->accept(p,t);
        if(i%100==0)plot.build(r->driver(random()%22),0,r->driver(random()%22),0,0,1000,0,1);
    }
    std::cout<<"300000 multi-driver mutated packet cases passed\n";
}

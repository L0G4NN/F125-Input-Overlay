#include "core/layout.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
 for(auto sw:{1280.f,1920.f,2560.f,3440.f}) for(auto sh:{720.f,1080.f,1440.f})
 for(int corner=0;corner<4;++corner) for(int x:{0,50,100}) for(int y:{0,50,100}) {
    auto p=input_trace::layout(sw,sh,900,360,corner,x,y);
    assert(p.x>=0 && p.y>=0 && p.x+p.w<=sw+.01 && p.y+p.h<=sh+.01);
    if(x==50) assert(std::abs(p.x-(sw-p.w)/2)<.01);
    if(y==50) assert(std::abs(p.y-(sh-p.h)/2)<.01);
 }
 auto a=input_trace::layout(1920,1080,420,160,0,0,0);
 assert(a.x==24 && a.y==896 && a.w==420 && a.h==160);
 auto b=input_trace::layout(1920,1080,420,160,3,0,0);
 assert(b.x==1476 && b.y==24);
 auto c=input_trace::layout(1920,1080,420,160,0,-1,101);
 assert(c.x==24 && c.y==24);
 std::cout<<"Layout checks passed\n";
}

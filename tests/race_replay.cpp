// Dense full-field recording through the production packet path. Run each case
// as its own process to measure peak resident memory for the same race.
#include "multi_fixture.hpp"
#include <charconv>
#include <iostream>
#include <string_view>

using namespace multi_fixture;
static bool parse(const char* text, unsigned& value) {
    std::string_view arg(text);
    auto [end, error]=std::from_chars(arg.begin(),arg.end(),value);
    return error==std::errc{} && end==arg.end();
}
int main(int argc,char** argv) {
    unsigned length=7004,laps=70;
    if(argc!=3 || !parse(argv[1],length) || !parse(argv[2],laps) ||
       length<100 || length>16384 || laps==0 || laps>254) {
        std::cerr<<"Usage: race_replay TRACK_METRES COMPLETED_LAPS\n";
        return 1;
    }
    auto recorder=std::make_unique<MultiRecorder>();
    if(!recorder->initialize()) return 2;
    recorder->accept(session(0,0,length),0);
    auto roster=participants(1,0,22);
    recorder->accept(roster,0);
    auto meta=header(2,1285,0,0), pedals=header(6,1352,0,0);
    auto status=car_status(0,0),motion=motion_extra(0,0);
    for(unsigned i=0;i<22;++i) {
        car_pedals(pedals,i,float(i+1)/23);
        fp(status,29+55*i+33,1000.f*(i+1));
    }
    unsigned frame=1;
    for(unsigned lap=1;lap<=laps;++lap) {
        for(unsigned distance=0;distance<length;distance+=2) {
            ++frame;
            const float time=frame*.016f;
            for(auto* packet:{&meta,&pedals,&status,&motion,&roster}) {
                put(*packet,23,frame);fp(*packet,15,time);
            }
            if(frame%300==0) recorder->accept(roster,time);
            for(unsigned i=0;i<22;++i) car_meta(meta,i,lap,float(distance));
            recorder->accept(meta,time);recorder->accept(pedals,time);
            if(distance%16==0) {recorder->accept(status,time);recorder->accept(motion,time);}
        }
    }
    // The start of the next lap supplies authoritative last-lap timing.
    ++frame;
    float time=frame*.016f;
    put(meta,23,frame);fp(meta,15,time);
    for(unsigned i=0;i<22;++i) car_meta(meta,i,laps+1,0);
    recorder->accept(meta,time);
    unsigned traces=0;
    for(unsigned i=0;i<22;++i) {
        const auto& driver=recorder->driver(i);
        if(driver.size()!=laps) return 3;
        for(unsigned j=0;j<laps;++j) {
            const auto& lap=driver.lap(j);
            if(lap.number!=j+1 || lap.time_ms!=90000 ||
               lap.limited!=(j>=recorder->detailed_lap_capacity())) return 4;
            if(lap.limited) continue;
            auto pedal=driver.bin(lap,(length-1)/2);
            auto ers=driver.extra(lap,2,(length-1)/16);
            if(!pedal || pedal->throttle_max!=float(i+1)/23 || !ers || ers->hi!=float(i+1) ||
               bool(driver.extra(lap,13,(length-1)/16))!=(i==0)) return 5;
            ++traces;
        }
    }
    std::cout<<length<<" m, "<<laps<<" laps, "<<traces<<" detailed traces, "
             <<recorder->active_storage_bytes()<<" track layout bytes\n";
}

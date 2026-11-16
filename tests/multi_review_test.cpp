#include "multi_fixture.hpp"
#include "core/review_plot.hpp"
#include "review_fixture.hpp"
#include <cassert>
#include <limits>
#include <iostream>
using namespace fixture;
using namespace input_trace::review;
using namespace multi_fixture;
int main() {
    auto owner=std::make_unique<MultiRecorder>();auto& r=*owner;assert(r.initialize());
    r.accept(session(),0);r.accept(participants(),.01);
    assert(r.driver_count()==3 && r.player_index()==0);
    pair_grid(r,2,.02f,1,0);pair_grid(r,3,.03f,1,32,3,true);
    r.accept(motion_extra(3,.03f),.03);
    auto status=header(7,1239,3,.03f);
    for(unsigned i=0;i<3;++i) {fp(status,29+55*i+33,1000.f*(i+1));fp(status,29+55*i+50,1e6f*(i+1));}
    r.accept(status,.03);
    pair_grid(r,4,.04f,1,998);pair_grid(r,5,.05f,2,0);
    for(unsigned i=0;i<3;++i) {
        const auto& c=r.driver(i);assert(c.size()==1);
        const auto& l=c.lap(0);assert(l.time_ms==90000+i*1000);
        assert(c.bin(l,16) && c.bin(l,16)->throttle_max==float(i+1)/23);
        assert(c.extra(l,2,2) && c.extra(l,2,2)->hi==float(i+1));
        assert(bool(c.extra(l,4,2))==(i==0));
        assert(c.lap(0).slot!=r.driver((i+1)%3).lap(0).slot);
    }
    Plot plot;
    plot.build(r.driver(0),0,r.driver(1),0,0,1000);
    assert(plot.series[0][0].hi!=plot.series[2][0].hi);
    // Same lap number refers to independent car histories. Lap transitions need not coincide.
    auto m=header(2,1285,6,.06f);car_meta(m,0,2,32);car_meta(m,1,3,0,81000);car_meta(m,2,2,32);
    r.accept(m,.06);assert(r.driver(1).size()==2 && r.driver(0).size()==1);
    plot.build(r.driver(0),0,r.driver(1),1,0,1000);
    plot.build(r.driver(1),0,r.driver(1),1,0,1000);
    // Duplicate roster refresh must not close the review or clear selections.
    auto gen=r.generation();r.accept(participants(7,.07f),.07);assert(r.generation()==gen);
    // Restriction change removes earlier ERS and rejects zeroed private data.
    auto roster=participants(8,.08f);roster[30+57+39]=0;r.accept(roster,.08);
    assert(r.driver(1).size()==0);
    pair_grid(r,9,.09f,3,0);r.accept(header(7,1239,9,.09f),.09);pair_grid(r,10,.1f,4,0);
    assert(r.driver(1).size()==1 && !r.driver(1).extra(r.driver(1).lap(0),2,0));
    assert(std::strstr(r.driver_hint(1,2),"restricted"));
    // All truncations and malformed roster packets are rejected without identity/order changes.
    for(std::size_t n=0;n<roster.size();++n) r.accept(std::span(roster.data(),n),.11);
    roster=participants(10000,100);roster[30+39]=2;r.accept(roster,.11);
    roster=participants(11,.11f);roster[30+57+39]=0;r.accept(roster,.11);
    assert(r.driver_count()==3);
    // Untrusted names cannot read outside their fixed field or inject line breaks.
    roster=participants(12,.12f);std::memset(roster.data()+30+7,'A',32);roster[30+8]='\n';
    r.accept(roster,.12);assert(std::strlen(r.driver_name(0))==32 && r.driver_name(0)[1]=='?');
    // Invalid high-frame data for one opponent must not poison its next valid frame or the player.
    m=header(2,1285,13,.13f);for(unsigned i=0;i<3;++i)car_meta(m,i,5,0);r.accept(m,.13);
    auto p=header(6,1352,10000,100);for(unsigned i=0;i<3;++i)car_pedals(p,i,.5f);
    fp(p,29+60+2,std::numeric_limits<float>::quiet_NaN());
    r.accept(p,.14);
    p=header(6,1352,14,.14f);car_pedals(p,1,.6f);r.accept(p,.14);
    m=header(2,1285,14,.14f);car_meta(m,1,5,32);r.accept(m,.14);
    m=header(2,1285,15,.15f);car_meta(m,1,6,0);r.accept(m,.15);
    auto& valid=r.driver(1);assert(valid.bin(valid.lap(valid.size()-1),16)->throttle_max==.6f);
    // Session reset frees shared slots. Full field remains below the fixed allocation cap.
    r.reset();r.accept(session(),0);r.accept(participants(1,.01f,22),.01);
    unsigned frame=2;float time=.02f;
    for(unsigned lap=1;lap<=200;++lap) {
        if(lap%20==1) r.accept(participants(frame++,time,22),time);
        pair_grid(r,frame++,time,lap,0,22);time+=.01f;
        pair_grid(r,frame++,time,lap,32,22,true);time+=.01f;
        pair_grid(r,frame++,time,lap,998,22);time+=.01f;
    }
    std::size_t traces=0,metadata=0;
    for(unsigned i=0;i<22;++i) {
        assert(r.driver(i).size()==199);metadata+=r.driver(i).size();
        for(std::size_t j=0;j<r.driver(i).size();++j) if(!r.driver(i).lap(j).limited) ++traces;
    }
    assert(traces==128 && metadata==4378);
    // Global flashback: each car's target lap/distance differs and retains its own prefix.
    m=header(2,1285,frame++,1.f);
    for(unsigned i=0;i<22;++i)car_meta(m,i,2+i%2,20);
    r.accept(m,time);
    for(unsigned i=0;i<22;++i) assert(r.driver(i).size()==1+i%2);
    // Complete the reopened lap and verify old post-rewind data has disappeared.
    m=header(2,1285,frame++,1.01f);
    for(unsigned i=0;i<22;++i)car_meta(m,i,3+i%2,0);
    r.accept(m,time+.01);
    for(unsigned i=0;i<22;++i) {
        auto& c=r.driver(i);auto& l=c.lap(c.size()-1);
        assert(c.bin(l,0) && !c.bin(l,16) && !c.bin(l,499));
    }
    // Nonzero local car: an AI garage must never grant focus or hide the player's driving HUD.
    r.reset();auto s=session();s[27]=2;r.accept(s,0);r.accept(participants(1,.01f,3,2),.01);
    m=header(2,1285,2,.02f,2);car_meta(m,0,1,0,90000,0);car_meta(m,2,1,0,90000,1);r.accept(m,.02);
    assert(r.on_track(.02) && !r.garage(.02));
    auto motion=motion_extra(2,.02f);motion[27]=2;r.accept(motion,.02);float angle;
    assert(r.steering(.02,angle));
    // Slot replacement cannot accept delayed packets from its previous occupant.
    r.reset();r.accept(session(),0);r.accept(participants(1,.01f),.01);
    pair_grid(r,2,.02f,1,0);pair_grid(r,3,.03f,2,0);
    roster=participants(10,.1f);roster[30+57+1]=99;r.accept(roster,.1);
    assert(r.driver(1).size()==0);
    pair_grid(r,8,.08f,1,0);pair_grid(r,9,.09f,2,0);
    assert(r.driver(1).size()==0);
    pair_grid(r,11,.11f,1,0);pair_grid(r,12,.12f,2,0);
    assert(r.driver(1).size()==1);
    // Roster expiry affects opponent recording, never local-player safety metadata.
    m=header(2,1285,13,.13f);car_meta(m,0,3,0);car_meta(m,1,3,0);
    r.accept(m,11);assert(r.driver(1).size()==1 && r.on_track(11));
    assert(std::strstr(r.driver_hint(1,0),"stale"));
    // Wrong-session and unsupported-format packets must leave every driver's history intact.
    roster=participants(100,.2f);put(roster,7,999);auto before=r.generation();
    r.accept(roster,11.1);assert(r.generation()==before && r.driver(1).size()==1);
    roster[0]=0;r.accept(roster,11.1);assert(r.generation()==before);
    // No participant packet: only player data is considered; missing slots never become zero-valued AI.
    r.reset();r.accept(session(),0);pair_grid(r,1,.01f,1,0);pair_grid(r,2,.02f,2,0);
    assert(r.driver(0).size()==1 && r.driver(1).size()==0);
    std::cout<<"Multi-driver isolation, privacy, rewind and 22-car/200-lap checks passed; "
             <<MultiRecorder::storage_bytes()+sizeof(MultiRecorder)<<" bytes total\n";
}

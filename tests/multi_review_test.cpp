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
    // Every accepted layout fits both backing arrays and retains at least as
    // many completed laps as the previous 24/16-byte per-bin layout.
    for(unsigned length=100;length<=16384;++length) {
        TrackHistory layout(length);
        const auto old_bytes=22*layout.pedals*24+98*layout.extended*16;
        assert(layout.slots>=std::min(71u,(255u*1024*1024)/old_bytes));
        assert(layout.pedal_blocks()<=history_backing[0] && layout.extra_blocks()<=history_backing[1]);
    }
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
    // A long race lets every driver finish 78 detailed laps, with timing-only overflow.
    r.reset();r.accept(session(1,0,7004),0);r.accept(participants(1,.01f,22),.01);
    assert(r.detailed_lap_capacity()==78);
    assert(r.active_storage_bytes()==147200384);
    unsigned frame=2;float time=.02f;
    for(unsigned lap=1;lap<=200;++lap) {
        if(lap%20==1) r.accept(participants(frame++,time,22),time);
        pair_grid(r,frame++,time,lap,0,22);time+=.01f;
        pair_grid(r,frame++,time,lap,32,22,true);time+=.01f;
        pair_grid(r,frame++,time,lap,7003,22);time+=.01f;
    }
    std::size_t traces=0,metadata=0;
    for(unsigned i=0;i<22;++i) {
        const auto& car=r.driver(i);
        assert(car.size()==199);metadata+=car.size();
        assert(car.bin(car.lap(77),3501) && !car.lap(77).limited);
        assert(car.lap(78).limited && car.lap(78).time_ms==90000+i*1000);
        for(std::size_t j=0;j<r.driver(i).size();++j) if(!r.driver(i).lap(j).limited) ++traces;
    }
    assert(traces==22*78 && metadata==4378);
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
        assert(c.bin(l,0) && !c.bin(l,16) && !c.bin(l,3501));
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
    // The maximum accepted length lowers the quota equally, with its last
    // pedal and extra bins accessible even in the last car slice.
    r.reset();r.accept(session(1,0,16384),0);
    auto capacity=r.detailed_lap_capacity();
    assert(capacity>0 && capacity<78);
    const auto max_bytes=r.active_storage_bytes();
    assert(max_bytes+sizeof(r)<telemetry_budget);
    r.accept(participants(1,.01f,22),.01);
    for(unsigned lap=1;lap<=capacity+2;++lap) {
        const unsigned f=lap+1;
        pair_grid(r,f,float(f)*.01f,lap,16383,22);
    }
    for(unsigned i=0;i<22;++i) {
        const auto& car=r.driver(i);
        assert(car.size()==capacity+1);
        assert(car.bin(car.lap(capacity-1),8191));
        assert(car.extra(car.lap(capacity-1),0,1023));
        assert(car.lap(capacity).limited);
    }

    // A new track changes both strides and carries no data from the old
    // partitions. The full channel set follows the new nonzero local player.
    s=session(100,.3f,101);s[27]=21;r.accept(s,.3);
    assert(r.active_storage_bytes()<max_bytes && r.detailed_lap_capacity()==78);
    r.accept(participants(101,.31f,22,21),.31);
    m=header(2,1285,102,.32f,21);car_meta(m,21,1,100);
    auto e=motion_extra(102,.32f);e[27]=21;
    p=header(6,1352,102,.32f,21);car_pedals(p,21,.5f);r.accept(p,.32);
    r.accept(e,.32);r.accept(m,.32);
    m=header(2,1285,103,.33f,21);car_meta(m,21,2,0);r.accept(m,.33);
    const auto& player=r.driver(21);
    assert(player.size()==1 && player.bin(player.lap(0),50) && !player.bin(player.lap(0),49));
    assert(player.extra(player.lap(0),13,6) && !player.extra(player.lap(0),13,5));
    // Same UID and local index, different track length. Late frames from the
    // previous layout must not seed the new roster or the local driver's laps.
    s=session(200,.4f,7004);s[27]=21;r.accept(s,.4);
    r.accept(participants(180,.36f,22,21),.41);
    assert(r.driver_count()==1);
    m=header(2,1285,181,.37f,21);car_meta(m,21,7,32);r.accept(m,.42);
    m=header(2,1285,182,.38f,21);car_meta(m,21,8,0);r.accept(m,.43);
    assert(r.driver(21).size()==0 && !r.fresh_lap(.43));
    r.accept(participants(200,.4f,22,21),.44);
    m=header(2,1285,200,.4f,21);car_meta(m,21,1,0);r.accept(m,.44);
    assert(r.driver_count()==22 && r.fresh_lap(.44));
    // A non-active array entry cannot masquerade as a flashback.
    r.reset();r.accept(session(),0);r.accept(participants(1,.01f,2),.01);
    pair_grid(r,2,.02f,1,32,2);
    pair_grid(r,3,.03f,2,960,2);
    m=header(2,1285,4,.04f);
    car_meta(m,0,2,970);car_meta(m,1,1,0);m[29+57+45]=1;
    auto generation=r.generation();
    r.accept(m,.04);
    assert(r.generation()==generation && r.driver(0).size()==1 && r.driver(1).size()==1);

    // A missing roster entry remains selectable, and classification can
    // finish its active lap even after the last roster has gone stale.
    r.accept(participants(5,.05f,1),.05);
    assert(r.driver_count()==2 && r.driver(1).size()==1);
    auto final=header(8,1042,7,10.2f);
    final[29]=2;final[31]=2;final[35]=3;final[31+46]=2;final[35+46]=3;
    auto history=header(11,1460,8,10.3f);
    history[29]=1;history[30]=2;put(history,36+14,91456);
    r.accept(history,10.19); // Newer packet arrives before classification.
    assert(r.driver(1).size()==1);
    r.accept(final,10.2);
    assert(r.driver(0).size()==2 && r.driver(1).size()==2 && r.driver(1).lap(1).time_ms==91456);
    m=header(2,1285,6,10.1f);
    car_meta(m,0,3,0,87321);
    r.accept(m,10.21);
    assert(r.driver(0).lap(1).time_ms==87321);

    // Timing-only laps release their slots. Extended-only coverage is enough
    // to retain a trace, even when no pedals arrived for that frame.
    r.reset();r.accept(session(),0);
    for(unsigned lap=1;lap<=79;++lap) r.accept(meta(lap,0,lap,lap*.01f),lap*.01);
    r.accept(meta(79,32,80,.8f),.8);
    r.accept(motion_extra(80,.8f),.8);
    r.accept(meta(80,0,81,.81f),.81);
    assert(r.driver(0).size()==79);
    for(unsigned i=0;i<78;++i) assert(r.driver(0).lap(i).limited);
    const auto& recorded=r.driver(0).lap(78);
    assert(!recorded.limited && r.driver(0).extra(recorded,12,2));
    plot.build(r.driver(0),78,-1,33,53,12,13);
    assert(plot.series[0][0].valid);
    std::cout<<"Multi-driver isolation, privacy, rewind and 22-car/200-lap checks passed; "
             <<MultiRecorder::storage_bytes()+sizeof(MultiRecorder)<<" bytes total\n";
}

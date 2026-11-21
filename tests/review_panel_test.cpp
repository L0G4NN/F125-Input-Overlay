#include <swofty_api.h>
#include <cstdio>
#define VK_ESCAPE 27
#include "swofty/review_panel.hpp"
#include "review_fixture.hpp"
#include "multi_fixture.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
using namespace input_trace::review;
namespace {
constexpr int default_hotkey=117; // Win32 VK_F6
int bound_hotkey=default_hotkey;
char rendered_key_hint[64]{};
bool opponent_selected=false,unavailable_hint=false,last_driver_visible=false,last_driver_selected=false;
SwoftyInput input{}; int sectors=0; int glyphs=0; bool amber=false,cyan=false; int focus_calls=0, lines=0, texts=0,rects=0, input_calls=0; bool focused=false; int focus_result=SWOFTY_ASSET_OK;
int read_input(const SwoftyPaint*,SwoftyInput* out) { ++input_calls; *out=input; out->focused=focused; return SWOFTY_ASSET_OK; }
int focus(int on) { ++focus_calls; if(focus_result==SWOFTY_ASSET_OK || !on) focused=on!=0; return focus_result; }
void size(const SwoftyPaint*,float* w,float* h) {*w=1920;*h=1080;}
void rect(SwoftyPaint*,float,float,float,float,uint32_t) {++rects;}
void line(SwoftyPaint*,float x,float y,float a,float b,float,uint32_t colour) {if(colour==0xffffff80) ++sectors;if(colour==0xffbd59ff) amber=true;if(colour==Theme::x) cyan=true;if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(a)||!std::isfinite(b))std::abort();++lines;}
void text(SwoftyPaint*,float,float,float,uint32_t,const char* str) {++texts;glyphs+=int(std::strlen(str));if(std::strstr(str,"22  Driver 22"))last_driver_visible=true;if(std::strstr(str,"Y: Driver 22 / no lap"))last_driver_selected=true;if(std::strstr(str,"Y: Driver 2 / lap 1"))opponent_selected=true;if(std::strstr(str,"Player-only"))unavailable_hint=true;if(std::strstr(str," / Esc  Close review"))std::snprintf(rendered_key_hint,sizeof rendered_key_hint,"%s",str);}
void check(bool ok) {if(!ok)std::abort();}
}
int main() {
    Recorder r;check(r.initialize());r.accept(fixture::session(),0);
    fixture::pair(r,1,0,1,.01f);fixture::pair(r,1,998,2,.02f);r.accept(fixture::meta(2,0,3,.03f),.03);
    SwoftyOverlay ov{};ov.input=read_input;ov.focus=focus;ov.size=size;ov.rect=rect;ov.line=line;ov.text=text;
    SwoftyApi api{};api.overlay=&ov;Panel p;
    auto paint=[&](bool safe=true,bool active=true,bool supported=true){p.paint(&api,nullptr,r,0,safe,active,bound_hotkey,supported);};
    paint();check(!p.open() && !texts && !lines && !rects);
    // F8 must not open a new F6-default panel or acquire input focus.
    input.keys_pressed[119/8]|=1u<<(119%8);paint();check(!p.open() && !focus_calls);input={};
    input.keys_pressed[default_hotkey/8]|=1u<<(default_hotkey%8);paint(false);check(!p.open()); auto reads=input_calls;paint(false);check(input_calls==reads);
    paint(true,true,false);check(!p.open() && !focus_calls); // old cores: never touch v3
    paint();check(p.open() && focused);input={};paint();
    check(lines<=2060 && texts>0 && std::strcmp(rendered_key_hint,"F6 / Esc  Close review")==0);
    bound_hotkey=118;paint();check(std::strcmp(rendered_key_hint,"F7 / Esc  Close review")==0);
    bound_hotkey=default_hotkey;
    // Another overlay takes focus: close without reclaiming it.
    focused=false;paint();check(!p.open() && !focused);
    input.keys_pressed[default_hotkey/8]|=1u<<(default_hotkey%8);paint();check(p.open());input={};paint(true,false);check(!p.open() && !focused);
    focus_result=SWOFTY_ASSET_LATE;input.keys_pressed[default_hotkey/8]|=1u<<(default_hotkey%8);paint();check(!p.open() && !focused);
    focus_result=SWOFTY_ASSET_OK;paint();check(p.open());input={};input.keys_pressed[27/8]|=1u<<(27%8);paint();check(!p.open() && !focused);
    // F6 must open and remain open despite held controls, drift and unrelated input.
    for(int kind=0;kind<6;++kind) {
        input={};input.keys_pressed[default_hotkey/8]|=1u<<(default_hotkey%8);
        if(kind==0) input.keys_down[87/8]|=1u<<(87%8);
        if(kind==1) input.pad_buttons=SWOFTY_PAD_A;
        if(kind==2) input.pad_rt=.8f;
        if(kind==3) input.pad_ly=.7f;
        if(kind==4) {input.char_count=1;input.chars[0]='S';}
        if(kind==5) input.keys_pressed[13/8]|=1u<<(13%8);
        paint();check(p.open() && focused);
        input.keys_pressed[default_hotkey/8]=0;paint();check(p.open());
        input={};input.keys_pressed[27/8]|=1u<<(27%8);paint();check(!p.open() && !focused);
    }
    // Losing the allowed context (e.g. resume on track) closes without an input read.
    input={};input.keys_pressed[default_hotkey/8]|=1u<<(default_hotkey%8);paint();check(p.open());
    auto before=input_calls;input={};paint(false);check(!p.open() && !focused && input_calls==before);
    // Full graphs, all twelve quality-labelled rows and cursor readout must
    // leave space within Swofty's 16,384-vertex budget (6 per glyph/quad/line).
    r.reset();auto session=fixture::session();fixture::fp(session,745,300);fixture::fp(session,749,700);r.accept(session,0);unsigned frame=1;float time=0;
    for(unsigned lap=1;lap<=13;++lap) {
        time+=.01f;r.accept(fixture::meta(lap,0,frame++,time,3,90000,true),time);
        if(lap==13) break;
        for(unsigned d=2;d<1000;d+=2) {
            if(d==10) r.gap();
            time+=.01f;
            r.accept(fixture::motion_extra(frame,time,float(d%100)/100),time);
            r.accept(fixture::car_status(frame,time,float(d%100)*1200,float(d)*2500),time);
            fixture::pair(r,lap,float(d),frame++,time,false,float(d%100)/100,float((d+30)%100)/100);
        }
    }
    input={};input.keys_pressed[default_hotkey/8]|=1u<<(default_hotkey%8);paint();check(p.open());
    input={};input.x=580;input.y=215;input.pressed=SWOFTY_MOUSE_LEFT;paint(); // Y = first row
    input={};input.x=1000;input.y=300;lines=rects=glyphs=0;amber=cyan=false;paint();
    check(amber && cyan && 6*(lines+rects+glyphs)<16384);
    // Switch to shared-axis view and back without changing the selected laps.
    input={};input.x=1050;input.y=145;input.pressed=SWOFTY_MOUSE_LEFT;paint();
    input={};input.x=1000;input.y=300;lines=rects=glyphs=0;amber=cyan=false;paint();
    check(amber && cyan && 6*(lines+rects+glyphs)<16384);
    // Exercise every page at full X/Y geometry density, with sectors and cursor.
    for(int page=0;page<5;++page) {
        input={};input.x=60+610+page*230;input.y=70+110;input.pressed=SWOFTY_MOUSE_LEFT;paint();
        input={};input.x=1000;input.y=300;lines=rects=glyphs=sectors=0;amber=cyan=false;paint();
        check(amber && cyan && sectors>0 && 6*(lines+rects+glyphs)<16384);
    }
    // Drive the real independent driver selectors, then a same-number lap in
    // another car. Input policy remains the same and missing player-only data is explicit.
    p.close(&ov);
    auto grid=std::make_unique<MultiRecorder>();check(grid->initialize());
    grid->accept(fixture::session(),0);grid->accept(multi_fixture::participants(),.01);
    multi_fixture::pair_grid(*grid,2,.02f,1,0);
    multi_fixture::pair_grid(*grid,3,.03f,1,32);
    grid->accept(fixture::motion_extra(3,.03f),.03);
    multi_fixture::pair_grid(*grid,4,.04f,1,998);
    multi_fixture::pair_grid(*grid,5,.05f,2,0);
    Panel cross;
    auto draw_grid=[&]{cross.paint(&api,nullptr,*grid,.05,true,true,bound_hotkey,true);};
    input={};input.keys_pressed[default_hotkey/8]|=1u<<(default_hotkey%8);draw_grid();check(cross.open());
    input={};input.x=380;input.y=130;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid(); // open Y dropdown
    input={};input.x=380;input.y=260;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid(); // choose car 2
    input={};input.x=580;input.y=215;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid(); // Y lap 1
    input={};lines=rects=glyphs=0;amber=cyan=false;draw_grid();
    check(opponent_selected && amber && cyan && 6*(lines+rects+glyphs)<16384);
    // Outside dismissal must not click Clear comparison underneath the popup.
    input={};input.x=380;input.y=130;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid();
    input={};input.x=700;input.y=145;input.pressed=SWOFTY_MOUSE_LEFT;opponent_selected=false;draw_grid();
    check(opponent_selected && cross.open());
    input={};input.x=380;input.y=130;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid();
    input={};input.keys_pressed[27/8]|=1u<<(27%8);draw_grid();check(cross.open());
    input={};input.x=60+610+3*230;input.y=180;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid();
    input={};lines=rects=glyphs=0;amber=cyan=false;draw_grid();
    check(unavailable_hint && cyan && !amber && 6*(lines+rects+glyphs)<16384);
    auto roster=multi_fixture::participants(6,.06f);grid->accept(roster,.06);draw_grid();
    check(cross.open()); // unchanged 5-second roster updates never close or reset the UI
    // A valid global rewind hides an opponent's stale future while its metadata is missing.
    auto rewind=fixture::pedals(7,.001f);grid->accept(rewind,.07);draw_grid();
    check(!cross.open() && grid->driver(1).size()==0);
    grid->reset();grid->accept(fixture::session(),0);grid->accept(multi_fixture::participants(1,.01f,22),.01);
    input={};input.keys_pressed[default_hotkey/8]|=1u<<(default_hotkey%8);draw_grid();check(cross.open());
    input={};input.x=380;input.y=130;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid();
    for(int n=0;n<2;++n) {input={};input.x=530;input.y=175;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid();}
    input={};lines=rects=glyphs=0;draw_grid();check(last_driver_visible && 6*(lines+rects+glyphs)<16384);
    input={};input.x=380;input.y=540;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid();
    check(last_driver_selected); // eighth entry on the final page is car 22
    // Wheel scrolling and Escape dismiss do not change the selected driver.
    input={};input.x=380;input.y=130;input.pressed=SWOFTY_MOUSE_LEFT;draw_grid();
    input={};input.x=380;input.y=300;input.wheel=1;draw_grid();
    input={};input.keys_pressed[27/8]|=1u<<(27%8);draw_grid();check(cross.open());
    input={};last_driver_selected=false;draw_grid();check(last_driver_selected);
    std::cout<<"Panel focus, feature-gating and draw-budget checks passed\n";
}

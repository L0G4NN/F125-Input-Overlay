#include <swofty_api.h>
#include <cstdio>
#define VK_ESCAPE 27
#include "swofty/review_panel.hpp"
#include "review_fixture.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
using namespace input_trace::review;
namespace {
SwoftyInput input{}; int sectors=0; int glyphs=0; bool amber=false,cyan=false; int focus_calls=0, lines=0, texts=0,rects=0, input_calls=0; bool focused=false; int focus_result=SWOFTY_ASSET_OK;
int read_input(const SwoftyPaint*,SwoftyInput* out) { ++input_calls; *out=input; out->focused=focused; return SWOFTY_ASSET_OK; }
int focus(int on) { ++focus_calls; if(focus_result==SWOFTY_ASSET_OK || !on) focused=on!=0; return focus_result; }
void size(const SwoftyPaint*,float* w,float* h) {*w=1920;*h=1080;}
void rect(SwoftyPaint*,float,float,float,float,uint32_t) {++rects;}
void line(SwoftyPaint*,float x,float y,float a,float b,float,uint32_t colour) {if(colour==0xffffff80) ++sectors;if(colour==0xffbd59ff) amber=true;if(colour==0x55d9ffff) cyan=true;if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(a)||!std::isfinite(b))std::abort();++lines;}
void text(SwoftyPaint*,float,float,float,uint32_t,const char* str) {++texts;glyphs+=int(std::strlen(str));}
void check(bool ok) {if(!ok)std::abort();}
}
int main() {
    Recorder r;check(r.initialize());r.accept(fixture::session(),0);
    fixture::pair(r,1,0,1,.01f);fixture::pair(r,1,998,2,.02f);r.accept(fixture::meta(2,0,3,.03f),.03);
    SwoftyOverlay ov{};ov.input=read_input;ov.focus=focus;ov.size=size;ov.rect=rect;ov.line=line;ov.text=text;
    SwoftyApi api{};api.overlay=&ov;Panel p;
    auto paint=[&](bool safe=true,bool active=true,bool supported=true){p.paint(&api,nullptr,r,0,safe,active,119,supported);};
    paint();check(!p.open() && !texts && !lines && !rects);
    input.keys_pressed[119/8]|=1u<<(119%8);paint(false);check(!p.open()); auto reads=input_calls;paint(false);check(input_calls==reads);
    paint(true,true,false);check(!p.open() && !focus_calls); // old cores: never touch v3
    paint();check(p.open() && focused);input={};paint();check(lines<=2060 && texts>0);
    // Another overlay takes focus: close without reclaiming it.
    focused=false;paint();check(!p.open() && !focused);
    input.keys_pressed[119/8]|=1u<<(119%8);paint();check(p.open());input={};paint(true,false);check(!p.open() && !focused);
    focus_result=SWOFTY_ASSET_LATE;input.keys_pressed[119/8]|=1u<<(119%8);paint();check(!p.open() && !focused);
    focus_result=SWOFTY_ASSET_OK;paint();check(p.open());input={};input.keys_pressed[27/8]|=1u<<(27%8);paint();check(!p.open() && !focused);
    // Setup naming: closed review must not acquire focus or draw, even with typed characters.
    input={};input.char_count=1;input.chars[0]='A';int acquisitions=focus_calls;
    paint();check(!p.open() && !focused && focus_calls==acquisitions);
    // Returning to the game with keyboard, characters or controller navigation releases review.
    for(int kind=0;kind<4;++kind) {
        input={};input.keys_pressed[119/8]|=1u<<(119%8);paint();check(p.open() && focused);
        input={};
        if(kind==0) {input.char_count=1;input.chars[0]='S';}
        if(kind==1) input.keys_pressed[13/8]|=1u<<(13%8); // Enter in game setup/menu
        if(kind==2) input.pad_pressed=SWOFTY_PAD_A;
        if(kind==3) input.pad_ly=.7f;
        paint();check(!p.open() && !focused);
    }
    // Never take focus while an existing control is held (including same-frame F8).
    for(int kind=0;kind<3;++kind) {
        input={};input.keys_pressed[119/8]|=1u<<(119%8);
        if(kind==0) input.keys_down[87/8]|=1u<<(87%8); // W already held, no new edge
        if(kind==1) input.pad_buttons=SWOFTY_PAD_A;
        if(kind==2) input.pad_rt=.8f;
        auto before=focus_calls;paint();check(!p.open() && focus_calls==before);
    }
    // Entering replay/flashback closes an open review without another input read.
    input={};input.keys_pressed[119/8]|=1u<<(119%8);paint();check(p.open());
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
    input={};input.keys_pressed[119/8]|=1u<<(119%8);paint();check(p.open());
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
    std::cout<<"Panel focus, feature-gating and draw-budget checks passed\n";
}

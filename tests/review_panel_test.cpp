#include <swofty_api.h>
#include <cstdio>
#define VK_ESCAPE 27
#include "swofty/review_panel.hpp"
#include "review_fixture.hpp"
#include <cstdlib>
#include <iostream>
using namespace input_trace::review;
namespace {
SwoftyInput input{}; int focus_calls=0, lines=0, texts=0,rects=0, input_calls=0; bool focused=false; int focus_result=SWOFTY_ASSET_OK;
int read_input(const SwoftyPaint*,SwoftyInput* out) { ++input_calls; *out=input; out->focused=focused; return SWOFTY_ASSET_OK; }
int focus(int on) { ++focus_calls; if(focus_result==SWOFTY_ASSET_OK || !on) focused=on!=0; return focus_result; }
void size(const SwoftyPaint*,float* w,float* h) {*w=1920;*h=1080;}
void rect(SwoftyPaint*,float,float,float,float,uint32_t) {++rects;}
void line(SwoftyPaint*,float x,float y,float a,float b,float,uint32_t) {if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(a)||!std::isfinite(b))std::abort();++lines;}
void text(SwoftyPaint*,float,float,float,uint32_t,const char*) {++texts;}
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
    std::cout<<"Panel focus, feature-gating and draw-budget checks passed\n";
}

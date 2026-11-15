#pragma once
#include "core/review_plot.hpp"
// Included by the Windows adapter. All methods are called by its render owner.
namespace input_trace::review {
class Panel {
public:
    void close(const SwoftyOverlay* ov) {
        if(open_) ov->focus(0);
        open_=false; dragging_=false;
    }
    bool open() const { return open_; }
    void paint(const SwoftyApi* api,SwoftyPaint* paint,const Recorder& r,
               double now,bool safe,bool active_window,int hotkey,bool supported) {
        if(!supported) return; // Never access v3 fields without runtime feature detection.
        auto ov=api->overlay;
        if(generation_!=r.generation()) { close(ov); generation_=r.generation(); x_=y_=-1; scroll_=0; view_end_=0; }
        if(!safe || !active_window || !r.available()) { close(ov); return; }
        SwoftyInput in{}; in.size=sizeof in;
        if(ov->input(paint,&in)!=SWOFTY_ASSET_OK) { close(ov); return; }
        if(open_ && !in.focused) { close(ov); return; } // Another addon owns focus; never reclaim it.
        if(key(in,hotkey)) {
            if(open_) { close(ov); return; }
            int result=ov->focus(1);
            if(result!=SWOFTY_ASSET_OK) { ov->focus(0); return; }
            open_=true;
            if(x_<0 && r.size()) x_=int(r.size()-1);
            return; // Wait for our own focused snapshot before interacting.
        }
        if(!open_) return;
        if(key(in,VK_ESCAPE)) { close(ov); return; }
        float sw=0,sh=0; ov->size(paint,&sw,&sh);
        float scale=std::min(sw/1920,sh/1080);
        if(scale<=0) { close(ov); return; }
        // All following coordinates are in a 1920x1080 design, centred on screen.
        float ox=(sw-1800*scale)/2, oy=(sh-940*scale)/2;
        float mx=(in.x-ox)/scale, my=(in.y-oy)/scale;
        bool click=(in.pressed&SWOFTY_MOUSE_LEFT)!=0;
        auto inside=[&](float x,float y,float w,float h){return mx>=x && mx<x+w && my>=y && my<y+h;};
        auto rect=[&](float x,float y,float w,float h,std::uint32_t c){ov->rect(paint,ox+x*scale,oy+y*scale,w*scale,h*scale,c);};
        auto text=[&](float x,float y,const char* s,std::uint32_t c=0xffffffff){ov->text(paint,ox+x*scale,oy+y*scale,18*scale,c,s);};
        auto button=[&](float x,float y,float w,const char* label){rect(x,y,w,32,inside(x,y,w,32)?0x40526aff:0x27344aff);text(x+8,y+4,label);return click&&inside(x,y,w,32);};
        rect(0,0,1800,940,0x101722fa);
        text(20,16,"LAP REVIEW  |  X solid / Y dashed  |  throttle green / brake red");
        if(button(1670,12,110,"Close")) { close(ov); return; }
        text(20,60,"Completed laps - click X or Y");
        if(button(20,92,110,"Previous")) scroll_=std::max(0,scroll_-12);
        if(button(145,92,110,"Next")) scroll_=std::min(std::max(0,int(r.size())-12),scroll_+12);
        scroll_=std::clamp(scroll_,0,std::max(0,int(r.size())-1));
        if(in.wheel && inside(0,130,580,660)) scroll_=std::clamp(scroll_-int(in.wheel)*3,0,std::max(0,int(r.size())-12));
        for(int row=0;row<12 && std::size_t(scroll_+row)<r.size();++row) {
            int i=scroll_+row; const auto& l=r.lap(std::size_t(i)); float yy=138+row*52.f;
            char label[160];
            std::snprintf(label,sizeof label,"Lap %u   %u:%02u.%03u  %s%s%s",l.number,l.time_ms/60000,(l.time_ms/1000)%60,l.time_ms%1000,
                l.invalid?"INVALID ":"",l.partial?"PARTIAL ":"",l.limited?"NO TRACE":l.out?"OUT LAP":l.in?"IN LAP":"");
            if(!l.time_ms) std::snprintf(label,sizeof label,"Lap %u   time unknown  %s%s",l.number,l.partial?"PARTIAL ":"",l.limited?"NO TRACE":"");
            text(20,yy,label,x_==i?0xadd8ffff:0xe0e6edff);
            if(button(435,yy-3,52,x_==i?"X *":"X")) { x_=i; if(y_==x_) y_=-1; dirty_=true; }
            if(button(500,yy-3,52,y_==i?"Y *":"Y")) { y_=i==x_?-1:i; dirty_=true; }
        }
        if(!r.size()) text(20,160,!r.recording()?"Recording disabled.":!r.session_ready()?"Waiting for UDP session metadata.":"No completed laps recorded yet.");
        if(button(600,62,180,"Clear comparison")) { y_=-1; dirty_=true; }
        if(button(800,62,140,"Reset view")) { view_begin_=0; view_end_=r.length(); dirty_=true; }
        if(view_end_<=view_begin_ || view_end_>r.length()) { view_begin_=0; view_end_=r.length(); dirty_=true; }
        char selection[160];
        unsigned xn=x_>=0 && std::size_t(x_)<r.size()?r.lap(std::size_t(x_)).number:0;
        unsigned yn=y_>=0 && std::size_t(y_)<r.size()?r.lap(std::size_t(y_)).number:0;
        if(yn) std::snprintf(selection,sizeof selection,"X: lap %u   /   Y: lap %u",xn,yn);
        else std::snprintf(selection,sizeof selection,"X: lap %u   /   Y: none",xn);
        text(1000,68,selection);
        const float gx=620,gw=1130,gy=170,gh=260,gy2=510;
        bool over=inside(gx,gy,gw,gy2+gh-gy);
        float fraction=std::clamp((mx-gx)/gw,0.f,1.f);
        if(in.wheel && over && r.length()>0) {
            float span=view_end_-view_begin_, centre=view_begin_+fraction*span;
            span=std::clamp(span*(in.wheel>0?.8f:1.25f),std::min(20.f,r.length()),r.length());
            view_begin_=std::clamp(centre-fraction*span,0.f,r.length()-span); view_end_=view_begin_+span; dirty_=true;
        }
        if(click && over) { dragging_=true; drag_x_=mx; drag_begin_=view_begin_; }
        if(!(in.buttons&SWOFTY_MOUSE_LEFT)) dragging_=false;
        if(dragging_) {
            float span=view_end_-view_begin_;
            view_begin_=std::clamp(drag_begin_-(mx-drag_x_)/gw*span,0.f,std::max(0.f,r.length()-span));
            view_end_=view_begin_+span; dirty_=true;
        }
        if(dirty_ || cache_x_!=x_ || cache_y_!=y_ || cache_count_!=r.size()) {
            plot_.build(r,x_,y_,view_begin_,view_end_); dirty_=false;
            cache_x_=x_; cache_y_=y_; cache_count_=r.size();
        }
        for(float top:{gy,gy2}) {
            rect(gx,top,gw,gh,0x080d15ff);
            for(int i=0;i<3;++i) ov->line(paint,ox+gx*scale,oy+(top+i*gh/2)*scale,ox+(gx+gw)*scale,oy+(top+i*gh/2)*scale,scale,0xffffff28);
        }
        text(gx,gy-28,"THROTTLE  0-100%"); text(gx,gy2-28,"BRAKE  0-100%");
        // 4 * 512 v1 line segments = at most 12,288 vertices, below the 16,384 budget.
        for(unsigned s=0;s<4;++s) for(unsigned c=0;c<Plot::columns;++c) {
            const auto& v=plot_.series[s][c];
            if(!v.valid || v.broken || (s>=2 && (c/4)%2)) continue;
            float xx=gx+gw*(float(c)+.5f)/Plot::columns, top=s%2?gy2:gy;
            float y0=top+gh*(1-v.hi), y1=top+gh*(1-v.lo);
            // A constant envelope still has a visible 2px marker.
            ov->line(paint,ox+xx*scale,oy+y0*scale,ox+xx*scale,oy+std::min(top+gh,y1+2)*scale,
                     2*scale,s%2?0xff4545ff:0x32e875ff);
        }
        char info[200];
        std::snprintf(info,sizeof info,"%.0f - %.0f m  |  wheel: zoom  drag: pan  |  2 m min/max bins",view_begin_,view_end_); text(gx,810,info);
        if(over) {
            float d=view_begin_+fraction*(view_end_-view_begin_);
            auto bi=std::size_t(std::max(0.f,d/2));
            for(int which=0;which<2;++which) {
                int sel=which?y_:x_; const Bin* b=sel>=0 && std::size_t(sel)<r.size()?r.bin(r.lap(std::size_t(sel)),bi):nullptr;
                if(b) std::snprintf(info,sizeof info,"%c @ %.0f m: throttle %.0f-%.0f%%   brake %.0f-%.0f%%",which?'Y':'X',d,b->throttle_min*100,b->throttle_max*100,b->brake_min*100,b->brake_max*100);
                else std::snprintf(info,sizeof info,"%c @ %.0f m: -- (no coverage)",which?'Y':'X',d);
                text(gx,850+which*28.f,info);
            }
        }
        text(20,810,"F8 / Escape to close. Game pause unchanged.");
        text(20,842,"Controllers/wheels still reach the game.");
        text(20,874,r.metadata_limit()?"Lap metadata limit reached.":"After 128 traces, later laps retain timing only.");
        (void)now;
    }
private:
    static bool key(const SwoftyInput& i,int vk) { return vk>0 && vk<256 && (i.keys_pressed[vk/8]&(1u<<(vk%8))); }
    Plot plot_{};
    unsigned generation_{};
    std::size_t cache_count_{};
    int x_{-1},y_{-1},cache_x_{-2},cache_y_{-2},scroll_{};
    float view_begin_{},view_end_{},drag_x_{},drag_begin_{};
    bool open_{},dragging_{},dirty_{true};
};
}

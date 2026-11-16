#pragma once
#include "core/review_plot.hpp"
#include "core/multi_review.hpp"
// Included by the Windows adapter. All methods are called by its render owner.
namespace input_trace::review {
class Panel {
public:
    void close(const SwoftyOverlay* ov) {
        if(open_) ov->focus(0);
        open_=false; dragging_=false;
    }
    bool open() const { return open_; }
    template<class Source>
    void paint(const SwoftyApi* api,SwoftyPaint* paint,const Source& r,
               double now,bool safe,bool active_window,int hotkey,bool supported) {
        if(!supported) return; // Never access v3 fields without runtime feature detection.
        auto ov=api->overlay;
        if(generation_!=r.generation()) { close(ov); generation_=r.generation(); x_=y_=-1; xd_=yd_=r.player_index(); scroll_=0; view_end_=0; dirty_=true; }
        if(!safe || !active_window || !r.available()) { close(ov); return; }
        SwoftyInput in{}; in.size=sizeof in;
        if(ov->input(paint,&in)!=SWOFTY_ASSET_OK) { close(ov); return; }
        if(open_ && !in.focused) { close(ov); return; } // Another addon owns focus; never reclaim it.
        if(key(in,hotkey)) {
            if(open_) { close(ov); return; }
            int result=ov->focus(1);
            if(result!=SWOFTY_ASSET_OK) { ov->focus(0); return; }
            open_=true;
            if(x_<0 && r.driver(xd_).size()) x_=int(r.driver(xd_).size()-1);
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
        text(20,16,"LAP REVIEW  |  X cyan / Y amber  |  distance aligned");
        if(button(1670,12,110,"Close")) { close(ov); return; }
        auto next_driver=[&](unsigned current) {
            for(unsigned n=0;n<r.driver_count();++n) if(r.driver_index(n)==current)
                return r.driver_index((n+1)%r.driver_count());
            return r.player_index();
        };
        char driver_label[80];
        std::snprintf(driver_label,sizeof driver_label,"X > %02u %.16s",xd_+1,r.driver_name(xd_));
        if(button(20,52,270,driver_label)) {xd_=next_driver(xd_);x_=-1;browse_y_=false;scroll_=0;dirty_=true;}
        std::snprintf(driver_label,sizeof driver_label,"Y > %02u %.16s",yd_+1,r.driver_name(yd_));
        if(button(300,52,270,driver_label)) {yd_=next_driver(yd_);y_=-1;browse_y_=true;scroll_=0;dirty_=true;}
        if(button(20,92,240,browse_y_?"Laps: Y driver":"Laps: X driver")) {browse_y_=!browse_y_;scroll_=0;}
        const unsigned browsed=browse_y_?yd_:xd_;
        const auto& list=r.driver(browsed);
        if(button(280,92,135,"Previous")) scroll_=std::max(0,scroll_-12);
        if(button(430,92,135,"Next")) scroll_=std::min(std::max(0,int(list.size())-12),scroll_+12);
        scroll_=std::clamp(scroll_,0,std::max(0,int(list.size())-1));
        if(in.wheel && inside(0,130,580,630)) scroll_=std::clamp(scroll_-int(in.wheel)*3,0,std::max(0,int(list.size())-12));
        for(int row=0;row<12 && std::size_t(scroll_+row)<list.size();++row) {
            int i=scroll_+row; const auto& l=list.lap(std::size_t(i)); float yy=138+row*52.f;
            const bool isx=xd_==browsed && x_==i,isy=yd_==browsed && y_==i;
            char label[96];
            rect(12,yy-5,555,48,(isx || isy)?0x253246ff:0x151f2dff);
            if(l.time_ms) std::snprintf(label,sizeof label,"Lap %u   %u:%02u.%03u",l.number,l.time_ms/60000,(l.time_ms/1000)%60,l.time_ms%1000);
            else std::snprintf(label,sizeof label,"Lap %u   time unknown",l.number);
            text(20,yy,label,isx?0x55d9ffff:isy?0xffbd59ff:0xe0e6edff);
            std::snprintf(label,sizeof label,"%s%s%s",l.invalid?"INVALID ":"",l.partial?"PARTIAL ":"",l.limited?"NO TRACE":l.out?"OUT LAP":l.in?"IN LAP":"");
            ov->text(paint,ox+20*scale,oy+(yy+23)*scale,14*scale,0x9eabc0ff,label);
            if(button(435,yy-3,52,isx?"X *":"X")) { xd_=browsed;x_=i;dirty_=true; }
            if(button(500,yy-3,52,isy?"Y *":"Y")) { yd_=browsed;y_=i;dirty_=true; }
        }
        if(!list.size()) text(20,160,!r.recording()?"Recording disabled.":!r.session_ready()?"Waiting for UDP session metadata.":"No completed laps for this driver yet.");
        const auto& rx=r.driver(xd_);const auto& ry=r.driver(yd_);
        if(button(600,62,180,"Clear comparison")) { y_=-1; dirty_=true; }
        if(button(965,62,200,split_?"View: separate":"View: overlay")) split_=!split_;
        if(button(800,62,140,"Reset view")) { view_begin_=0; view_end_=r.length(); dirty_=true; }
        if(view_end_<=view_begin_ || view_end_>r.length()) { view_begin_=0; view_end_=r.length(); dirty_=true; }
        char selection[160];
        unsigned xn=x_>=0 && std::size_t(x_)<rx.size()?rx.lap(std::size_t(x_)).number:0;
        unsigned yn=y_>=0 && std::size_t(y_)<ry.size()?ry.lap(std::size_t(y_)).number:0;
        std::snprintf(selection,sizeof selection,"X: %.16s / lap %u",r.driver_name(xd_),xn);text(1200,52,selection);
        if(yn) std::snprintf(selection,sizeof selection,"Y: %.16s / lap %u",r.driver_name(yd_),yn);
        else std::snprintf(selection,sizeof selection,"Y: %.16s / no lap",r.driver_name(yd_));
        text(1200,76,selection);
        const char* pages[]={"Pedals","Speed / Gear","ERS","Wheel slip","Aero height"};
        for(int i=0;i<5;++i) {
            char tab[40];std::snprintf(tab,sizeof tab,"%s%s",page_==i?"> ":"",pages[i]);
            if(button(600+i*230.f,104,220,tab)) {page_=i;dirty_=true;}
        }
        if(page_==3 && button(20,770,250,wheel_names_[wheel_])) {wheel_=(wheel_+1)%4;dirty_=true;}
        int channel0=-1,channel1=-1;
        const char* title0="THROTTLE (%)",*title1="BRAKE (%)";
        if(page_==1) {channel0=0;channel1=1;title0="SPEED (km/h)";title1="GEAR (-1 reverse, 0 neutral)";}
        if(page_==2) {channel0=2;channel1=3;title0="MGU-K POWER (kW)";title1="ENERGY DEPLOYED THIS LAP (MJ)";}
        if(page_==3) {channel0=4+wheel_;channel1=8+wheel_;title0="WHEEL SLIP RATIO (signed)";title1="WHEEL SLIP ANGLE (raw)";}
        if(page_==4) {channel0=12;channel1=13;title0="FRONT PLANK HEIGHT (raw)";title1="REAR PLANK HEIGHT (raw)";}
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
        if(dirty_ || cache_x_!=x_ || cache_y_!=y_ || cache_count_!=rx.size()+ry.size() || cache_xd_!=xd_ || cache_yd_!=yd_) {
            plot_.build(rx,x_,ry,y_,view_begin_,view_end_,channel0,channel1); dirty_=false;
            cache_x_=x_; cache_y_=y_; cache_count_=rx.size()+ry.size();cache_xd_=xd_;cache_yd_=yd_;
        }
        for(unsigned s=0;s<4;++s) {
            if(s>=2 && !split_) continue;
            float top=(s%2?gy2:gy)+(split_ && s>=2?gh/2+12:0);
            float lane=split_?gh/2-12:gh;
            rect(gx,top,gw,lane,0x080d15ff);
            for(int i=0;i<3;++i) ov->line(paint,ox+gx*scale,oy+(top+i*lane/2)*scale,ox+(gx+gw)*scale,oy+(top+i*lane/2)*scale,scale,0xffffff20);
            text(gx-30,top,split_?(s<2?"X":"Y"):(page_==0?"%":""));
        }
        char axis[128];
        auto norm=[&](float value,unsigned channel) {return std::clamp((value-plot_.low[channel])/(plot_.high[channel]-plot_.low[channel]),0.f,1.f);};
        for(unsigned c=0;c<2;++c) {
            float factor=page_==0?100.f:1.f;
            std::snprintf(axis,sizeof axis,"%s   %.3f to %.3f",c?title1:title0,plot_.low[c]*factor,plot_.high[c]*factor);
            text(gx,(c?gy2:gy)-28,axis);
        }
        // At most 4 * 176 * 2 lines = 8,448 trace vertices. Same bounded
        // pool and no allocation; cache rebuilt only on selection/view changes.
        for(unsigned s=0;s<4;++s) for(unsigned c=0;c<Plot::columns;++c) {
            const auto& v=plot_.series[s][c];
            if(!v.valid) continue;
            const auto colour=s<2?0x55d9ffffu:0xffbd59ffu;
            float top=(s%2?gy2:gy)+(split_ && s>=2?gh/2+12:0);
            float lane=split_?gh/2-12:gh;
            float xx=gx+gw*(float(c)+.5f)/Plot::columns;
            if(v.previous>=0 && !v.broken) {
                const auto& prev=plot_.series[s][v.previous];
                if(!prev.broken) {
                    float px=gx+gw*(float(v.previous)+.5f)/Plot::columns;
                    ov->line(paint,ox+px*scale,oy+(top+lane*(1-norm(prev.last,s%2)))*scale,
                             ox+xx*scale,oy+(top+lane*(1-norm(channel1==1 && s%2?prev.last:v.first,s%2)))*scale,2*scale,colour);
                }
            }
            // Extrema preserve brief pedal spikes; connections follow the upper
            // envelope, with no smoothing or interpolation over an input gap.
            float lo=v.lo,hi=v.hi;
            if(channel1==1 && s%2 && v.previous>=0 && !v.broken && !plot_.series[s][v.previous].broken) {
                lo=std::min(lo,plot_.series[s][v.previous].last);hi=std::max(hi,plot_.series[s][v.previous].last);
            }
            float y0=top+lane*(1-norm(hi,s%2)), y1=top+lane*(1-norm(lo,s%2));
            ov->line(paint,ox+xx*scale,oy+y0*scale,ox+xx*scale,oy+y1*scale,2*scale,colour);
        }
        // Sector locations come from Session data, never inferred from thirds.
        for(unsigned sector=0;sector<2;++sector) {
            float distance=r.sector(sector);
            if(distance<=0 || distance<view_begin_ || distance>view_end_) continue;
            float xx=gx+gw*(distance-view_begin_)/(view_end_-view_begin_);
            for(float top:{gy,gy2}) for(float dy=0;dy<gh;dy+=16)
                ov->line(paint,ox+xx*scale,oy+(top+dy)*scale,ox+xx*scale,oy+(top+std::min(dy+8,gh))*scale,scale,0xffffff80);
            text(xx+4,gy+4,sector?"S3":"S2");
        }
        char info[200];
        for(int tick=0;tick<=4;++tick) {
            const float xx=gx+gw*float(tick)/4;
            std::snprintf(info,sizeof info,"%.0f m",view_begin_+(view_end_-view_begin_)*float(tick)/4);
            ov->text(paint,ox+(xx-(tick==4?65:0))*scale,oy+(gy2+gh+8)*scale,14*scale,0x9eabc0ff,info);
        }
        std::snprintf(info,sizeof info,"%.0f - %.0f m | wheel: zoom  drag: pan | %s",view_begin_,view_end_,page_==0?"2 m bins":"16 m min/max bins"); text(gx,810,info);
        if(over) {
            for(float top:{gy,gy2}) ov->line(paint,ox+(gx+fraction*gw)*scale,oy+top*scale,
                ox+(gx+fraction*gw)*scale,oy+(top+gh)*scale,scale,0xffffff70);
            float d=view_begin_+fraction*(view_end_-view_begin_);
            auto bi=std::size_t(std::max(0.f,d/2));
            for(int which=0;which<2;++which) {
                const auto& selected=which?ry:rx;
                int sel=which?y_:x_;
                if(page_==0) {
                    const Bin* b=sel>=0 && std::size_t(sel)<selected.size()?selected.bin(selected.lap(std::size_t(sel)),bi):nullptr;
                    if(b) std::snprintf(info,sizeof info,"%c @ %.0f m: throttle %.0f-%.0f%%  brake %.0f-%.0f%%",which?'Y':'X',d,b->throttle_min*100,b->throttle_max*100,b->brake_min*100,b->brake_max*100);
                    else std::snprintf(info,sizeof info,"%c @ %.0f m: -- (no coverage)",which?'Y':'X',d);
                } else {
                    auto* a=sel>=0 && std::size_t(sel)<selected.size()?selected.extra(selected.lap(std::size_t(sel)),unsigned(channel0),unsigned(d/16)):nullptr;
                    auto* b=sel>=0 && std::size_t(sel)<selected.size()?selected.extra(selected.lap(std::size_t(sel)),unsigned(channel1),unsigned(d/16)):nullptr;
                    char av[48]="--",bv[48]="--";
                    if(a) std::snprintf(av,sizeof av,"%.4f to %.4f",a->lo,a->hi);
                    if(b) std::snprintf(bv,sizeof bv,"%.4f to %.4f",b->lo,b->hi);
                    std::snprintf(info,sizeof info,"%c @ %.0f m: upper %s | lower %s",which?'Y':'X',d,av,bv);
                }
                text(gx,850+which*28.f,info);
            }
        }
        for(unsigned which=0;which<2;++which) {
            const char* hint=r.driver_hint(which?yd_:xd_,page_);
            if(*hint) {
                char note[128];std::snprintf(note,sizeof note,"%c: %s",which?'Y':'X',hint);
                ov->text(paint,ox+20*scale,oy+(808+which*18)*scale,14*scale,0x9eabc0ff,note);
            }
        }
        text(20,850,"F8 / Escape to close. Garage or paused session.");
        text(20,878,"Controllers/wheels still reach the game.");
        text(20,906,r.metadata_limit()?"Lap metadata limit reached.":"128 shared trace slots; later laps: timing only.");
        (void)now;
    }
private:
    static bool key(const SwoftyInput& i,int vk) { return vk>0 && vk<256 && (i.keys_pressed[vk/8]&(1u<<(vk%8))); }
    static constexpr const char* wheel_names_[4]={"Wheel: rear left","Wheel: rear right","Wheel: front left","Wheel: front right"};
    int page_{},wheel_{};
    Plot plot_{};
    unsigned generation_{},xd_{},yd_{},cache_xd_{},cache_yd_{};
    bool browse_y_{};
    std::size_t cache_count_{};
    int x_{-1},y_{-1},cache_x_{-2},cache_y_{-2},scroll_{};
    float view_begin_{},view_end_{},drag_x_{},drag_begin_{};
    bool open_{},dragging_{},dirty_{true},split_{true};
};
}

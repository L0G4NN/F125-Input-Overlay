#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <swofty_api.h>
#include "core/udp.hpp"
#include "core/layout.hpp"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>

namespace {
const SwoftyApi* api{};
std::atomic<bool> enabled{true}, bars{true}, paused{false}, reset_requested{false};
std::atomic<int> history{5}, width{420}, height{160}, opacity{25}, anchor{0}, horizontal{0}, vertical{0};
SOCKET sock=INVALID_SOCKET;
input_trace::Feed feed;
bool attempted{}, socket_error{}, was_enabled=true;
void settings(const char* id,const SwoftySettingValue* v,void*) {
    if(!v) return;
    if(v->type==SWOFTY_SETTING_TOGGLE) {
        if(std::strcmp(id,"enabled")==0) enabled=v->toggle!=0;
        if(std::strcmp(id,"bars")==0) bars=v->toggle!=0;
    } else if(v->type==SWOFTY_SETTING_SLIDER && std::isfinite(v->slider)) {
        auto value=[&](int lo,int hi){return int(std::clamp(v->slider,double(lo),double(hi)));};
        if(std::strcmp(id,"history")==0) history=value(2,15);
        if(std::strcmp(id,"width")==0) width=value(280,900);
        if(std::strcmp(id,"height")==0) height=value(120,360);
        if(std::strcmp(id,"opacity")==0) opacity=value(0,100);
        if(std::strcmp(id,"anchor")==0) anchor=value(0,3);
        if(std::strcmp(id,"horizontal")==0) horizontal=value(0,100);
        if(std::strcmp(id,"vertical")==0) vertical=value(0,100);
    }
}
void reset_event(const SwoftyEvent*,void*) { reset_requested=true; paused=false; }
void pause_event(const SwoftyEvent* e,void*) {
    paused=std::strcmp(e->name,SWOFTY_EVENT_PAUSE)==0;
}
void open_socket() {
    attempted=true;
    WSADATA w{};
    if(WSAStartup(MAKEWORD(2,2),&w)!=0) {socket_error=true; return;}
    sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    BOOL exclusive=TRUE;
    u_long nonblock=1;
    sockaddr_in address{}; address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); address.sin_port=htons(20777);
    if(sock==INVALID_SOCKET || setsockopt(sock,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,
       reinterpret_cast<const char*>(&exclusive),sizeof exclusive)==SOCKET_ERROR ||
       ioctlsocket(sock,FIONBIO,&nonblock)==SOCKET_ERROR ||
       bind(sock,reinterpret_cast<sockaddr*>(&address),sizeof address)==SOCKET_ERROR) {
        api->log("[input-overlay] UDP bind failed (%d). Close other listeners on port 20777 and restart.",WSAGetLastError());
        if(sock!=INVALID_SOCKET) closesocket(sock);
        sock=INVALID_SOCKET; WSACleanup(); socket_error=true;
    }
}
void paint(SwoftyPaint* p,void*) {
    if(reset_requested.exchange(false)) feed.clear();
    bool on=enabled.load();
    if(!on) {
        if(sock!=INVALID_SOCKET) { closesocket(sock); sock=INVALID_SOCKET; WSACleanup(); }
        attempted=false; socket_error=false; feed.clear(); was_enabled=false; return;
    }
    if(!attempted) open_socket();
    if(on!=was_enabled) {feed.clear(); was_enabled=on;}
    feed.trace.set_history(history.load());
    if(paused.load()) feed.trace.pause(); else feed.trace.resume();
    const double now=double(GetTickCount64())/1000;
    if(sock!=INVALID_SOCKET) {
        unsigned char packet[2048];
        // Fixed receive budget: no blocking or unbounded drain in render callback.
        for(int n=0;n<128;++n) {
            int got=recv(sock,reinterpret_cast<char*>(packet),sizeof packet,0);
            if(got==SOCKET_ERROR) break;
            if(on && !paused.load()) feed.accept({packet,std::size_t(got)},now);
        }
    }
    if(!on) return;
    feed.tick(now);
    float sw=0,sh=0; auto ov=api->overlay; ov->size(p,&sw,&sh);
    if(sw<1 || sh<1) return;
    const auto panel=input_trace::layout(sw,sh,width.load(),height.load(),anchor.load(),horizontal.load(),vertical.load());
    const float x=panel.x,y=panel.y,w=panel.w,h=panel.h,scale=panel.scale;
    ov->rect(p,x,y,w,h,std::uint32_t(opacity.load()*255/100));
    const float gx=x+36*scale, gy=y+32*scale, gw=w-100*scale, gh=h-52*scale;
    for(int i=0;i<3;++i) {
        float yy=gy+gh*i/2;
        ov->line(p,gx,yy,gx+gw,yy,scale,0xffffff30);
    }
    ov->text(p,x+4*scale,gy,12*scale,0xffffffb0,"100");
    ov->text(p,x+12*scale,gy+gh-12*scale,12*scale,0xffffffb0,"0");
    const char* status=socket_error?"UDP port 20777 unavailable":
        paused.load()?"Paused":feed.trace.state()==input_trace::State::live?"Throttle / Brake":
        "Waiting for UDP 2025 :20777";
    ov->text(p,x+8*scale,y+6*scale,14*scale,0xffffffff,status);
    for(std::size_t i=1;i<feed.trace.size();++i) {
        if(!feed.trace[i].connected) continue;
        for(bool brake:{false,true}) {
            auto a=feed.trace.point(feed.trace[i-1],brake,gw,gh), b=feed.trace.point(feed.trace[i],brake,gw,gh);
            ov->line(p,gx+float(a.x),gy+float(a.y),gx+float(b.x),gy+float(b.y),2*scale,brake?0xff4545ff:0x32e875ff);
        }
    }
    auto last=feed.trace.latest();
    for(int i=0;i<2;++i) {
        const float bx=gx+gw+(8+26*i)*scale;
        const auto colour=i?0xff4545ff:0x32e875ff;
        const float value=last?(i?last->brake:last->throttle):0;
        if(bars.load()) {
            ov->rect(p,bx,gy,16*scale,gh,0xffffff18);
            if(last) ov->rect(p,bx,gy+gh*(1-value),16*scale,gh*value,colour);
        }
        char label[16];
        if(last) std::snprintf(label,sizeof label,"%d",int(std::lround(value*100)));
        else std::snprintf(label,sizeof label,"--");
        ov->text(p,bx,gy+gh+3*scale,11*scale,colour,label);
    }
}
bool registered=false;
void register_overlay(double,void*) {
    if(!registered && api->overlay->available()) {
        registered=api->overlay->on_paint(paint,nullptr,nullptr)==SWOFTY_ASSET_OK;
        if(registered) api->log("[input-overlay] overlay registered");
    }
}
void start() {
    const char* names[]={"enabled","bars","history","width","height","opacity","anchor","horizontal","vertical"};
    for(auto id:names) {
        const unsigned type=(std::strcmp(id,"enabled")==0 || std::strcmp(id,"bars")==0)?SWOFTY_SETTING_TOGGLE:SWOFTY_SETTING_SLIDER;
        SwoftySettingValue v{};
        if(api->setting && api->setting(id,type,&v)) settings(id,&v,nullptr);
        if(api->watch_setting) api->watch_setting(id,settings,nullptr);
    }
    if(api->events) {
        api->events->subscribe(SWOFTY_EVENT_SESSION_START,reset_event,nullptr,nullptr);
        api->events->subscribe(SWOFTY_EVENT_SESSION_END,reset_event,nullptr,nullptr);
        api->events->subscribe(SWOFTY_EVENT_PAUSE,pause_event,nullptr,nullptr);
        api->events->subscribe(SWOFTY_EVENT_RESUME,pause_event,nullptr,nullptr);
    }
    int result=api->events ? api->events->on_frame(register_overlay,nullptr,nullptr) : SWOFTY_ASSET_UNAVAILABLE;
    api->log("[input-overlay] overlay registration %d; enable UDP 2025 to 127.0.0.1:20777",result);
}
}
extern "C" __declspec(dllexport) const uint32_t SwoftyAddonApi=5;
extern "C" __declspec(dllexport) int SwoftyAddonInit(const SwoftyApi* a) {
    if(!a || a->version<5 || !a->overlay || !a->register_mixins) return 0;
    api=a;
    static const SwoftyMixin load={nullptr,SWOFTY_MIXIN_ON_LOAD,0,reinterpret_cast<void*>(&start),nullptr};
    return a->register_mixins(&load,1);
}

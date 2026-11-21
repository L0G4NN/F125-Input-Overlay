#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <swofty_api.h>
#include "core/udp.hpp"
#include "core/layout.hpp"
#include "core/hud_visibility.hpp"
#include "core/receive_gate.hpp"
#include "review_panel.hpp"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>

namespace {
const SwoftyApi* api{};
std::atomic<bool> enabled{true}, bars{true}, paused{false}, reset_requested{false}, recording{true}, hide_in_menus{true};
std::atomic<int> history{5}, width{420}, height{160}, opacity{25}, anchor{0}, horizontal{0}, vertical{0}, review_key{6};
SOCKET sock=INVALID_SOCKET;
input_trace::Feed feed;
input_trace::ReceiveGate receive_gate;
input_trace::review::Recorder recorder;
input_trace::review::Panel review_panel;
bool review_supported{};
using PauseQuery=std::uint32_t (*)();
PauseQuery pause_query{};
bool attempted{}, socket_error{}, was_enabled=true;
void settings(const char* id,const SwoftySettingValue* v,void*) {
    if(!v) return;
    if(v->type==SWOFTY_SETTING_TOGGLE) {
        if(std::strcmp(id,"enabled")==0) enabled=v->toggle!=0;
        if(std::strcmp(id,"bars")==0) bars=v->toggle!=0;
        if(std::strcmp(id,"recording")==0) recording=v->toggle!=0;
        if(std::strcmp(id,"hide_in_menus")==0) hide_in_menus=v->toggle!=0;
    } else if(v->type==SWOFTY_SETTING_SLIDER && std::isfinite(v->slider)) {
        auto value=[&](int lo,int hi){return int(std::clamp(v->slider,double(lo),double(hi)));};
        if(std::strcmp(id,"review_key")==0) review_key=value(6,12);
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
    const bool reset=reset_requested.exchange(false);
    if(reset) { feed.clear(); recorder.reset(); if(review_supported) review_panel.close(api->overlay); }
    bool on=enabled.load();
    if(!on) {
        if(sock!=INVALID_SOCKET) { closesocket(sock); sock=INVALID_SOCKET; WSACleanup(); }
        attempted=false; socket_error=false; feed.clear();
        recorder.set_recording(false); recorder.gap();
        if(review_supported) review_panel.close(api->overlay);
        was_enabled=false; return;
    }
    if(!attempted) open_socket();
    if(on!=was_enabled) {feed.clear(); recorder.reset(); receive_gate=input_trace::ReceiveGate{}; was_enabled=on;}
    recorder.set_recording(recording.load());
    feed.trace.set_history(history.load());
    const bool is_paused=pause_query?pause_query()!=0:paused.load();
    if(is_paused) feed.trace.pause(); else feed.trace.resume();
    const double now=double(GetTickCount64())/1000;
    if(receive_gate.begin(now,reset,is_paused)) { recorder.gap(); if(!is_paused) feed.trace.unavailable(); }
    if(sock!=INVALID_SOCKET) {
        unsigned char packet[2048];
        bool drained=false;
        // Fixed receive budget: no blocking or unbounded drain in render callback.
        for(int n=0;n<128;++n) {
            int got=recv(sock,reinterpret_cast<char*>(packet),sizeof packet,0);
            if(got==SOCKET_ERROR) {
                const int error=WSAGetLastError();
                if(error==WSAEMSGSIZE) { feed.trace.unavailable(); recorder.gap(); continue; }
                if(error==WSAEWOULDBLOCK) { receive_gate.drained(); drained=true; }
                else { receive_gate.exhausted(); feed.trace.unavailable(); recorder.gap(); }
                break;
            }
            if(!is_paused && receive_gate.accept()) {
                const std::span<const unsigned char> bytes{packet,std::size_t(got)};
                feed.accept(bytes,now); recorder.accept(bytes,now);
            }
        }
        if(!drained) { receive_gate.exhausted(); feed.trace.unavailable(); recorder.gap(); }
    }
    if(!on) return;
    feed.tick(now);
    float sw=0,sh=0; auto ov=api->overlay; ov->size(p,&sw,&sh);
    if(sw<1 || sh<1) { if(review_supported) review_panel.close(ov); return; }
    const bool review_safe=input_trace::review_interactive(is_paused,recorder.garage(now));
    DWORD foreground_pid=0;
    if(review_supported && review_safe) GetWindowThreadProcessId(GetForegroundWindow(),&foreground_pid);
    review_panel.paint(api,p,recorder,now,review_safe,
                       foreground_pid==GetCurrentProcessId(),VK_F1+review_key.load()-1,review_supported);
    if(review_panel.open()) return;
    if(!input_trace::show_live_hud(hide_in_menus.load(),is_paused,recorder.on_track(now))) return;
    const auto panel=input_trace::layout(sw,sh,width.load(),height.load(),anchor.load(),horizontal.load(),vertical.load());
    const float x=panel.x,y=panel.y,w=panel.w,h=panel.h,scale=panel.scale;
    ov->rect(p,x,y,w,h,std::uint32_t(opacity.load()*255/100));
    const float gx=x+36*scale, gy=y+48*scale, gw=w-100*scale, gh=h-68*scale;
    for(int i=0;i<3;++i) {
        float yy=gy+gh*i/2;
        ov->line(p,gx,yy,gx+gw,yy,scale,0xffffff30);
    }
    ov->text(p,x+4*scale,gy,12*scale,0xffffffb0,"100");
    ov->text(p,x+12*scale,gy+gh-12*scale,12*scale,0xffffffb0,"0");
    const char* status=socket_error?"UDP port 20777 unavailable":
        is_paused?"Paused":feed.trace.state()==input_trace::State::live?"Throttle / Brake":
        "Waiting for UDP 2025 :20777";
    ov->text(p,x+8*scale,y+6*scale,14*scale,0xffffffff,status);
    float steering_angle=0;char steering_text[80];
    if(!is_paused && recorder.steering(now,steering_angle))
        std::snprintf(steering_text,sizeof steering_text,"Front-wheel angle: %+.1f deg",steering_angle);
    else std::snprintf(steering_text,sizeof steering_text,"Front-wheel angle: --");
    ov->text(p,x+8*scale,y+24*scale,12*scale,0xb7dfffff,steering_text);
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
    HMODULE core=GetModuleHandleW(L"swofty_core.dll");
    using VersionQuery=std::uint32_t (*)();
    auto version=core?std::bit_cast<VersionQuery>(GetProcAddress(core,"SwoftyCoreOverlayVersion")):nullptr;
    pause_query=core?std::bit_cast<PauseQuery>(GetProcAddress(core,"SwoftyCorePauseReasons")):nullptr;
    review_supported=version && version()>=3;
    if(review_supported && !recorder.initialize()) { review_supported=false; api->log("[input-overlay] review pool allocation failed; live overlay remains enabled"); }
    if(!review_supported) api->log("[input-overlay] lap review needs overlay input v3; live overlay remains enabled");
    const char* names[]={"enabled","bars","history","width","height","opacity","anchor","horizontal","vertical","recording","review_key","hide_in_menus"};
    for(auto id:names) {
        const unsigned type=(std::strcmp(id,"enabled")==0 || std::strcmp(id,"bars")==0 || std::strcmp(id,"recording")==0 || std::strcmp(id,"hide_in_menus")==0)?SWOFTY_SETTING_TOGGLE:SWOFTY_SETTING_SLIDER;
        SwoftySettingValue v{};
        if(api->setting && api->setting(id,type,&v)) settings(id,&v,nullptr);
        if(api->watch_setting) api->watch_setting(id,settings,nullptr);
    }
    api->log("[input-overlay] live HUD hidden in menus by default; requires fresh UDP lap data while driving");
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
    if(!a || a->version<5 || !a->overlay || !a->events || !a->register_mixins || !a->log) return 0;
    api=a;
    static const SwoftyMixin load={nullptr,SWOFTY_MIXIN_ON_LOAD,0,reinterpret_cast<void*>(&start),nullptr};
    return a->register_mixins(&load,1);
}

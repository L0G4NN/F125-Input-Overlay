#pragma once
#include <algorithm>
namespace input_trace {
struct Layout { float x,y,w,h,scale; };
inline Layout layout(float sw,float sh,int width,int height,int corner,int horizontal,int vertical) {
    if(sw<=0 || sh<=0) return {};
    const float scale=std::min(sw/1920,sh/1080);
    const float w=std::min(float(std::clamp(width,280,900))*scale,sw);
    const float h=std::min(float(std::clamp(height,120,360))*scale,sh);
    const float margin=std::max(0.f,std::min(24*scale,std::min((sw-w)/2,(sh-h)/2)));
    const float dx=std::max(0.f,sw-w-2*margin)*std::clamp(horizontal,0,100)/100;
    const float dy=std::max(0.f,sh-h-2*margin)*std::clamp(vertical,0,100)/100;
    return {(corner%2)?sw-w-margin-dx:margin+dx,
            (corner>=2)?margin+dy:sh-h-margin-dy,w,h,scale};
}
}

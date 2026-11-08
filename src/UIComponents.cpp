#include "UIComponents.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace ui {
namespace { constexpr int sizes[]={12,14,16,20,24,32,48}; Font fonts[7]{}; bool owned[7]{}; }
void init() {
    std::string path=std::string(GetApplicationDirectory())+"assets/DejaVuSans.ttf";
    for(int i=0;i<7;++i) {
        if(FileExists(path.c_str())) {
            fonts[i]=LoadFontEx(path.c_str(),sizes[i],nullptr,0);
            owned[i]=fonts[i].texture.id!=0 && fonts[i].texture.id!=GetFontDefault().texture.id;
            if(owned[i]) SetTextureFilter(fonts[i].texture,TEXTURE_FILTER_BILINEAR);
        }
        if(!owned[i]) fonts[i]=GetFontDefault();
    }
}
void shutdown() { for(int i=0;i<7;++i) { if(owned[i]) UnloadFont(fonts[i]); fonts[i]={}; owned[i]=false; } }
void label(const std::string& value,float x,float y,float size,Color color) {
    float pixels=std::max(12.0f,std::round(size)); int best=0; float diff=std::numeric_limits<float>::max();
    for(int i=0;i<7;++i) if(std::abs(sizes[i]-pixels)<diff) { diff=std::abs(sizes[i]-pixels); best=i; }
    DrawTextEx(fonts[best],value.c_str(),{std::round(x),std::round(y)},pixels,0.0f,color);
}
void box(Rectangle r,Color fill,Color border) { DrawRectangleRec(r,fill); DrawRectangleLinesEx(r,1,border); }
bool hit(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(),r); }
bool button(Rectangle r,const std::string& title,float scale,bool active) {
    bool hover=hit(r);
    box(r,active?Color{49,101,145,250}:hover?Color{70,101,131,245}:panelLight,active?cyan:line);
    if(active) DrawRectangle(static_cast<int>(r.x),static_cast<int>(r.y),3,static_cast<int>(r.height),orange);
    label(title,r.x+10*scale,r.y+(r.height-std::max(12.0f,14*scale))/2,14*scale,text);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
}

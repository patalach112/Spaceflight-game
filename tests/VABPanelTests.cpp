// Standalone with Raylib headers (no display required):
// g++ -std=c++20 -ffunction-sections -fdata-sections -Iinclude -I<raylib-src> src/Game.cpp src/PartCatalog.cpp src/VABSystem.cpp src/VABInteraction.cpp tests/VABPanelTests.cpp -Wl,--gc-sections -o panel-tests && ./panel-tests
#include "Renderer.hpp"
#include "VABInteraction.hpp"
#include "UIComponents.hpp"
#define private public
#include "Game.hpp"
#undef private
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdlib>
static Vector2 mouse{}; static bool pressed=false,released=false,down=false; static int key=0; static float wheel=0;
struct Tile { int id; Rectangle r; }; static std::vector<Tile> tiles; static std::vector<std::string> labels;
extern "C" {
int GetScreenWidth(void){return 900;} int GetScreenHeight(void){return 600;}
Vector2 GetMousePosition(void){return mouse;} Vector2 GetMouseDelta(void){return {};}
float GetMouseWheelMove(void){return wheel;} float GetFrameTime(void){return 1.0f/60;}
bool IsKeyPressed(int k){return key==k;} bool IsMouseButtonPressed(int b){return b==MOUSE_BUTTON_LEFT && pressed;}
bool IsMouseButtonReleased(int b){return b==MOUSE_BUTTON_LEFT && released;} bool IsMouseButtonDown(int b){return b==MOUSE_BUTTON_LEFT && down;}
bool CheckCollisionPointRec(Vector2 p,Rectangle r){return p.x>=r.x&&p.x<r.x+r.width&&p.y>=r.y&&p.y<r.y+r.height;}
void DrawRectangleRec(Rectangle,Color){} void DrawRectangle(int,int,int,int,Color){} void DrawRectangleLinesEx(Rectangle,float,Color){}
void DrawLineEx(Vector2,Vector2,float,Color){} void DrawCircleV(Vector2,float,Color){} void DrawCircleLinesV(Vector2,float,Color){}
void DrawCircleSector(Vector2,float,float,float,int,Color){} void BeginScissorMode(int,int,int,int){} void EndScissorMode(){}
}
namespace ui {
int pixelSize(float size){return std::max(12,(int)std::round(size));}
void label(const std::string& s,float,float,float,Color){labels.push_back(s);}
void box(Rectangle,Color,Color){} bool hit(Rectangle r){return CheckCollisionPointRec(mouse,r);}
bool button(Rectangle r,const std::string&,float,bool){return hit(r)&&pressed;}
}
Vector2 Renderer::toScreen(double x,double y,const Viewport& v){return {v.originX+(float)x*20*v.zoom,v.originY+(float)y*20*v.zoom};}
Vector2 Renderer::toGrid(Vector2 p,const Viewport& v){return {(p.x-v.originX)/(20*v.zoom),(p.y-v.originY)/(20*v.zoom)};}
void Renderer::workspace(const Viewport&,Rectangle){}
void Renderer::part(const PartDef&,double,double,int,const Viewport&,Color,bool,bool,bool){}
void Renderer::tilePart(const PartDef& d,Rectangle r,bool){tiles.push_back({d.id,r});}
void check(bool ok,const char* msg){if(!ok){std::cerr<<"FAIL: "<<msg<<"\n";std::exit(1);}}
void frame(Game& g,Vector2 p,bool press=false,bool release=false,bool hold=false,float scroll=0,int k=0){mouse=p;pressed=press;released=release;down=hold;wheel=scroll;key=k;tiles.clear();labels.clear();g.workshop();}
int main(){
 Game g; float s=600.0f/660.0f;
 CatalogTab tabs[]={CatalogTab::Basics,CatalogTab::Wide6,CatalogTab::Wide8,CatalogTab::Wide10,CatalogTab::Wide12,CatalogTab::Aerodynamics,CatalogTab::Fairings,CatalogTab::Structural,CatalogTab::Other};
 for(int i=0;i<9;++i){ frame(g,{20*s,(48+i*52+10)*s},true); check(g.category_==tabs[i],"all nine category tabs selectable"); int n=0; for(const auto& d:partCatalog()) n+=partInTab(d,tabs[i]); check((int)tiles.size()==n,"tab draws all matching parts"); }
 frame(g,{100*s,100*s},false,false,false,-100); check(g.catalogScroll_>0,"icon list scrolls");
 auto last=tiles.back(); float listBottom=600-10*s; check(last.r.y<listBottom && last.r.y+last.r.height>48*s,"last icon reachable after scrolling");
 // Scroll the Other catalog to the small solar panel and pick it up.
 int index=0; for(const auto& d:partCatalog()) if(partInTab(d,CatalogTab::Other)){if(d.id==74)break;++index;}
 g.catalogScroll_=std::max(0.0f,index*60*s-70*s);
 frame(g,{100*s,100*s}); Tile target{}; for(auto t:tiles)if(t.id==74)target=t;
 check(target.id==74,"solar panel icon in Other");
 Vector2 icon{target.r.x+target.r.width*.5f,target.r.y+target.r.height*.5f};
 frame(g,icon); check(std::find(labels.begin(),labels.end(),"SMALL SOLAR PANEL")!=labels.end(),"hover identifies the part");
 frame(g,icon,true,false,true); check(g.interaction_.active() && g.interaction_.partId()==74,"icon pickup starts drag");
 frame(g,{500,350},false,false,true,0,KEY_D); check(g.interaction_.deployed(),"D changes live ghost state");
 frame(g,{500,350},false,true); check(!g.interaction_.active() && g.vab_.parts().size()==1 && g.vab_.parts()[0].deployed,"release commits deployed panel");
 auto p=g.vab_.parts()[0]; Vector2 pos=Renderer::toScreen(p.position.x+1,p.position.y+1,g.view_);
 frame(g,pos,true,false,true); frame(g,pos,false,true); check(g.interaction_.selected()!=0,"click selects placed panel");
 frame(g,{500,350},false,false,false,0,KEY_D); check(!g.vab_.parts()[0].deployed,"D stows selected panel");
 float side=148*s,top=60*s; frame(g,{side+(14+336+30)*s,top+(12+20)*s},true); check(g.vab_.parts()[0].deployed,"toolbar toggles selected panel");
 std::cout<<"Nine tabs, scrolling, pickup, release and deployment UI passed\n";
}

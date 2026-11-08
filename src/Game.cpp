#include "Game.hpp"
#include "EngineEntry.hpp"
#include "UIComponents.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
namespace {
std::string number(double n, int digits = 2) { char b[64]; std::snprintf(b,sizeof(b),"%.*f",digits,n); return b; }
Rectangle rect(float x,float y,float w,float h) { return {x,y,w,h}; }
void header(const char* eyebrow, const char* title, float x, float y, float s) {
    ui::label(eyebrow,x,y,13*s,ui::cyan); ui::label(title,x,y+25*s,32*s,ui::text);
}
}
extern "C" int aegisRun() { Game game; game.run(); return 0; }
void Game::run() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1440,900,"AEGIS: Spaceflight Simulator");
    SetWindowMinSize(900,600); SetExitKey(KEY_NULL); SetTargetFPS(120);
    ui::init();
    InitAudioDevice(); if (IsAudioDeviceReady()) SetMasterVolume(settings_.volume);
    while (!quit_ && !WindowShouldClose()) { BeginDrawing(); ClearBackground(ui::background); update(); EndDrawing(); }
    if (IsAudioDeviceReady()) CloseAudioDevice();
    ui::shutdown();
    CloseWindow();
}
void Game::update() {
    switch (state_) { case GameState::MAIN_MENU: menu(); break; case GameState::SETTINGS: settings(); break; case GameState::VAB: workshop(); break; case GameState::FLIGHT: flight(); break; }
}
void Game::menu() {
    float w=static_cast<float>(GetScreenWidth()),h=static_cast<float>(GetScreenHeight()),s=std::min(settings_.uiScale,std::min(w/900.0f,h/660.0f));
    for (float x=0;x<w;x+=80) DrawLineV({x,0},{x,h},ui::grid);
    for (float y=0;y<h;y+=80) DrawLineV({0,y},{w,y},ui::grid);
    DrawRectangle(static_cast<int>(w*.62f),0,static_cast<int>(w*.38f),static_cast<int>(h),Color{19,30,48,255});
    float cx=w*.76f, cy=h*.50f;
    DrawCircleLines(static_cast<int>(cx),static_cast<int>(cy),130*s,ui::line);
    DrawCircleLines(static_cast<int>(cx),static_cast<int>(cy),190*s,ui::line);
    DrawLineEx({cx,cy-230*s},{cx,cy+230*s},1,Color{75,118,159,255});
    DrawLineEx({cx-230*s,cy},{cx+230*s,cy},1,ui::line);
    Renderer::tilePart(partById(0),rect(cx-45*s,cy-105*s,90*s,80*s));
    Renderer::tilePart(partById(1),rect(cx-45*s,cy-25*s,90*s,100*s));
    Renderer::tilePart(partById(2),rect(cx-45*s,cy+75*s,90*s,65*s));
    float x=std::max(55.0f,w*.095f), y=h*.19f;
    ui::label("AEGIS  /  FLIGHT SYSTEMS",x,y,15*s,ui::cyan);
    ui::label("AEGIS:",x,y+55*s,48*s,ui::text);
    ui::label("SPACEFLIGHT SIMULATOR",x,y+110*s,24*s,ui::text);
    ui::label("DESIGN  /  ASSEMBLE  /  EXPLORE",x,y+155*s,13*s,ui::muted);
    float bw=300*s, bh=49*s, by=y+220*s;
    if(ui::button(rect(x,by,bw,bh),"LAUNCH / VAB",s,true)) state_=GameState::VAB;
    if(ui::button(rect(x,by+60*s,bw,bh),"SETTINGS",s)) state_=GameState::SETTINGS;
    if(ui::button(rect(x,by+120*s,bw,bh),"QUIT",s)) quit_=true;
    ui::label("MILESTONE 01  /  VEHICLE ASSEMBLY",x,h-40*s,12*s,ui::muted);
}
void Game::settings() {
    float w=static_cast<float>(GetScreenWidth()),h=static_cast<float>(GetScreenHeight()),s=std::min(settings_.uiScale,std::min(w/900.0f,h/660.0f));
    float x=std::max(45.0f,(w-700*s)/2),y=60*s, row=60*s;
    header("SYSTEM CONFIGURATION","SETTINGS",x,y,s); y+=100*s;
    ui::label("DISPLAY MODE",x,y+15*s,15*s,ui::muted);
    if(ui::button(rect(x+300*s,y,260*s,43*s),settings_.fullscreen?"FULLSCREEN":"WINDOWED",s,settings_.fullscreen)) { ToggleFullscreen(); settings_.fullscreen=IsWindowFullscreen(); }
    y+=row;
    ui::label("VERTICAL SYNC",x,y+15*s,15*s,ui::muted);
    if(ui::button(rect(x+300*s,y,260*s,43*s),settings_.vsync?"ON":"OFF",s,settings_.vsync)) {
        settings_.vsync=!settings_.vsync;
        if(settings_.vsync) SetWindowState(FLAG_VSYNC_HINT); else ClearWindowState(FLAG_VSYNC_HINT);
    }
    y+=row;
    ui::label("INTERFACE SCALE",x,y+15*s,15*s,ui::muted);
    if(ui::button(rect(x+300*s,y,55*s,43*s),"-",s)) settings_.uiScale=std::max(.8f,settings_.uiScale-.1f);
    ui::label(number(settings_.uiScale*100,0)+"%",x+375*s,y+14*s,15*s);
    if(ui::button(rect(x+450*s,y,55*s,43*s),"+",s)) settings_.uiScale=std::min(1.3f,settings_.uiScale+.1f);
    y+=row;
    ui::label("MASTER VOLUME",x,y+15*s,15*s,ui::muted);
    if(ui::button(rect(x+300*s,y,55*s,43*s),"-",s)) settings_.volume=std::max(0.0f,settings_.volume-.1f);
    ui::label(number(settings_.volume*100,0)+"%",x+375*s,y+14*s,15*s);
    if(ui::button(rect(x+450*s,y,55*s,43*s),"+",s)) settings_.volume=std::min(1.0f,settings_.volume+.1f);
    if(IsAudioDeviceReady()) SetMasterVolume(settings_.volume);
    y+=row+25*s;
    if(ui::button(rect(x,y,200*s,45*s),"<  BACK",s) || IsKeyPressed(KEY_ESCAPE)) state_=GameState::MAIN_MENU;
    ui::label("DISPLAY CHANGES APPLY IMMEDIATELY",x,std::min(h-30*s,y+75*s),11*s,ui::muted);
}
void Game::workshop() {
    if(IsKeyPressed(KEY_ESCAPE)) {
        if(interaction_.active()) interaction_.cancel();
        else { state_=GameState::MAIN_MENU; return; }
    }
    bool rightCancel=interaction_.active() && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    if(rightCancel) interaction_.cancel();
    float w=static_cast<float>(GetScreenWidth()),h=static_cast<float>(GetScreenHeight());
    float s=std::min(settings_.uiScale,std::min(w/900.0f,h/660.0f));
    float side=280*s,rail=78*s,top=60*s,bottom=84*s;
    Rectangle area=rect(side,top,w-side,h-top-bottom);
    Rectangle list=rect(rail,97*s,side-rail,h-164*s);
    Rectangle tools=rect(side+14*s,top+12*s,332*s,42*s);
    Vector2 mouse=GetMousePosition(); bool inArea=CheckCollisionPointRec(mouse,area);
    bool overTools=CheckCollisionPointRec(mouse,tools);
    if(!viewInitialized_) { view_={side+area.width*.5f,top+area.height*.52f,1}; viewInitialized_=true; }
    if(inArea && !overTools && (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || (!interaction_.active() && !rightCancel && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)))) {
        Vector2 delta=GetMouseDelta(); view_.originX+=delta.x; view_.originY+=delta.y;
    }
    float wheel=GetMouseWheelMove();
    if(CheckCollisionPointRec(mouse,list) && wheel!=0) catalogScroll_=std::max(0.0f,catalogScroll_-wheel*42*s);
    else if(inArea && !overTools && wheel!=0) { zoomAnchor_=mouse; targetZoom_=std::clamp(targetZoom_*std::pow(1.15f,wheel),.25f,3.5f); }
    if(std::abs(targetZoom_-view_.zoom)>0.0001f) {
        Vector2 before=Renderer::toGrid(zoomAnchor_,view_);
        view_.zoom+=(targetZoom_-view_.zoom)*(1.0f-std::exp(-14.0f*std::min(GetFrameTime(),0.1f)));
        Vector2 after=Renderer::toScreen(before.x,before.y,view_); view_.originX+=zoomAnchor_.x-after.x; view_.originY+=zoomAnchor_.y-after.y;
    }
    if(IsKeyPressed(KEY_Q) && interaction_.rotate(-1) && interaction_.moving()) movedActually_=true;
    if(IsKeyPressed(KEY_E) && interaction_.rotate(1) && interaction_.moving()) movedActually_=true;
    Vector2 grid=Renderer::toGrid(mouse,view_);
    if(inArea && !overTools && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) interaction_.eraseSelectedOrAt(grid.x,grid.y);
    if(inArea && !overTools && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !interaction_.active()) {
        interaction_.beginMove(grid.x,grid.y); dragStart_=mouse; movedActually_=false;
    }
    if(interaction_.moving() && (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT))) {
        float dx=mouse.x-dragStart_.x,dy=mouse.y-dragStart_.y;
        if(dx*dx+dy*dy>16) movedActually_=true;
    }
    if(interaction_.active() && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        interaction_.finishDrag(grid.x,grid.y,inArea && !overTools,symmetry_,movedActually_);

    Renderer::workspace(view_,rect(side,0,w-side,h));
    BeginScissorMode(static_cast<int>(area.x),static_cast<int>(area.y),static_cast<int>(area.width),static_cast<int>(area.height));
    for(const auto& p:vab_.parts()) Renderer::part(partById(p.defId),p.position.x,p.position.y,p.rotation,view_,Color{255,255,255,255},p.uid==interaction_.selected(),p.flipped);
    if(showCenter_) {
        CenterOfMass c=interaction_.centerOfMass();
        if(c.mass>0) {
            Vector2 p=Renderer::toScreen(c.x,c.y,view_); float r=9*s;
            DrawCircleV(p,r,ui::text); DrawCircleSector(p,r,0,90,12,ui::panelLight); DrawCircleSector(p,r,180,270,12,ui::panelLight);
            DrawCircleLinesV(p,r,ui::text); DrawLineEx({p.x-r-4,p.y},{p.x+r+4,p.y},1,ui::text); DrawLineEx({p.x,p.y-r-4},{p.x,p.y+r+4},1,ui::text);
            ui::label("CENTER OF MASS",p.x+13*s,p.y-7*s,12*s,ui::text);
        }
    }
    EndScissorMode();

    // Header and telemetry occupy only the workspace; the catalog remains full-height.
    ui::box(rect(side,0,w-side,top),ui::panel,ui::line);
    ui::label("VEHICLE ASSEMBLY",side+18*s,17*s,18*s,ui::text);
    if(ui::button(rect(w-306*s,10*s,136*s,40*s),"SYMMETRY",s,symmetry_)) symmetry_=!symmetry_;
    if(ui::button(rect(w-160*s,10*s,145*s,40*s),"FLIGHT >",s)) { interaction_.cancel(); state_=GameState::FLIGHT; }
    float bx=tools.x,by=tools.y,bh=tools.height;
    if(ui::button(rect(bx,by,72*s,bh),"< ROT",s)) interaction_.rotate(-1);
    if(ui::button(rect(bx+76*s,by,72*s,bh),"ROT >",s)) interaction_.rotate(1);
    if(ui::button(rect(bx+152*s,by,80*s,bh),"CLEAR",s)) interaction_.clear();
    if(ui::button(rect(bx+236*s,by,96*s,bh),"MASS",s,showCenter_)) showCenter_=!showCenter_;

    ui::box(rect(0,0,side,h),ui::panel,ui::line);
    DrawRectangleRec(rect(0,0,rail,h),Color{19,32,49,235});
    ui::label("AEGIS",14*s,16*s,18*s,ui::text);
    ui::label("PARTS",rail+14*s,17*s,17*s,ui::text);
    ui::label("DRAG TO BUILD",rail+14*s,43*s,12*s,ui::muted);
    DrawLineEx({rail,0},{rail,h},1,ui::line);
    DrawLineEx({rail,74*s},{side,74*s},1,ui::line);
    const char* names[]={"PODS","TANKS","ENGINES","STRUCT"};
    Category cats[]={Category::Pods,Category::FuelTanks,Category::Engines,Category::Structural};
    for(int i=0;i<4;++i) {
        Rectangle tab=rect(0,76*s+i*70*s,rail,68*s); bool active=category_==cats[i],hover=ui::hit(tab);
        if(active || hover) DrawRectangleRec(tab,active?Color{50,91,125,245}:Color{51,76,98,210});
        if(active) DrawRectangle(0,static_cast<int>(tab.y),3,static_cast<int>(tab.height),ui::cyan);
        Vector2 center{rail*.5f,tab.y+22*s}; Color icon=active?ui::cyan:ui::muted;
        if(i==0) { DrawTriangle({center.x,center.y-10*s},{center.x-9*s,center.y+8*s},{center.x+9*s,center.y+8*s},icon); }
        else if(i==1) { DrawRectangleLinesEx(rect(center.x-8*s,center.y-10*s,16*s,20*s),1.5f,icon); DrawLineEx({center.x-8*s,center.y-5*s},{center.x+8*s,center.y-5*s},1,icon); }
        else if(i==2) { DrawLineEx({center.x-6*s,center.y-8*s},{center.x-10*s,center.y+9*s},2,icon); DrawLineEx({center.x+6*s,center.y-8*s},{center.x+10*s,center.y+9*s},2,icon); DrawLineEx({center.x-10*s,center.y+9*s},{center.x+10*s,center.y+9*s},2,icon); }
        else { DrawRectangleLinesEx(rect(center.x-10*s,center.y-5*s,20*s,10*s),1.5f,icon); }
        ui::label(names[i],8*s,tab.y+45*s,12*s,active?ui::text:ui::muted);
        if(hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { category_=cats[i]; catalogScroll_=0; }
    }
    int count=0; for(const auto& d:partCatalog()) if(d.category==category_) ++count;
    // Compact flat rows: the vector part itself is the icon, without an inset
    // preview window. The whole row remains a drag target.
    const float titlePixels=static_cast<float>(ui::pixelSize(12*s));
    const float detailPixels=static_cast<float>(ui::pixelSize(11*s));
    const float titleOffset=19*s;
    const float detailGap=std::max(titlePixels+1,18*s);
    const float extraGap=std::max(detailPixels+1,14*s);
    const float tileHeight=std::max(72*s,titleOffset+detailGap+extraGap+detailPixels+3*s);
    float contentHeight=count*tileHeight;
    float maxScroll=std::max(0.0f,contentHeight-list.height);
    catalogScroll_=std::clamp(catalogScroll_,0.0f,maxScroll);
    BeginScissorMode(static_cast<int>(list.x),static_cast<int>(list.y),static_cast<int>(list.width),static_cast<int>(list.height));
    float tileY=list.y-catalogScroll_;
    for(const auto& d:partCatalog()) if(d.category==category_) {
        Rectangle tile=rect(rail+4*s,tileY,side-rail-12*s,tileHeight);
        bool hover=CheckCollisionPointRec(mouse,list) && CheckCollisionPointRec(mouse,tile);
        if(hover) DrawRectangleRec(tile,Color{57,87,113,200});
        DrawLineEx({tile.x,tile.y+tile.height-1},{tile.x+tile.width,tile.y+tile.height-1},1,Color{59,82,103,150});
        Renderer::tilePart(d,rect(tile.x+4*s,tile.y+7*s,53*s,58*s));
        // Labels are clamped to at least 12 physical pixels by ui::label.
        // Keep their baselines separated even at the smallest UI scale.
        float textX=tile.x+60*s, titleY=tile.y+titleOffset;
        float detailY=titleY+detailGap;
        float extraY=detailY+extraGap;
        ui::label(std::string(d.name),textX,titleY,12*s,ui::text);
        ui::label(number(d.dryMass,d.dryMass<1?2:1)+" t dry",textX,detailY,11*s,ui::muted);
        if(d.fuelCapacity>0) ui::label(number(d.fuelCapacity)+" t fuel",textX,extraY,11*s,ui::muted);
        else if(d.thrust>0) ui::label(number(d.thrust,0)+" kN "+(d.category==Category::Engines?"thrust":"RCS"),textX,extraY,11*s,ui::muted);
        if(hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { interaction_.choosePart(d.id); dragStart_=mouse; movedActually_=true; }
        tileY+=tileHeight;
    }
    EndScissorMode();
    if(maxScroll>0) {
        float barHeight=std::max(18*s,list.height*list.height/contentHeight);
        float barY=list.y+(list.height-barHeight)*(catalogScroll_/maxScroll);
        DrawRectangleRec(rect(side-5*s,barY,2*s,barHeight),ui::muted);
    }
    ui::label("Q / E  ROTATE",rail+10*s,h-60*s,12*s,ui::muted);
    ui::label("MMB  PAN",rail+10*s,h-42*s,12*s,ui::muted);
    ui::label("WHEEL  ZOOM",rail+10*s,h-24*s,12*s,ui::muted);

    ui::box(rect(side,h-bottom,w-side,bottom),ui::panel,ui::line);
    Telemetry t=interaction_.telemetry(); float left=side+20*s,gap=(w-side-40*s)/4;
    const std::string labels[]={"TOTAL MASS","THRUST","TWR / 1 G","PARTS"};
    const std::string vals[]={number(t.mass)+" t",number(t.thrust,0)+" kN",number(t.twr),std::to_string(t.count)};
    for(int i=0;i<4;++i) { ui::label(labels[i],left+i*gap,h-bottom+13*s,12*s,ui::muted); ui::label(vals[i],left+i*gap,h-bottom+39*s,20*s,ui::text); }

    // Draw a continuous ghost above the interface so it appears immediately on pickup.
    if(interaction_.active()) {
        const auto& d=partById(interaction_.partId()); auto p=interaction_.preview(grid.x,grid.y,symmetry_);
        int rotation=interaction_.rotation(), width=partWidth(d,rotation);
        bool snapped=inArea && p.snapped;
        double gx=snapped?p.position.x:interaction_.ghostX(grid.x), gy=snapped?p.position.y:interaction_.ghostY(grid.y);
        Color ghost=(!inArea || p.valid)?Color{150,220,255,180}:Color{255,128,125,180};
        Renderer::part(d,gx,gy,rotation,view_,ghost,false,interaction_.flipped());
        double mirrored=-gx-width;
        bool drawMirror=inArea && symmetry_ && !interaction_.moving() && -p.position.x-width!=p.position.x;
        if(drawMirror) Renderer::part(d,mirrored,gy,-rotation,view_,ghost,false,!interaction_.flipped());
        if(p.snapped && inArea) {
            Color indicator=p.valid?ui::cyan:ui::orange;
            DrawCircleLinesV(Renderer::toScreen(p.snapPoint.x,p.snapPoint.y,view_),7*s,indicator);
            if(drawMirror) DrawCircleLinesV(Renderer::toScreen(-p.snapPoint.x,p.snapPoint.y,view_),7*s,indicator);
        }
    }
}
void Game::flight() {
    float w=static_cast<float>(GetScreenWidth()),h=static_cast<float>(GetScreenHeight()),s=std::min(settings_.uiScale,std::min(w/900.0f,h/660.0f));
    header("MISSION CONTROL","FLIGHT",w*.12f,h*.24f,s);
    ui::label("Flight simulation is not part of this milestone.",w*.12f,h*.24f+85*s,16*s,ui::muted);
    if(ui::button(rect(w*.12f,h*.24f+135*s,250*s,48*s),"<  RETURN TO VAB",s) || IsKeyPressed(KEY_ESCAPE)) state_=GameState::VAB;
}

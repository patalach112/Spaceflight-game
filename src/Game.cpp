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
    float w=static_cast<float>(GetScreenWidth()),h=static_cast<float>(GetScreenHeight()),s=std::min(settings_.uiScale,std::min(w/900.0f,h/660.0f));
    float top=62*s, side=248*s, bottom=82*s;
    Rectangle area=rect(side,top,w-side,h-top-bottom);
    if(!viewInitialized_) { view_={side+area.width*.5f,top+area.height*.80f,1}; viewInitialized_=true; }
    Vector2 mouse=GetMousePosition(); bool inArea=CheckCollisionPointRec(mouse,area);
    if(inArea && (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || (!interaction_.active() && !rightCancel && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)))) { Vector2 d=GetMouseDelta(); view_.originX+=d.x; view_.originY+=d.y; }
    float wheel=GetMouseWheelMove();
    if(inArea && wheel!=0) { zoomAnchor_=mouse; targetZoom_=std::clamp(targetZoom_*std::pow(1.15f,wheel),.25f,3.5f); }
    if(std::abs(targetZoom_-view_.zoom) > 0.0001f) {
        Vector2 before=Renderer::toGrid(zoomAnchor_,view_);
        float blend=1.0f-std::exp(-14.0f*std::min(GetFrameTime(),0.1f));
        view_.zoom += (targetZoom_-view_.zoom)*blend;
        Vector2 after=Renderer::toScreen(before.x,before.y,view_);
        view_.originX+=zoomAnchor_.x-after.x; view_.originY+=zoomAnchor_.y-after.y;
    }
    Renderer::workspace(view_,area);
    Vector2 grid=Renderer::toGrid(mouse,view_);
    if(inArea && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) interaction_.eraseSelectedOrAt(grid.x,grid.y);
    if(inArea && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(interaction_.active()) interaction_.placeClick(grid.x,grid.y,symmetry_);
        else { interaction_.beginMove(grid.x,grid.y); dragStart_=mouse; movedActually_=false; }
    }
    if(interaction_.moving() && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float dx=mouse.x-dragStart_.x, dy=mouse.y-dragStart_.y;
        if(dx*dx+dy*dy > 16.0f) movedActually_=true;
    }
    if(interaction_.moving() && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if(inArea) interaction_.endMove(grid.x,grid.y,movedActually_);
        else interaction_.cancel();
    }
    Placement preview{};
    if(interaction_.active() && inArea) preview=interaction_.preview(grid.x,grid.y,symmetry_);
    BeginScissorMode(static_cast<int>(area.x),static_cast<int>(area.y),static_cast<int>(area.width),static_cast<int>(area.height));
    for(const auto& p:vab_.parts()) Renderer::part(partById(p.defId),p.position,view_,ui::text,p.uid==interaction_.selected());
    if(interaction_.active() && inArea) {
        const auto& d=partById(interaction_.partId()); Color ghost=preview.valid ? Color{96,190,255,170} : Color{244,116,115,170};
        Renderer::part(d,preview.position,view_,ghost);
        GridPoint mirrored{-preview.position.x-d.width,preview.position.y};
        if(symmetry_ && !interaction_.moving() && mirrored.x!=preview.position.x) Renderer::part(d,mirrored,view_,ghost);
        if(preview.snapped) {
            Color indicator=preview.valid?ui::cyan:ui::orange;
            DrawCircleLinesV(Renderer::toScreen(preview.snapPoint.x,preview.snapPoint.y,view_),7*s,indicator);
            if(symmetry_ && !interaction_.moving() && mirrored.x!=preview.position.x)
                DrawCircleLinesV(Renderer::toScreen(-preview.snapPoint.x,preview.snapPoint.y,view_),7*s,indicator);
        }
    }
    EndScissorMode();
    if(interaction_.active()) ui::label("LEFT CLICK TO PLACE   /   RIGHT CLICK OR ESC TO CANCEL",side+16*s,top+14*s,11*s,ui::muted);
    ui::box(rect(0,0,w,top),ui::panel);
    ui::label("AEGIS",20*s,18*s,22*s,ui::text); ui::label("/  VEHICLE ASSEMBLY",105*s,24*s,12*s,ui::muted);
    if(ui::button(rect(w-330*s,10*s,145*s,42*s),"SYMMETRY",s,symmetry_)) symmetry_=!symmetry_;
    if(ui::button(rect(w-174*s,10*s,154*s,42*s),"FLIGHT >",s)) { interaction_.cancel(); state_=GameState::FLIGHT; }
    ui::box(rect(0,top,side,h-top-bottom),ui::panel);
    ui::label("PART CATALOG",18*s,top+20*s,14*s,ui::text);
    ui::label("SELECT A CATEGORY",18*s,top+42*s,10*s,ui::muted);
    const char* names[]={"PODS","TANKS","ENGINES","STRUCT"};
    Category cats[]={Category::Pods,Category::FuelTanks,Category::Engines,Category::Structural};
    float tabW=(side-36*s)/2;
    for(int i=0;i<4;++i) if(ui::button(rect(18*s+(i%2)*tabW,top+65*s+(i/2)*37*s,tabW-3*s,33*s),names[i],s,category_==cats[i])) category_=cats[i];
    float tileY=top+156*s;
    for(const auto& d:partCatalog()) if(d.category==category_) {
        Rectangle tile=rect(18*s,tileY,side-36*s,128*s); bool hovered=ui::hit(tile);
        ui::box(tile,hovered?Color{53,76,105,255}:ui::panelLight,hovered?ui::cyan:ui::line);
        Renderer::tilePart(d,rect(tile.x+8*s,tile.y+8*s,65*s,80*s));
        ui::label(std::string(d.name),tile.x+79*s,tile.y+22*s,12*s,ui::text);
        ui::label(number(d.dryMass,2)+" t DRY",tile.x+79*s,tile.y+44*s,10*s,ui::muted);
        ui::label("CLICK TO PICK UP",tile.x+9*s,tile.y+105*s,10*s,ui::muted);
        if(hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { interaction_.choosePart(d.id); }
        tileY+=140*s;
    }
    ui::label("MMB / RMB  PAN",18*s,h-bottom-55*s,11*s,ui::muted);
    ui::label("WHEEL  ZOOM   /   DEL  REMOVE",18*s,h-bottom-36*s,10*s,ui::muted);
    ui::box(rect(0,h-bottom,w,bottom),ui::panel);
    Telemetry t=interaction_.telemetry();
    float left=side+20*s, gap=std::max(110*s,(w-left-20*s)/4);
    const std::string labels[]={"TOTAL MASS", "TOTAL THRUST", "TWR  /  1 G", "PART COUNT"};
    const std::string vals[]={number(t.mass)+" t",number(t.thrust,0)+" kN",number(t.twr),std::to_string(t.count)};
    for(int i=0;i<4;i++) { ui::label(labels[i],left+i*gap,h-bottom+15*s,10*s,ui::muted); ui::label(vals[i],left+i*gap,h-bottom+34*s,19*s,i==2?ui::cyan:ui::text); }
    ui::label("BUILD STATUS",18*s,h-bottom+15*s,10*s,ui::muted); ui::label("ASSEMBLY",18*s,h-bottom+35*s,17*s,ui::orange);
}
void Game::flight() {
    float w=static_cast<float>(GetScreenWidth()),h=static_cast<float>(GetScreenHeight()),s=std::min(settings_.uiScale,std::min(w/900.0f,h/660.0f));
    header("MISSION CONTROL","FLIGHT",w*.12f,h*.24f,s);
    ui::label("Flight simulation is not part of this milestone.",w*.12f,h*.24f+85*s,16*s,ui::muted);
    if(ui::button(rect(w*.12f,h*.24f+135*s,250*s,48*s),"<  RETURN TO VAB",s) || IsKeyPressed(KEY_ESCAPE)) state_=GameState::VAB;
}

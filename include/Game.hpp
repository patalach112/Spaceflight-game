#pragma once
#include "GameState.hpp"
#include "Renderer.hpp"
#include <optional>
class Game {
public:
    void run();
private:
    GameState state_ = GameState::MAIN_MENU;
    Settings settings_;
    VABSystem vab_;
    Viewport view_{0,0,1};
    bool viewInitialized_ = false, symmetry_ = false, quit_ = false;
    Category category_ = Category::Pods;
    int dragId_ = -1;
    std::optional<PartInstance> moved_;
    unsigned long long selected_ = 0;
    bool mouseWasOverPalette_ = false, movedActually_ = false;
    Vector2 dragStart_{}, dragOffset_{}, zoomAnchor_{};
    float targetZoom_ = 1.0f;
    void update();
    void menu();
    void settings();
    void workshop();
    void flight();
    void cancelDrag();
};

#pragma once

enum class GameState { MAIN_MENU, SETTINGS, VAB, FLIGHT };
struct Settings {
    bool fullscreen = false;
    bool vsync = true;
    float uiScale = 1.0f;
    float volume = 0.7f;
};

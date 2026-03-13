#include <cmath>
#include <string>
#include <random>
#include <deque>
#include "raylib.h"
#include "raymath.h"

#define WIN_SZ 1024
#define WIN_NOM "⚡⚡⚡"
#define FONT_SZ 32

#define SLIDER_M 100
#define SLIDER_H 64
#define SLIDER_TH 3
#define SLIDER_R 8
#define SLIDER_R2 SLIDER_R * SLIDER_R
#define SLIDER_CLR1 DARKGRAY
#define SLIDER_CLR2 GRAY

#define DOT_R 16
#define DOT_R2 DOT_R * DOT_R
#define DOT_M 50

#define LGT_MAX_STEP 1000
#define LGT_R DOT_R * 3

struct Slider {
    std::string nom;
    float min, val, max;
};

Slider sliders[4] = {
    {"rate", 0.6f, 0.9f, 1.0f},
    {"brat", 0.0f, 0.1f, 0.9f},
    {"blen", 0.1f, 0.5f, 0.9f},
    {"step", 1.0f, 10.0f, 100.0f},
};

void handleAndDrawSliders(const Font& font) {
    static Slider* selectedSlider = nullptr;
    const auto mpos = GetMousePosition();
    const auto nsliders = (sizeof(sliders) / sizeof(sliders[0])); 
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        selectedSlider = nullptr;
    if (selectedSlider) {
        selectedSlider->val = 
            selectedSlider->min + (selectedSlider->max - selectedSlider->min) * 
                ((mpos.x - SLIDER_M) / (WIN_SZ - 2.0f * SLIDER_M));
        selectedSlider->val = std::max(selectedSlider->min, std::min(selectedSlider->val, selectedSlider->max));
    }

    for (int i = 0; i < nsliders; ++i) {
        auto& slider = sliders[i];
        auto sliderY = WIN_SZ - DOT_M - (nsliders - i - 0.5f) * SLIDER_H;
        auto sldPos = Vector2{
            SLIDER_M + ((slider.val - slider.min) / (slider.max - slider.min)) * (WIN_SZ - 2.0f * SLIDER_M),
            sliderY
        };
        if (!selectedSlider && Vector2DistanceSqr(sldPos, mpos) < SLIDER_R2)
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                selectedSlider = &slider;
        DrawTextEx(font, slider.nom.c_str(), {FONT_SZ * 0.5f, sliderY + (FONT_SZ - SLIDER_H) * 0.5f}, FONT_SZ, 1, WHITE);
        DrawLineV({SLIDER_M, sliderY}, {WIN_SZ - SLIDER_M, sliderY}, SLIDER_CLR1);
        auto valstr = std::to_string(slider.val);
        valstr = valstr.substr(0, std::min((int)valstr.length(), 4));
        DrawTextEx(font, valstr.c_str(), {WIN_SZ - SLIDER_M + FONT_SZ * 0.5f, sliderY + (FONT_SZ - SLIDER_H) * 0.5f}, FONT_SZ, 1, WHITE);
        DrawCircleV(sldPos, SLIDER_R, SLIDER_CLR2);
    }
}

struct Dot {
    Vector2 pos;
    Color clr;
};

Dot dots[4] = {
    {{100, 100}, {0x50, 0, 0, 0xff}},
    {{100, 300}, {0, 0x50, 0, 0xff}},
    {{300, 100}, {0, 0, 0x50, 0xff}},
    {{300, 300}, {0x50, 0x50, 0x50, 0xff}},
};

float get_normal_sample(float mean = 0, float stddev = 1.0f) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<float> dist(mean, stddev);
    return dist(gen);
}

void drawLightning(Dot* dot1, Dot* dot2, float rate, float brat, float blen, float step) {
    Vector2 pos1 = dot1->pos, pos = pos1;
    Vector2 pos2 = dot2 ? dot2->pos : 
        Vector2Add(pos1, Vector2Scale(Vector2{get_normal_sample(), get_normal_sample()}, LGT_R * 0.5f));
    float currate = rate;
    float initdist = Vector2Distance(pos1, pos2);
    const float step2 = step * step;
    bool done = false;
    std::deque<Vector2> hist;
    for (int i = 0; i < LGT_MAX_STEP; ++i) {
        auto n = Vector2Normalize(Vector2Subtract(pos2, pos));
        float dir = atan2(n.y, n.x);
        float val = get_normal_sample(dir, 3.14159f * (1.0f - currate) + 1e-6);
        auto newpos = Vector2Add(pos, Vector2Scale({cos(val), sin(val)}, step));
        if (Vector2DistanceSqr(newpos, pos2) < step2) {
            newpos = pos2;
            done = true;
        }
        DrawLineV(pos, newpos, SKYBLUE);
        if (done)
            break;
        hist.push_back(pos);
        pos = newpos;
        float dist = Vector2Distance(pos1, pos);
        currate = rate + (1.0f - rate) * (1.0f - rate) * std::max(0.0f, std::min(dist / initdist, 1.0f));

        if ((rand() % 1000) < brat * 1000) {
            for (int i = 0; i < hist.size(); ++i) {
                pos = hist.back();
                hist.pop_back();
                if ((rand() % 1000) < blen * 1000)
                    break;
            }
        }
    }
}

void handleAndDrawDots() {
    static Dot* selectedDot = nullptr;
    const auto mpos = GetMousePosition();
    const auto nsliders = (sizeof(sliders) / sizeof(sliders[0])); 
    float rctM = DOT_M;
    float rctW = WIN_SZ - 2.0f * DOT_M;
    float rctH = WIN_SZ - nsliders * SLIDER_H - 3.0f * DOT_M;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        selectedDot = nullptr;
    if (selectedDot) {
        selectedDot->pos = mpos;
        selectedDot->pos.x = std::max(rctM + DOT_R, std::min(selectedDot->pos.x, rctM + rctW - DOT_R));
        selectedDot->pos.y = std::max(rctM + DOT_R, std::min(selectedDot->pos.y, rctM + rctH - DOT_R));
    } else {
        for (auto& dot : dots)
            if (Vector2DistanceSqr(dot.pos, mpos) < DOT_R2)
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    selectedDot = &dot;
    }

    DrawRectangleLines(rctM, rctM, rctW, rctH, WHITE);
    for (const auto& dot : dots)
        DrawCircleV(dot.pos, DOT_R, dot.clr);

    for (int i = 0; i < 3; ++i)
        drawLightning(&dots[i], &dots[(i + 1) % 3], sliders[0].val, sliders[1].val, sliders[2].val, sliders[3].val);
    drawLightning(&dots[3], nullptr, sliders[0].val, sliders[1].val, sliders[2].val, sliders[3].val);
}

int main() {
    
    //SetTraceLogLevel(LOG_ERROR);
    InitWindow(WIN_SZ, WIN_SZ, WIN_NOM);
    SetTargetFPS(60);

    char8_t _allChars[228] = u8" !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";
    int c; auto cdpts = LoadCodepoints((const char*)_allChars, &c);
    Font font = LoadFontEx("font.ttf", FONT_SZ, cdpts, c);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        handleAndDrawDots();
        handleAndDrawSliders(font);
        EndDrawing();
    }

    CloseWindow();
    
    return 0;
}
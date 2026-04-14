#include <algorithm>
#include <cmath>
#include <string>
#include <random>
#include <deque>
#include <vector>
#include "raylib.h"
#include "raymath.h"

#define WIN_SZ 800
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

Slider sliders[5] = {
    {"rate", 0.6f, 0.9f, 1.0f},
    {"brat", 0.0f, 0.1f, 0.9f},
    {"blen", 0.1f, 0.5f, 0.9f},
    {"step", 1.0f, 10.0f, 100.0f},
    {"round", 0.0f, 0.5f, 1.0f},
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

Dot dots[6] = {
    {{100, 100}, {0x50, 0, 0, 0xff}},
    {{300, 100}, {0, 0x50, 0, 0xff}},
    {{100, 300}, {0, 0, 0x50, 0xff}},
    {{300, 300}, {0x50, 0x50, 0x50, 0xff}},
    {{50, 50}, {0x50, 0x50, 0, 0xff}},
    {{350, 350}, {0, 0x50, 0x50, 0xff}},
};

std::vector<int> pipes[2] = {
    {4,0,1,5},
    {-4,0,2,1,-5}
};

float get_normal_sample(float mean = 0, float stddev = 1.0f) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<float> dist(mean, stddev);
    return dist(gen);
}

Vector2 mapLineToBez(Vector2 p1, Vector2 c2, Vector2 p3, Vector2 p) {
    Vector2 line = Vector2Subtract(p3, p1);
    float lineLen = Vector2Length(line);
    if (lineLen <= 1e-6f) return p1;
    Vector2 lineDir = Vector2Scale(line, 1.0f / lineLen);
    float t = Vector2DotProduct(Vector2Subtract(p, p1), lineDir) / lineLen;
    t = std::clamp(t, 0.0f, 1.0f);
    Vector2 lineNormal(-lineDir.y, lineDir.x);
    float d = Vector2DotProduct(Vector2Subtract(p, p1), lineNormal);
    float u = 1.0f - t;
    Vector2 bez =
        Vector2Add(Vector2Add(
            Vector2Scale(p1, u * u),
            Vector2Scale(c2, 2.0f * u * t)),
            Vector2Scale(p3, t * t)
        );
    Vector2 tan =
        Vector2Add(
            Vector2Scale(Vector2Subtract(c2, p1), 2.0f * u),
            Vector2Scale(Vector2Subtract(p3, c2), 2.0f * t)
        );
    float tanLen = Vector2Length(tan);
    if (tanLen <= 1e-6f) return bez;
    Vector2 bezDir = Vector2Scale(tan, 1.0f / tanLen);
    Vector2 bezNormal(-bezDir.y, bezDir.x);
    return Vector2Add(bez, Vector2Scale(bezNormal, d));
}

void drawLightning(Dot* dot1, Dot* dot2, float rate, float brat, float blen, float step, float round, Dot* dot0 = nullptr, Dot* dot3 = nullptr) {
    Vector2 pos1 = dot1->pos, pos = pos1;
    Vector2 pos2 = dot2 ? dot2->pos : 
        Vector2Add(pos1, Vector2Scale(Vector2{get_normal_sample(), get_normal_sample()}, LGT_R * 0.5f));
    Vector2 cp1 = pos1;
    Vector2 cp2 = pos2;
    if (dot0 || dot3) {
        auto pos0 = dot0 ? dot0->pos : pos1;
        auto pos3 = dot3 ? dot3->pos : pos2;
        auto mid = Vector2Add(pos0, Vector2Scale(Vector2Subtract(pos2, pos0), 0.5f));
        auto midir = Vector2Rotate(Vector2Normalize(Vector2Subtract(mid, pos1)), -PI * 0.5f);
        cp1 = Vector2Add(pos1, Vector2Scale(midir, round * Vector2DotProduct(Vector2Subtract(pos2, pos1), midir)));
        mid = Vector2Add(pos1, Vector2Scale(Vector2Subtract(pos3, pos1), 0.5f));
        midir = Vector2Rotate(Vector2Normalize(Vector2Subtract(mid, pos2)), -PI * 0.5f);
        cp2 = Vector2Add(pos2, Vector2Scale(midir, -round * Vector2DotProduct(Vector2Subtract(pos2, pos1), midir)));
        //DrawCircleV(cp1, 2, RED);
        //DrawCircleV(cp2, 2, GREEN);
    }
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
        float dist = Vector2Distance(pos1, newpos);
        if (dot3) {
            auto p11 = mapLineToBez(pos1, cp1, pos2, pos);
            auto p12 = mapLineToBez(pos1, cp2, pos2, pos);
            auto p21 = mapLineToBez(pos1, cp1, pos2, newpos);
            auto p22 = mapLineToBez(pos1, cp2, pos2, newpos);
            float dist0 = Vector2Distance(pos1, pos);
            auto p1 = Vector2Lerp(p11, p12, dist0 / initdist);
            auto p2 = Vector2Lerp(p21, p22, dist / initdist);
            DrawLineV(p1, p2, SKYBLUE);
        } else {
            DrawLineV(pos, newpos, SKYBLUE);
        }
        if (done)
            break;
        hist.push_back(pos);
        pos = newpos;
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

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < pipes[i].size() - 1; ++j) {
            Dot* prv = (j > 1) ? (&dots[pipes[i][j-1]]) : nullptr;
            Dot* cur = &dots[pipes[i][j]];
            Dot* nxt = &dots[pipes[i][j+1]];
            Dot* aft = (j < pipes[i].size() - 2) ? (&dots[pipes[i][j+2]]) : nullptr;
            if (pipes[i][j] >= 0 && pipes[i][j+1] >= 0)
                drawLightning(cur, nxt, sliders[0].val, sliders[1].val, sliders[2].val, sliders[3].val, sliders[4].val, prv, aft);
        }
    }
    //for (int i = 0; i < 3; ++i)
    //    drawLightning(&dots[i], &dots[(i + 1) % 3], sliders[0].val, sliders[1].val, sliders[2].val, sliders[3].val, sliders[4].val,  &dots[(i + 2) % 3],  &dots[(i + 2) % 3]);
    drawLightning(&dots[3], nullptr, sliders[0].val, sliders[1].val, sliders[2].val, sliders[3].val, sliders[4].val);
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
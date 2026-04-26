#include <algorithm>
#include <deque>
#include <random>
#include "raylib.h"
#include "raymath.h"

float _getNormalSample(float mean = 0, float stddev = 1.0f) 
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<float> dist(mean, stddev);
    return dist(gen);
}

Vector2 _mapLineToBez(Vector2 p1, Vector2 c2, Vector2 p3, Vector2 p) 
{
    Vector2 line = Vector2Subtract(p3, p1);
    float lineLen = Vector2Length(line);
    if (lineLen <= 1e-6f) 
        return p1;
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

struct LGTcfg {
    const int LGT_MAX_STEP = 10000;
    const int LGT_R = 16 * 3;
    const float rate  = 0.9;
    const float brat  = 0.1;
    const float blen  = 0.5;
    const float step  = 10.0f;
    const float round = 0.5f;
};

void drawLightning(Vector2* dot1, Vector2* dot2, Vector2* dot0 = nullptr, Vector2* dot3 = nullptr, LGTcfg* cfg_ = nullptr) {
    static LGTcfg _cfg = {};
    LGTcfg& cfg = cfg_ ? *cfg_ : _cfg;
    Vector2 pos1 = *dot1, pos = pos1;
    Vector2 pos2 = dot2 ? *dot2 : 
        Vector2Add(pos1, Vector2Scale(Vector2{_getNormalSample(), _getNormalSample()}, cfg.LGT_R * 0.5f));
    Vector2 cp1 = pos1;
    Vector2 cp2 = pos2;
    auto pos0 = dot0 ? *dot0 : pos1;
    auto pos3 = dot3 ? *dot3 : pos2;
    if (dot0) {
        auto cangle = (Vector2Angle(Vector2Subtract(pos0, pos1), Vector2Subtract(pos2, pos1)));
        auto hangle = 0.5f * (PI - cangle);
        auto dir = Vector2Rotate(Vector2Normalize(Vector2Subtract(pos2, pos1)), hangle);
        cp1 = Vector2Add(pos1, Vector2Scale(dir, cfg.round * Vector2DotProduct(Vector2Subtract(pos2, pos1), dir)));
        //DrawCircleV(cp1, 2, RED);
    }
    if (dot3) {
        auto cangle = (Vector2Angle(Vector2Subtract(pos3, pos2), Vector2Subtract(pos1, pos2)));
        auto hangle = 0.5f * (PI - cangle);
        auto dir = Vector2Rotate(Vector2Normalize(Vector2Subtract(pos1, pos2)), hangle);
        cp2 = Vector2Add(pos2, Vector2Scale(dir, cfg.round * Vector2DotProduct(Vector2Subtract(pos1, pos2), dir)));
        //DrawCircleV(cp2, 2, GREEN);
    }
    float currate = cfg.rate;
    float initdist = Vector2Distance(pos1, pos2);
    const float step2 = cfg.step * cfg.step;
    bool done = false;
    std::deque<Vector2> hist;
    for (int i = 0; i < cfg.LGT_MAX_STEP; ++i) {
        auto n = Vector2Normalize(Vector2Subtract(pos2, pos));
        float dir = atan2(n.y, n.x);
        float val = _getNormalSample(dir, 3.14159f * (1.0f - currate) + 1e-6);
        auto newpos = Vector2Add(pos, Vector2Scale({cos(val), sin(val)}, cfg.step));
        if (Vector2DistanceSqr(newpos, pos2) < step2) {
            newpos = pos2;
            done = true;
        }
        float dist = Vector2Distance(pos1, newpos);
        if (dot0 || dot3) {
            auto p11 = _mapLineToBez(pos1, cp1, pos2, pos);
            auto p12 = _mapLineToBez(pos1, cp2, pos2, pos);
            auto p21 = _mapLineToBez(pos1, cp1, pos2, newpos);
            auto p22 = _mapLineToBez(pos1, cp2, pos2, newpos);
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
        currate = cfg.rate + (1.0f - cfg.rate) * (1.0f - cfg.rate) * std::max(0.0f, std::min(dist / initdist, 1.0f));

        if ((rand() % 1000) < cfg.brat * 1000) {
            for (int i = 0; i < hist.size(); ++i) {
                pos = hist.back();
                hist.pop_back();
                if ((rand() % 1000) < cfg.blen * 1000)
                    break;
            }
        }
    }
}
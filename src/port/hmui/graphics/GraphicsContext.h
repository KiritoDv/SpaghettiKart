#pragma once

#define HMUI_N64 1

#ifdef HMUI_N64
#include <Fast3D/lus_gbi.h>
typedef Fast::F3DGfx GfxList;
#else
#error "Unsupported platform for GraphicsContext.h"
typedef struct GfxList {
    // Placeholder for non-N64 platforms
} GfxList;
#endif

struct Color2D {
    float r, g, b, a;

    Color2D(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
};

struct Rect {
    float x, y, width, height;

    Rect(float xPos, float yPos, float w, float h)
        : x(xPos), y(yPos), width(w), height(h) {}

    bool contains(float px, float py) const {
        return (px >= x && px <= x + width && py >= y && py <= y + height);
    }
};

class GraphicsContext {
public:
    virtual void init() = 0;
    virtual void dispose() = 0;
    virtual void drawLine(float x1, float y1, float x2, float y2) = 0;
    virtual void drawRect(const Rect& rect, const Color2D& color) = 0;
    virtual void fillRect(const Rect& rect, const Color2D& color) = 0;
    virtual void drawText(float x, float y, const char* text, const Color2D& color) = 0;
    virtual void drawImage(const Rect& rect, const char* imagePath, const Color2D& color, float scale = 1.0f) = 0;
    virtual void drawImageEx(const Rect& rect, const Rect& srcRect, const char* imagePath, const Color2D& color) = 0;
    virtual void setScissor(const Rect& rect) = 0;
    virtual void clearScissor() = 0;

    virtual void build(GfxList** out) = 0;
};

class N64GraphicsContext : public GraphicsContext {
public:
    void init() override;
    void dispose() override;
    void drawLine(float x1, float y1, float x2, float y2) override;
    void drawRect(const Rect& rect, const Color2D& color) override;
    void fillRect(const Rect& rect, const Color2D& color) override;
    void drawText(float x, float y, const char* text, const Color2D& color) override;
    void drawImage(const Rect& rect, const char* texture, const Color2D& color, float scale = 1.0f) override;
    void drawImageEx(const Rect& rect, const Rect& srcRect, const char* texture, const Color2D& color) override;
    void setScissor(const Rect& rect) override;
    void clearScissor() override;

    void build(GfxList** out) override;
};
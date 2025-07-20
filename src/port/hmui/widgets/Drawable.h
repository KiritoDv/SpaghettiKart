#pragma once

#include <memory>
#include "port/hmui/view/IView.h"
#include "port/hmui/graphics/GraphicsContext.h"

struct EdgeInsets {
    float left, top, right, bottom;

    EdgeInsets(float l = 0, float t = 0, float r = 0, float b = 0)
        : left(l), top(t), right(r), bottom(b) {}

    static EdgeInsets all(float value) {
        return EdgeInsets(value, value, value, value);
    }

    static EdgeInsets symmetric(float horizontal, float vertical) {
        return EdgeInsets(horizontal, vertical, horizontal, vertical);
    }

    static EdgeInsets only(float left, float top, float right, float bottom) {
        return EdgeInsets(left, top, right, bottom);
    }
};

class Drawable {
public:
    Drawable() : bounds(Rect(0, 0, 0, 0)) {}
    virtual ~Drawable() = default;

    virtual void init() {};
    virtual void dispose() {};

    // Internal methods for rendering
    virtual void onDraw(GraphicsContext* ctx, float x, float y) = 0;
    virtual void onUpdate(float delta) = 0;

    virtual Rect getBounds() const {
        return bounds;
    }

    virtual void setBounds(const Rect& rect) {
        bounds = rect;
    }

    virtual void setParent(Drawable* _parent) {
        this->parent = _parent;
    }

    virtual Drawable* getParent() const {
        return parent;
    }

    virtual void setView(std::shared_ptr<IView> _view) {
        this->view = _view;
    }

    virtual std::shared_ptr<IView> getView() const {
        return view;
    }
protected:
    Rect bounds;
    Drawable* parent;
    std::shared_ptr<IView> view;
};

// Suggestion: Make defines for instanciating Drawable
// #define COLUMN(...) std::make_shared<Column>(__VA_ARGS__)

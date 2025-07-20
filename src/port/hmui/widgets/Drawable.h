#pragma once

#include <memory>
#include "port/hmui/view/IView.h"
#include "port/hmui/graphics/GraphicsContext.h"

class Drawable {
public:
    virtual void init() {};
    virtual void dispose() {};
    virtual Drawable build() = 0;

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

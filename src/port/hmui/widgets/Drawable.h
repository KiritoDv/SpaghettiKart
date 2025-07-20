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
        throw std::runtime_error("getBounds() not implemented");
    }

    virtual void setParent(Drawable* parent) {
        this->parent = parent;
    }
    virtual Drawable* getParent() const {
        return parent;
    }
    virtual void setView(std::shared_ptr<IView> view) {
        this->view = view;
    }
    virtual std::shared_ptr<IView> getView() const {
        return view;
    }
protected:
    Drawable* parent;
    std::shared_ptr<IView> view;
};


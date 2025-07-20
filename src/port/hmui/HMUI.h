#pragma once

#include <memory>
#include "port/hmui/graphics/GraphicsContext.h"
#include "port/hmui/view/IView.h"

class HMUI {
public:
    virtual ~HMUI();
    void initialize(std::shared_ptr<GraphicsContext> ctx);
    void show(std::shared_ptr<IView> view);
    void close();
    bool isActive() const {
        return view != nullptr;
    }
private:
    std::shared_ptr<IView> view;
    std::shared_ptr<Drawable> drawable;
    std::shared_ptr<GraphicsContext> context;

    // Internal methods for rendering
    void draw(GfxList** out);
    void update(float delta);
};
#pragma once

#include "port/hmui/graphics/GraphicsContext.h"
#include "port/hmui/widgets/Drawable.h"
#include <memory>

class IView {
public:
    virtual void init() = 0;
    virtual void dispose() = 0;
    virtual std::shared_ptr<Drawable> build() = 0;
};
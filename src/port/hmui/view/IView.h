#pragma once

#include <memory>

class Drawable;

class IView {
public:
    virtual void init() = 0;
    virtual void dispose() = 0;
    virtual std::shared_ptr<Drawable> build() = 0;
};
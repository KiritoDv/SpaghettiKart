#pragma once

#include <memory>

class Drawable;

class IView {
public:
    virtual void init() {};
    virtual void dispose() {};
    virtual std::shared_ptr<Drawable> build() = 0;
};
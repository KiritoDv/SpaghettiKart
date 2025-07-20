#pragma once

#include "Drawable.h"

struct ContainerProperties {
    float width = 0;
    float height = 0;
    EdgeInsets padding = EdgeInsets();
    EdgeInsets margin = EdgeInsets();
    bool clipToBounds = false;
    Color2D color = Color2D(1.0f, 1.0f, 1.0f, 1.0f);
    std::shared_ptr<Drawable> child = nullptr;
};

class D_Container : public Drawable {
public:
    explicit D_Container(
        ContainerProperties properties
    ) : properties(properties) {
        if (properties.child) {
            properties.child->setParent(this);
        }
    };

    void init() override {
        if (properties.child) {
            properties.child->init();
            bounds.width = properties.width > 0 ? properties.width : properties.child->getBounds().width;
            bounds.height = properties.height > 0 ? properties.height : properties.child->getBounds().height;
        } else {
            bounds.width = properties.width;
            bounds.height = properties.height;
        }
    }

    void dispose() override {
        if (properties.child) {
            properties.child->dispose();
        }
    }

    void onDraw(GraphicsContext* ctx, float x, float y) override {
        if (properties.clipToBounds) {
            ctx->setScissor(bounds);
        }

        ctx->fillRect(bounds, properties.color);

        if (properties.child) {
            properties.child->setBounds(Rect(x + properties.padding.left, y + properties.padding.top,
                                             bounds.width - properties.padding.left - properties.padding.right,
                                             bounds.height - properties.padding.top - properties.padding.bottom));
            properties.child->onDraw(ctx, x + properties.padding.left, y + properties.padding.top);
        }

        if (properties.clipToBounds) {
            ctx->clearScissor();
        }
    }

    void onUpdate(float delta) override {
        if (properties.child) {
            properties.child->onUpdate(delta);
        }
    }

protected:
    ContainerProperties properties;
};

#define Container(...) \
    std::make_shared<D_Container>(ContainerProperties{__VA_ARGS__})
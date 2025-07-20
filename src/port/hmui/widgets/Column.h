#pragma once

#include "port/hmui/widgets/Drawable.h"
#include <vector>
#include <memory>
#include <algorithm>

enum class CrossAxisAlignment {
    LEFT,
    CENTER,
    RIGHT
};

enum class MainAxisAlignment {
    START,
    CENTER,
    END,
    SPACE_BETWEEN,
    SPACE_AROUND
};

typedef std::vector<std::shared_ptr<Drawable>> ChildrenList;

struct ColumnProperties {
    MainAxisAlignment mainAxisAlignment = MainAxisAlignment::START;
    CrossAxisAlignment crossAxisAlignment = CrossAxisAlignment::LEFT;
    ChildrenList children;
};

class D_Column : public Drawable {
public:
    explicit D_Column(
        const ColumnProperties& properties
    ) : children(properties.children), mainAxisAlignment(properties.mainAxisAlignment), crossAxisAlignment(properties.crossAxisAlignment) {
        if (properties.children.empty()) {
            bounds = Rect(0, 0, 0, 0);
        } else {
            bounds = Rect(0, 0, properties.children[0]->getBounds().width, 0);
        }
    };

    void init() override {
        for (const auto& child : children) {
            child->init();
            bounds.width = std::max(bounds.width, child->getBounds().width);
            bounds.height += child->getBounds().height;
        }
    }

    void onDraw(GraphicsContext* ctx, float x, float y) override {
        float xPos = x;
        float yPos = mainAxisAlignment == MainAxisAlignment::START ? y : 
                      (mainAxisAlignment == MainAxisAlignment::CENTER ? y + (bounds.height / 2) : y + bounds.height);
        for (const auto& child : children) {
            if (crossAxisAlignment == CrossAxisAlignment::LEFT) {
                xPos = x;
            } else if (crossAxisAlignment == CrossAxisAlignment::CENTER) {
                xPos = x + (bounds.width - child->getBounds().width) / 2;
            } else if (crossAxisAlignment == CrossAxisAlignment::RIGHT) {
                xPos = x + bounds.width - child->getBounds().width;
            }

            child->setBounds(Rect(xPos, yPos, child->getBounds().width, child->getBounds().height));
            child->onDraw(ctx, xPos, yPos);
            
            switch(mainAxisAlignment) {
                case MainAxisAlignment::START:
                    yPos += child->getBounds().height;
                    break;
                case MainAxisAlignment::CENTER:
                    yPos += (child->getBounds().height / 2);
                    break;
                case MainAxisAlignment::END:
                    yPos -= child->getBounds().height;
                    break;
                case MainAxisAlignment::SPACE_BETWEEN:
                    yPos += (bounds.height - child->getBounds().height) / (children.size() - 1);
                    break;
                case MainAxisAlignment::SPACE_AROUND:
                    yPos += (bounds.height - child->getBounds().height) / (children.size() + 1);
                    break;
            }
        }
    }

    void onUpdate(float delta) override {
        for (const auto& child : children) {
            child->onUpdate(delta);
        }
    }

protected:
    std::vector<std::shared_ptr<Drawable>> children;
    MainAxisAlignment mainAxisAlignment;
    CrossAxisAlignment crossAxisAlignment;
    Rect bounds;
};

#define Column(...) std::make_shared<D_Column>(ColumnProperties{__VA_ARGS__})
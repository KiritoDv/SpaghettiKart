#include "HMUI.h"

void HMUI::initialize() {
    context = std::make_unique<N64GraphicsContext>();
    context->init();
}

void HMUI::show(std::shared_ptr<IView> view){
    if (view) {
        this->view = view;
        this->view->init();
        this->drawable = this->view->build();

        if(drawable == nullptr) {
            throw std::runtime_error("Drawable cannot be null");
        }

        drawable->setView(view);
        drawable->init();
    } else {
        throw std::invalid_argument("View cannot be null");
    }
}

void HMUI::draw(GfxList** out) {
    std::shared_ptr<IView> current = this->view;

    if (current == nullptr) {
        return;
    }

    if(this->drawable == nullptr) {
        throw std::runtime_error("Drawable cannot be null");
    }

    this->drawable->onDraw(context.get(), 0, 0);
}

void HMUI::update(float delta){
    std::shared_ptr<IView> current = this->view;

    if (current == nullptr) {
        return;
    }

    if(this->drawable == nullptr) {
        throw std::runtime_error("Drawable cannot be null");
    }

    this->drawable->onUpdate(delta);
}

void HMUI::close(){
    if (!this->view) {
        return;
    }

    if(this->drawable == nullptr) {
        throw std::runtime_error("Drawable cannot be null");
    }

    this->drawable->dispose();

    this->view->dispose();
    this->drawable = nullptr;
    this->view = nullptr;
}
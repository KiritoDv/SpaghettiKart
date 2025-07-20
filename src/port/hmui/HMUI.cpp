#include "HMUI.h"

#include "port/Engine.h"
#include "port/hmui/widgets/Drawable.h"
#include "port/hmui/graphics/N64GraphicsContext.h"

void HMUI::initialize(std::shared_ptr<GraphicsContext> ctx) {
    this->context = ctx;
    this->context->init();
}

void HMUI::show(std::shared_ptr<IView> _view){
    if (_view) {
        this->view = _view;
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

    auto interpreter = GetInterpreter();
    auto dimensions = interpreter->mCurDimensions;

    this->drawable->setBounds(Rect(0, 0, dimensions.width, dimensions.height));
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
HMUI::~HMUI() {
    this->context->dispose();
    this->context = nullptr;
}

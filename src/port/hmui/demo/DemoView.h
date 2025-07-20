#pragma once

#include "port/hmui/view/IView.h"
#include "port/hmui/widgets/Column.h"
#include "port/hmui/widgets/Container.h"

class DemoView : public IView {
public:
    std::shared_ptr<Drawable> build() override {
        // Example of building a simple UI with a column and a container
        return Container(
            .width = 300,
            .height = 200,
            .padding = EdgeInsets::all(10),
            .color = Color2D(0.8f, 0.8f, 0.8f),
            .child = Column(
                .children = {
                    Container(
                        .width = 280,
                        .height = 50,
                        .padding = EdgeInsets::all(5),
                        .color = Color2D(1.0f, 0.5f, 0.5f),
                    ),
                    Container(
                        .width = 280,
                        .height = 50,
                        .padding = EdgeInsets::all(5),
                        .color = Color2D(0.5f, 1.0f, 0.5f),
                    ),
                    Container(
                        .width = 280,
                        .height = 50,
                        .padding = EdgeInsets::all(5),
                        .color = Color2D(0.5f, 0.5f, 1.0f),
                    ),
                }
            )
        );
    }
};
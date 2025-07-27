#pragma once

#include <libultraship/libultraship.h>
#include "UIWidgets.h"
#include "Menu.h"
#include "Fast3D/backends/gfx_rendering_api.h"

namespace GameUI {

class PortMenu;

class Net64Menu {
public:
    void AddTabs(PortMenu* menu);

private:
    void AddRegisterTab(PortMenu* menu);
    void AddAccountTab(PortMenu* menu);
};
}
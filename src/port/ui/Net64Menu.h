#pragma once

#include <vector>

#include "Menu.h"
#include "UIWidgets.h"
#include "Fast3D/backends/gfx_rendering_api.h"
#include <libultraship/libultraship.h>

namespace GameUI {

class PortMenu;

class Net64Menu {
public:
    void AddTabs(PortMenu* menu);
    void AddAuthTabs(PortMenu* menu);

private:
    void AddRegisterTab(PortMenu* menu);
};
}
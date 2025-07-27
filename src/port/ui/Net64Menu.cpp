#include "Net64Menu.h"
#include "PortMenu.h"
#include "UIWidgets.h"
#include "ImguiUI.h"

#include "port/Engine.h"

using namespace GameUI;
using namespace UIWidgets;

static char mCodeBuf[7] = { 0 };

namespace GameUI {
extern std::shared_ptr<PortMenu> mPortMenu;
extern std::shared_ptr<Net64Menu> mNet64Menu;
};

SatellaApi* api;

void Net64Menu::AddRegisterTab(PortMenu* menu) {
    WidgetPath path = { "Net64", "Register", SECTION_COLUMN_1 };
    menu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);

    menu->AddWidget(path, "Auth Code:", WIDGET_TEXT);

    menu->AddWidget(path, "##LinkCodeInput", WIDGET_INPUT_TEXT)
        .CVar("gNet64.LinkCode")
        .Options(InputTextOptions(mCodeBuf, (sizeof(mCodeBuf) / sizeof(mCodeBuf)[0]), 0, "123456"));

    menu->AddWidget(path, "Link Account", WIDGET_BUTTON)
        .Options(UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))
        .Callback([](WidgetInfo& info) {
            api->LinkAccount(std::string(mCodeBuf), DeviceType::MAC, [&](const SatellaResponse& _) {
                api->SyncUser([&](const SatellaResponse& response) {
                    if(response.isValid) {
                        GameEngine::Instance->context->GetLogger()->info("Successfully linked account: {}", response.message);
                        mPortMenu->RemoveSidebarEntry("Net64", "Register");
                        mNet64Menu->AddAccountTab(mPortMenu.get());
                    } else {
                        GameEngine::Instance->context->GetLogger()->error("Error linking account: {}", response.message);
                    }
                });
            });
        });
}

void Net64Menu::AddAccountTab(PortMenu* menu){
    auto user = GameEngine::Instance->gSatellaApi->GetUser();

    if(user == nullptr){
        return;
    }

    WidgetPath path = { "Net64", "Account", SECTION_COLUMN_1 };
    menu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);
    menu->AddWidget(path, "ULID: " + user->ulid, WIDGET_TEXT);
    menu->AddWidget(path, "Username: " + user->username, WIDGET_TEXT);
}

void Net64Menu::AddTabs(PortMenu* menu){
    api = GameEngine::Instance->gSatellaApi;
    menu->AddMenuEntry("Net64", "gSettings.Menu.Net64.RegisterTab");
    if(api->GetUser() == nullptr) {
        AddRegisterTab(menu);
    } else {
        AddAccountTab(menu);
    }
}
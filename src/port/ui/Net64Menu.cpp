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
                        mNet64Menu->AddAuthTabs(mPortMenu.get());
                    } else {
                        GameEngine::Instance->context->GetLogger()->error("Error linking account: {}", response.message);
                    }
                });
            });
        });
}

ImU32 ParseHexColor(const std::string& hex, float alpha = 0.4f) {
    unsigned int rgba = 0xFFFFFFFF;

    if (hex.length() == 7 && hex[0] == '#') {
        // #RRGGBB
        sscanf(hex.c_str() + 1, "%06x", &rgba);
        rgba = (rgba << 8) | 0xFF; // Add full alpha
    } else if (hex.length() == 9 && hex[0] == '#') {
        // #RRGGBBAA
        sscanf(hex.c_str() + 1, "%08x", &rgba);
    } else {
        return IM_COL32_WHITE; // Fallback
    }

    unsigned char r = (rgba >> 24) & 0xFF;
    unsigned char g = (rgba >> 16) & 0xFF;
    unsigned char b = (rgba >> 8) & 0xFF;
    unsigned char a = rgba & 0xFF;

    // Apply override alpha if valid
    if (alpha >= 0.0f && alpha <= 1.0f) {
        a = static_cast<unsigned char>(alpha * 255.0f);
    }

    return IM_COL32(r, g, b, a);
}

void DrawFriendCard(WidgetInfo& info, const User& user) {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    float imageSize = 80;
    ImVec2 cardSize = ImVec2(imageSize + 20, imageSize + 60); // Card size with padding
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImU32 accent = ParseHexColor(user.accentColor);
    draw_list->AddRectFilled(
        cursorPos,
        ImVec2(cursorPos.x + cardSize.x, cursorPos.y + cardSize.y),
        accent,
        2.0f
    );

    // Start inner group
    ImGui::BeginGroup();
    ImGui::SetCursorScreenPos(cursorPos);

    // Centered image
    float availableWidth = cardSize.x;
    float imageOffsetX = (availableWidth - imageSize) / 2.0f;
    const char* name = user.alias.c_str();

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + imageOffsetX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
    ImGui::Image((ImTextureID) gui->GetTextureByName(user.ulid), ImVec2(imageSize, imageSize));
    ImGui::Dummy(ImVec2(0, 0));

    // Space and centered text
    ImGui::Spacing();
    ImGui::SetCursorPosX((ImGui::GetCursorPosX() + (cardSize.x / 2)) - (ImGui::CalcTextSize(name).x / 2.0f));
    ImGui::Text("%s", name);

    // Reserve space so other widgets don't overlap
    ImGui::Dummy(cardSize);
    ImGui::EndGroup();
}

void Net64Menu::AddAuthTabs(PortMenu* menu){
    auto user = GameEngine::Instance->gSatellaApi->GetUser();

    if(user == nullptr){
        return;
    }

    WidgetPath path = { "Net64", "Account", SECTION_COLUMN_1 };
    menu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);
    menu->AddWidget(path, "ULID: " + user->ulid, WIDGET_TEXT);
    menu->AddWidget(path, "Username: " + user->username, WIDGET_TEXT);

    path = { "Net64", "Friends", SECTION_COLUMN_1 };
    menu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);

    menu->AddWidget(path, "Search Friends", WIDGET_TEXT);
    menu->AddWidget(path, "##SearchFriendsInput", WIDGET_INPUT_TEXT)
        .Options(InputTextOptions("", 64, 0, "Search by username"));

    menu->AddWidget(path, "Friends List", WIDGET_TEXT);

    api->GetFriends([&](const SatellaResponse& response) {
        if (response.isValid) {
            auto friends = api->GetFriends();
            if (friends && !friends->empty()) {
                menu->AddWidget(path, "FriendsCard", WIDGET_CUSTOM)
                    .CustomFunction([friends](WidgetInfo& info) {
                        for (auto& user : *friends) {
                            ImGui::SameLine();
                            DrawFriendCard(info, user);
                        }
                    });
            } else {
                menu->AddWidget(path, "No friends found.", WIDGET_TEXT);
            }
        } else {
            GameEngine::Instance->context->GetLogger()->error("Error fetching friends: {}", response.message);
        }
    });
}

void Net64Menu::AddTabs(PortMenu* menu){
    api = GameEngine::Instance->gSatellaApi;
    menu->AddMenuEntry("Net64", "gSettings.Menu.Net64.RegisterTab");
    if(api->GetUser() == nullptr) {
        AddRegisterTab(menu);
    } else {
        AddAuthTabs(menu);
    }
}
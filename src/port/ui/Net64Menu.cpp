#include "Net64Menu.h"
#include "PortMenu.h"
#include "UIWidgets.h"
#include "ImguiUI.h"

#include "port/Engine.h"
#include "port/satella/SatellaCache.h"

using namespace GameUI;
using namespace UIWidgets;

static char mCodeBuf[6 + 1] = { 0 };
static char mPakName[16 + 1] = { 0 };
static char mSearchBuf[64 + 1] = { 0 };
static std::vector<User> mSearchResults;
static VirtualControllerPak* mSelectedPak;
static SatellaApi* api;

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

namespace GameUI {
extern std::shared_ptr<PortMenu> mPortMenu;
extern std::shared_ptr<Net64Menu> mNet64Menu;
};

ImU32 ParseHexColor(int rgb, float alpha = 0.4f) {
    unsigned char r = (rgb >> 16) & 0xFF;
    unsigned char g = (rgb >> 8) & 0xFF;
    unsigned char b = rgb & 0xFF;
    unsigned char a = 255 * alpha;

    return IM_COL32(r, g, b, a);
}

void ParseHexColorFloat(int rgb, float* output) {
    unsigned char r = (rgb >> 16) & 0xFF;
    unsigned char g = (rgb >> 8) & 0xFF;
    unsigned char b = rgb & 0xFF;

    output[0] = r / 255.0f;
    output[1] = g / 255.0f;
    output[2] = b / 255.0f;
    output[3] = 1.0f;
}

std::string toHex(int value) {
    std::stringstream ss;
    ss << std::hex << value; // use std::hex for hex format
    return ss.str();
}

#ifndef __SWITCH__
void LaunchBrowser(const std::string& url) {
#if defined(_WIN32)
    std::string command = "start " + url;
#elif defined(__APPLE__)
    std::string command = "open " + url;
#else // Linux and others
    std::string command = "xdg-open " + url;
#endif
    system(command.c_str());
}
#endif

enum class FriendCardType {
    Search,
    Pending,
    Friend
};

void DrawFriendCard(User& user, FriendCardType type) {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    float imageSize = 80;
    const char* name = user.alias.c_str();
    float textWidth = ImGui::CalcTextSize(name).x;

    api->DownloadAvatar(user);

    ImVec2 cardSize = ImVec2(std::max(textWidth + 20.0f, imageSize + 20.0f), imageSize + 60 + (type == FriendCardType::Search ? 30 : 0)); // Card size with padding
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
    ImGui::PushID(user.ulid.c_str());
    ImGui::BeginGroup();
    ImGui::SetCursorScreenPos(cursorPos);

    // Centered image
    float availableWidth = cardSize.x;
    float imageOffsetX = (availableWidth - imageSize) / 2.0f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + imageOffsetX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
    ImGui::Image((ImTextureID) gui->GetTextureByName(user.ulid), ImVec2(imageSize, imageSize));

    // Space and centered text
    ImGui::Spacing();
    ImGui::SetCursorPosX((ImGui::GetCursorPosX() + (cardSize.x / 2)) - (textWidth / 2.0f));
    ImGui::Text("%s", name);

    // Add buttons for search results
    if (type == FriendCardType::Search) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Add")) {
            api->AddFriend(user, [user](const SatellaResponse& response) {
                if (response.isValid) {
                    GameEngine::Instance->context->GetLogger()->info("Friend request sent to {}", user.username);
                    mSearchResults.clear();
                    memset(mSearchBuf, 0, sizeof(mSearchBuf));
                } else {
                    GameEngine::Instance->context->GetLogger()->error("Error sending friend request: {}", response.message);
                }
            });
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Block")) {
            GameEngine::Instance->context->GetLogger()->info("Blocked user {}", user.username);
        }
        ImGui::PopStyleColor();
    }

    ImGui::SetCursorScreenPos(cursorPos);
    ImGui::Dummy(cardSize);
    ImGui::SetCursorScreenPos(cursorPos);
    // ImVec2 pmax = ImGui::GetItemRectMax();
    // draw_list->AddRect(cursorPos, pmax, IM_COL32(255, 0, 0, 255));

    if(type != FriendCardType::Search && (ImGui::IsMouseHoveringRect(cursorPos, ImVec2(cursorPos.x + cardSize.x, cursorPos.y + cardSize.y)))) {
        draw_list->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + cardSize.x, cursorPos.y + cardSize.y),
            IM_COL32(0, 0, 0, 60),
            2.0f
        );

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
        switch(user.status) {
            case FriendRequestStatus::ACCEPTED: {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Remove")) {
                    GameEngine::Instance->context->GetLogger()->info("Removing friend {}", user.username);
                    api->RemoveFriend(user.ulid, [user](const SatellaResponse& response) {
                        if (response.isValid) {
                            GameEngine::Instance->context->GetLogger()->info("Removed friend {}", user.username);
                        } else {
                            GameEngine::Instance->context->GetLogger()->error("Error removing friend: {}", response.message);
                        }
                    });
                }
                ImGui::PopStyleColor();
                break;
            }
            case FriendRequestStatus::SENT: {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                if (ImGui::Button("Cancel")) {
                    api->ModifyFriendRequest(user.ulid, false, [user](const SatellaResponse& response) {
                        if (response.isValid) {
                            GameEngine::Instance->context->GetLogger()->info("Cancelled friend request to {}", user.username);
                        } else {
                            GameEngine::Instance->context->GetLogger()->error("Error cancelling friend request: {}", response.message);
                        }
                    });
                }
                ImGui::PopStyleColor();
                break;
            }
            case FriendRequestStatus::RECEIVED: {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
                if (ImGui::Button("Accept")) {
                    api->ModifyFriendRequest(
                        user.ulid, true, [user](const SatellaResponse& response) {
                            if (response.isValid) {
                                GameEngine::Instance->context->GetLogger()->info("Accepted friend request from {}",
                                                                                 user.username);
                            } else {
                                GameEngine::Instance->context->GetLogger()->error("Error accepting friend request: {}",
                                                                                  response.message);
                            }
                        });
                }
                ImGui::PopStyleColor();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Decline")) {
                    api->ModifyFriendRequest(
                        user.ulid, false, [user](const SatellaResponse& response) {
                            if (response.isValid) {
                                GameEngine::Instance->context->GetLogger()->info("Declined friend request from {}",
                                                                                 user.username);
                            } else {
                                GameEngine::Instance->context->GetLogger()->error("Error declining friend request: {}",
                                                                                  response.message);
                            }
                        });
                }
                ImGui::PopStyleColor();
                break;
            }
        }
    }

    ImGui::EndGroup();
    ImGui::PopID();
}

void DrawControllerPakCard(VirtualControllerPak& pak) {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    ImVec2 imageSize(128, 97);
    const char* name = pak.name.c_str();
    float textWidth = ImGui::CalcTextSize(name).x;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    auto user = api->GetUser();
    auto current = api->GetCurrentPak();
    bool isInserted = current && current->pakId == pak.pakId;
    bool isGuest = pak.ownerId != user->ulid;

    ImVec2 pakSize = ImVec2(imageSize.x, imageSize.y); // Card size with padding
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    auto image = isGuest ?
        isInserted ? "satella/ControllerPak-Inserted-Guest.png" : "satella/ControllerPak-Exists-Guest.png" :
        isInserted ? "satella/ControllerPak-Inserted.png" : "satella/ControllerPak-Exists.png";
    SatellaCache::LoadPNG(image);

    // Start inner group
    ImGui::BeginGroup();
    ImGui::SetCursorScreenPos(cursorPos);
    ImGui::Dummy(imageSize);
    ImGui::SetCursorScreenPos(cursorPos);

#ifdef DEBUG_WIDGET
    ImVec2 pmax = ImGui::GetItemRectMax();
#endif

    // Centered image
    ImGui::Image((ImTextureID) gui->GetTextureByName(image), imageSize);
    ImGui::SetCursorScreenPos(cursorPos);

    if(!isInserted) {
        // Space and centered text
        ImGui::SetCursorPosX((ImGui::GetCursorPosX() + (pakSize.x / 2)) - (textWidth / 2.0f));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
        // ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 70);
        ImGui::Text("%s", name);
        // ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
#ifdef DEBUG_WIDGET
    draw_list->AddRect(cursorPos, pmax, IM_COL32(255, 0, 0, 255));
#endif

    ImGui::SetCursorScreenPos(cursorPos);
    if(ImGui::IsMouseHoveringRect(cursorPos, ImVec2(cursorPos.x + pakSize.x, cursorPos.y + pakSize.y))) {
        draw_list->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + pakSize.x, cursorPos.y + pakSize.y),
            IM_COL32(0, 0, 0, 60),
            10.0f
        );

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
        if (isInserted){
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.263f, 0.263f, 0.443f, 1.0f));
            if (ImGui::Button("Sync")) {
                if(isGuest) {
                    api->InsertPak(pak.pakId, [pak](const SatellaResponse& response) {
                        if (response.isValid) {
                            GameEngine::Instance->context->GetLogger()->info("Synced Controller Pak: {}", pak.name);
                        } else {
                            GameEngine::Instance->context->GetLogger()->error("Error syncing Controller Pak: {}", response.message);
                        }
                    });
                } else {
                    api->UploadPak(pak.pakId, [pak](const SatellaResponse& response) {
                        if (response.isValid) {
                            GameEngine::Instance->context->GetLogger()->info("Synced Controller Pak: {}", pak.name);
                        } else {
                            GameEngine::Instance->context->GetLogger()->error("Error syncing Controller Pak: {}", response.message);
                        }
                    });
                }
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();
        }
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button(isInserted ? "Eject" : "Insert")) {
            if (isInserted) {
                if(!isGuest){
                    api->UploadPak(pak.pakId, [pak](const SatellaResponse& response) {
                        if (response.isValid) {
                            GameEngine::Instance->context->GetLogger()->info("Inserted Controller Pak: {}", pak.name);
                        } else {
                            GameEngine::Instance->context->GetLogger()->error("Error inserting Controller Pak: {}", response.message);
                        }
                    });
                }
                api->EjectPak();
            } else {
                api->InsertPak(pak.pakId, [pak](const SatellaResponse& response) {
                    if (response.isValid) {
                        GameEngine::Instance->context->GetLogger()->info("Inserted Controller Pak: {}", pak.name);
                        api->SaveSession();
                    } else {
                        GameEngine::Instance->context->GetLogger()->error("Error inserting Controller Pak: {}", response.message);
                    }
                });
            }
        }
        ImGui::PopStyleColor();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.2f, 1.0f));
        if (!isGuest && ImGui::Button("Options")) {
            if(mSelectedPak != &pak) {
                mSelectedPak = &pak;
                memcpy(mPakName, pak.name.c_str(), std::min(sizeof(mPakName) - 1, pak.name.size()));
                mPakName[sizeof(mPakName) - 1] = '\0';
            } else {
                mSelectedPak = nullptr;
            }
            ImGui::OpenPopup(("Controller Pak Options##CPO" + pak.pakId).c_str());
        }
        ImGui::PopStyleColor();
        if(!isInserted && !isGuest) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Delete")) {
                api->DeletePak(pak.pakId, [pak](const SatellaResponse& response) {
                    if (response.isValid) {
                        GameEngine::Instance->context->GetLogger()->info("Deleted Controller Pak: {}", pak.name);
                    } else {
                        GameEngine::Instance->context->GetLogger()->error("Error deleting Controller Pak: {}", response.message);
                    }
                });
            }
            ImGui::PopStyleColor();
        }
    }

    if(ImGui::BeginPopup(("Controller Pak Options##CPO" + pak.pakId).c_str())) {
        ImGui::Text("Name:");
        ImGui::PushItemWidth(250);
        if(ImGui::InputText("##CPOName", mPakName, ARRAY_SIZE(mPakName))){
            mSelectedPak->name = std::string(mPakName);
        }
        ImGui::PopItemWidth();
        // List friends to handle access
        ImGui::Text("Allow Access:");
        ImGui::BeginChild("##PakAccessList", ImVec2(0, 100), true);
        bool hasFriends = false;
        for (const auto& friendUser : *api->GetFriends()) {
            if(friendUser.status != FriendRequestStatus::ACCEPTED) {
                continue;
            }

            bool hasAccess = std::find(pak.access.begin(), pak.access.end(), friendUser.ulid) != pak.access.end();
            ImGui::PushID(friendUser.ulid.c_str());
            if (ImGui::Checkbox(friendUser.alias.c_str(), &hasAccess)) {
                if (hasAccess) {
                    pak.access.push_back(friendUser.ulid);
                } else {
                    pak.access.erase(std::remove(pak.access.begin(), pak.access.end(), friendUser.ulid), pak.access.end());
                }
            }
            ImGui::PopID();
            hasFriends = true;
        }
        if (!hasFriends) {
            ImGui::Text("No friends available.");
        }
        ImGui::EndChild();
        if(ImGui::Button("Save")) {
            api->UpdatePak(pak, [](const SatellaResponse& response) {
                if(response.isValid) {
                    GameEngine::Instance->context->GetLogger()->info("Controller Pak updated successfully.");
                } else {
                    GameEngine::Instance->context->GetLogger()->error("Error updating Controller Pak: {}", response.message);
                }
            });
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::EndGroup();
}

void DrawEmptyControllerPakCard() {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    ImVec2 imageSize(128, 97);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 pakSize = ImVec2(imageSize.x, imageSize.y);
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    auto image = "satella/ControllerPak-Empty.png";
    SatellaCache::LoadPNG(image);

    // Start inner group
    ImGui::BeginGroup();
    ImGui::SetCursorScreenPos(cursorPos);
    ImGui::Dummy(imageSize);
    ImGui::SetCursorScreenPos(cursorPos);

#ifdef DEBUG_WIDGET
    ImVec2 pmax = ImGui::GetItemRectMax();
#endif

    // Centered image
    ImGui::Image((ImTextureID) gui->GetTextureByName(image), imageSize);
    ImGui::SetCursorScreenPos(cursorPos);

#ifdef DEBUG_WIDGET
    draw_list->AddRect(cursorPos, pmax, IM_COL32(255, 0, 0, 255));
#endif

    ImGui::SetCursorScreenPos(cursorPos);
    if(ImGui::IsMouseHoveringRect(cursorPos, ImVec2(cursorPos.x + pakSize.x, cursorPos.y + pakSize.y))) {
        draw_list->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + pakSize.x, cursorPos.y + pakSize.y),
            IM_COL32(0, 0, 0, 60),
            10.0f
        );
    }

    if(ImGui::IsItemClicked()) {
        api->CreatePak([](const SatellaResponse& response) {
            if(response.isValid) {
                GameEngine::Instance->context->GetLogger()->info("Controller Pak created successfully.");
            } else {
                GameEngine::Instance->context->GetLogger()->error("Error creating Controller Pak: {}", response.message);
            }
        });
    }

    ImGui::EndGroup();
}

void DrawUpdateTitle(std::string title, std::function<void()> callback) {
    float baseY = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 7);
    ImGui::Text(title.c_str());
    ImGui::SameLine();
    ImGui::SetCursorPosY(baseY);
    UIWidgets::ButtonOptions options = {};
    options.color = UIWidgets::Colors::Indigo;
    options.size = UIWidgets::Sizes::Inline;
    options.tooltip = "Sync";
    if (UIWidgets::Button(ICON_FA_UNDO, options)) {
        if (callback) {
            callback();
        }
    }
}

void Net64Menu::AddRegisterTab() {
    WidgetPath path = { "Net64", "Register", SECTION_COLUMN_1 };
    mPortMenu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);

    mPortMenu->AddWidget(path, "Auth Code:", WIDGET_TEXT);

    mPortMenu->AddWidget(path, "##LinkCodeInput", WIDGET_INPUT_TEXT)
        .CVar("gNet64.LinkCode")
        .Options(InputTextOptions(mCodeBuf, ARRAY_SIZE(mCodeBuf), 0, "123456"));

    mPortMenu->AddWidget(path, "AccountActions", WIDGET_CUSTOM)
        .CustomFunction([](WidgetInfo& info) {
            UIWidgets::ButtonOptions options = {};
            options.color = UIWidgets::Colors::Indigo;
            options.size = UIWidgets::Sizes::Inline;
            options.tooltip = "Sync";
            if (UIWidgets::Button("Link Account", options)) {
                memset(mCodeBuf, 0, sizeof(mCodeBuf));
                api->LinkAccount(std::string(mCodeBuf), DeviceType::MAC, [&](const SatellaResponse& _) {
                api->SyncUser([&](const SatellaResponse& response) {
                        if(response.isValid) {
                            GameEngine::Instance->context->GetLogger()->info("Successfully linked account: {}", response.message);
                            mPortMenu->RemoveSidebarEntry("Net64", "Register");
                            mNet64Menu->AddAuthTabs();
                        } else {
                            GameEngine::Instance->context->GetLogger()->error("Error linking account: {}", response.message);
                        }
                    });
                });
            }
        #ifndef __SWITCH__
            ImGui::SameLine();
            if (UIWidgets::Button("Register", options)) {
                LaunchBrowser(api->GetAuthURL());
            }
        #endif
        });
}

void Net64Menu::AddAccountTab(){
    auto user = api->GetUser();

    if(user == nullptr){
        return;
    }

    auto avatar = Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(user->ulid);

    WidgetPath path = { "Net64", "Account", SECTION_COLUMN_1 };
    mPortMenu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);
    if(avatar != nullptr){
        mPortMenu->AddWidget(path, "Avatar:", WIDGET_TEXT);
        mPortMenu->AddWidget(path, "##AvatarImage", WIDGET_CUSTOM)
            .CustomFunction([avatar](WidgetInfo& info) {
                ImGui::Image((ImTextureID) avatar, ImVec2(80, 80));
            });
    }

    mPortMenu->AddWidget(path, "ULID: " + user->ulid, WIDGET_TEXT);
    mPortMenu->AddWidget(path, "Username: " + user->username, WIDGET_TEXT);
    mPortMenu->AddWidget(path, "Alias: " + user->alias, WIDGET_TEXT);
    mPortMenu->AddWidget(path, "AccentColor", WIDGET_CUSTOM)
        .CustomFunction([user](WidgetInfo& info) {
            float accent[4];
            ParseHexColorFloat(user->accentColor, accent);
            ImGui::Text("Accent Color:");
            ImGui::SameLine();
            ImGui::ColorEdit3("##AccentColor", accent, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            // if (ImGui::IsItemDeactivatedAfterEdit()) {
                // api->UpdateUser(user, [&](const SatellaResponse& response) {
                //     if (response.isValid) {
                //         GameEngine::Instance->context->GetLogger()->info("Accent color updated successfully.");
                //     } else {
                //         GameEngine::Instance->context->GetLogger()->error("Error updating accent color: {}", response.message);
                //     }
                // });
            // }
        });
    mPortMenu->AddWidget(path, "Favorite Games:", WIDGET_TEXT);
    if (!user->favoriteGames.empty()) {
        for (const auto& game : user->favoriteGames) {
            mPortMenu->AddWidget(path, game, WIDGET_TEXT);
        }
    } else {
        mPortMenu->AddWidget(path, "No favorite games set.", WIDGET_TEXT);
    }

    mPortMenu->AddWidget(path, "Logout", WIDGET_BUTTON)
        .Options(UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))
        .Callback([](WidgetInfo& info) {
            api->Logout([&](const SatellaResponse& response) {
                if (response.isValid) {
                    GameEngine::Instance->context->GetLogger()->info("Logged out successfully.");
                    mPortMenu->RemoveSidebarEntry("Net64", "Account");
                    mPortMenu->RemoveSidebarEntry("Net64", "Friends");
                    mPortMenu->RemoveSidebarEntry("Net64", "Controller Pak");
                    mNet64Menu->AddRegisterTab();
                } else {
                    GameEngine::Instance->context->GetLogger()->error("Error logging out: {}", response.message);
                }
            });
        });
}

void Net64Menu::AddFriendsTab() {
    WidgetPath path = { "Net64", "Friends", SECTION_COLUMN_1 };
    mPortMenu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);

    mPortMenu->AddWidget(path, "FriendsTitle", WIDGET_CUSTOM)
        .CustomFunction([](WidgetInfo& info) {
            DrawUpdateTitle("Friends", []() {
                api->GetFriends([](const SatellaResponse& response) {
                    if (response.isValid) {
                        GameEngine::Instance->context->GetLogger()->info("Friends list updated successfully.");
                    } else {
                        GameEngine::Instance->context->GetLogger()->error("Error updating friends list: {}", response.message);
                    }
                });
            });
        });
    
    mPortMenu->AddWidget(path, "Search Users", WIDGET_TEXT);
    mPortMenu->AddWidget(path, "##SearchFriendsInput", WIDGET_INPUT_TEXT)
        .Options(InputTextOptions(mSearchBuf, ARRAY_SIZE(mSearchBuf), 0, "Search by username"))
        .PostFunc([](WidgetInfo& info) {
            // Draw the search results if any
            if (!mSearchResults.empty()) {
                ImGui::BeginChild("SearchResults", ImVec2(0, 200), true);
                for (auto& user : mSearchResults) {
                    ImGui::PushID(user.ulid.c_str());
                    DrawFriendCard(user, FriendCardType::Search);
                    ImGui::PopID();
                }
                ImGui::EndChild();
            }
        })
        .Callback([&](WidgetInfo& info) {
            if (strlen(mSearchBuf) > 1) {
                api->SearchFriends(std::string(mSearchBuf), [&](const std::vector<User>& response) {
                    if (!response.empty()) {
                        mSearchResults.clear();
                        mSearchResults = response;
                        GameEngine::Instance->context->GetLogger()->info("Search completed successfully - {} results found", response.size());
                    } else {
                        mSearchResults.clear();
                        GameEngine::Instance->context->GetLogger()->error("Error searching friends");
                    }
                });
            } else {
                mSearchResults.clear();
            }
        });

    api->GetFriends([&](const SatellaResponse& response) {
        if (response.isValid) {
            mPortMenu->AddWidget(path, "Friends List", WIDGET_TEXT);
            auto friends = api->GetFriends();

            mPortMenu->AddWidget(path, "FriendsCard", WIDGET_CUSTOM)
                .CustomFunction([friends](WidgetInfo& info) {
                    bool hasFriends = false;
                    for (auto& user : *friends) {
                        if(user.status != FriendRequestStatus::ACCEPTED) {
                            continue;
                        }
                        ImGui::SameLine();
                        DrawFriendCard(user, FriendCardType::Friend);
                        hasFriends = true;
                    }

                    if (!hasFriends) {
                        ImGui::Text("No friends found.");
                    }
                });

            mPortMenu->AddWidget(path, "Pending Friend Requests", WIDGET_TEXT);

            mPortMenu->AddWidget(path, "PendingFriendsCard", WIDGET_CUSTOM)
                .CustomFunction([friends](WidgetInfo& info) {
                    bool hasPendingFriends = false;
                    for (auto& user : *friends) {
                        if(user.status == FriendRequestStatus::ACCEPTED) {
                            continue;
                        }
                        ImGui::SameLine();
                        DrawFriendCard(user, FriendCardType::Pending);
                        hasPendingFriends = true;
                    }

                    if (!hasPendingFriends) {
                        ImGui::Text("No pending friend requests.");
                    }
                });
        } else {
            GameEngine::Instance->context->GetLogger()->error("Error fetching friends: {}", response.message);
        }
    });
}

void Net64Menu::AddControllerPaksTab() {
    WidgetPath path = { "Net64", "Controller Pak", SECTION_COLUMN_1 };
    mPortMenu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);
    auto user = api->GetUser();
    api->ListPaks([&](const SatellaResponse& response) {
        mPortMenu->AddWidget(path, "ControllerPaksListTitle", WIDGET_CUSTOM)
            .CustomFunction([](WidgetInfo& info) {
                DrawUpdateTitle("Controller Paks", []() {
                    api->ListPaks([](const SatellaResponse& response) {
                        if (response.isValid) {
                            GameEngine::Instance->context->GetLogger()->info("Controller Paks updated successfully.");
                        } else {
                            GameEngine::Instance->context->GetLogger()->error("Error updating Controller Paks: {}", response.message);
                        }
                    });
                });
            });
        if (response.isValid) {
            auto paks = api->GetPaks();
            auto user = api->GetUser();
            mPortMenu->AddWidget(path, "ControllerPaks", WIDGET_CUSTOM)
                .CustomFunction([paks](WidgetInfo& info) {
                    size_t count = 0;
                    for(auto& pak : *paks) {
                        if(pak.ownerId == api->GetUser()->ulid) {
                            count++;
                        }
                        ImGui::SameLine();
                        DrawControllerPakCard(pak);
                    }

                    if(count < 3){
                        ImGui::SameLine();
                        DrawEmptyControllerPakCard();
                    }
                });
        } else {
            GameEngine::Instance->context->GetLogger()->error("Error fetching controller paks: {}", response.message);
        }
    });
}

void Net64Menu::AddAuthTabs() {
    AddAccountTab();
    AddFriendsTab();
    AddControllerPaksTab();
}

void Net64Menu::AddTabs(){
    api = GameEngine::Instance->gSatellaApi;
    mPortMenu->AddMenuEntry("Net64", "gSettings.Menu.Net64.RegisterTab");
    if(api->GetUser() == nullptr) {
        AddRegisterTab();
    } else {
        AddAuthTabs();
    }
}
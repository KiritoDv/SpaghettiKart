#include "Net64Menu.h"
#include "PortMenu.h"
#include "UIWidgets.h"
#include "ImguiUI.h"

#include "port/Engine.h"

using namespace GameUI;
using namespace UIWidgets;

static char mCodeBuf[7] = { 0 };
static char mSearchBuf[64] = { 0 };
static std::vector<User> mSearchResults;
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
            GameEngine::Instance->gSatellaApi->AddFriend(user, [user](const SatellaResponse& response) {
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
                    GameEngine::Instance->gSatellaApi->RemoveFriend(user.ulid, [user](const SatellaResponse& response) {
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
                    GameEngine::Instance->gSatellaApi->ModifyFriendRequest(user.ulid, false, [user](const SatellaResponse& response) {
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
                    GameEngine::Instance->gSatellaApi->ModifyFriendRequest(
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
                    GameEngine::Instance->gSatellaApi->ModifyFriendRequest(
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

void Net64Menu::AddRegisterTab() {
    WidgetPath path = { "Net64", "Register", SECTION_COLUMN_1 };
    mPortMenu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);

    mPortMenu->AddWidget(path, "Auth Code:", WIDGET_TEXT);

    mPortMenu->AddWidget(path, "##LinkCodeInput", WIDGET_INPUT_TEXT)
        .CVar("gNet64.LinkCode")
        .Options(InputTextOptions(mCodeBuf, ARRAY_SIZE(mCodeBuf), 0, "123456"));

    mPortMenu->AddWidget(path, "Link Account", WIDGET_BUTTON)
        .Options(UIWidgets::ButtonOptions().Size(UIWidgets::Sizes::Inline))
        .Callback([](WidgetInfo& info) {
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
        });
}

void Net64Menu::AddAuthTabs(){
    auto user = GameEngine::Instance->gSatellaApi->GetUser();

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
                // GameEngine::Instance->gSatellaApi->UpdateUser(user, [&](const SatellaResponse& response) {
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

    path = { "Net64", "Friends", SECTION_COLUMN_1 };
    mPortMenu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);

    mPortMenu->AddWidget(path, "Search Friends", WIDGET_TEXT);
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

    path = { "Net64", "Controller Pak", SECTION_COLUMN_1 };
    mPortMenu->AddSidebarEntry(path.sectionName, path.sidebarName, 1);
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
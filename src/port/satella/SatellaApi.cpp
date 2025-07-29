#include "SatellaApi.h"

#include "port/Engine.h"
#include "SatellaCache.h"
#include <cpr/cpr.h>
#include <thread>

#define SATELLA_API_BASE_URL "http://localhost:8080/v1"
#define SATELLA_MAX_TIMEOUT 10 * 1000

#define SATELLA_API_KEY "change-it"
#define SATELLA_API_SECRET "change-it"

#ifdef __WIIU__
#define SSL_OPTIONS ,cpr::SslOptions({.ca_path = Ship::WiiU::GetCAPath()}),
#else
#define SSL_OPTIONS
#endif

#define DEFAULT_AUTH cpr::Header{ \
                        { "Authorization", "Bearer " + session->token }, \
                        { "x-refresh-token", session->refreshToken }, \
                        { "x-net64-key", SATELLA_API_KEY }, \
                        { "x-net64-secret", SATELLA_API_SECRET } \
                    }

void AsyncRequest(const std::function<void()>& func) {
    std::thread([func]() {
        try {
            func();
        } catch (const std::exception& e) {
            GameEngine::Instance->context->GetLogger()->error("Async request error: {}", e.what());
        }
    }).detach();
}

std::string SatellaApi::GetAuthURL() {
    return SATELLA_API_BASE_URL "/auth";
}

void SatellaApi::LinkAccount(std::string linkCode, DeviceType device, DefaultCallback callback) {
    cpr::Response response = cpr::Post(
        cpr::Url{ SATELLA_API_BASE_URL "/auth/link" },
        cpr::Body{json{
            { "linkCode", linkCode },
            { "deviceId", deviceNames[(int) device] }
        }.dump()},
        cpr::Header{{
            "Content-Type", "application/json"
        }},
        cpr::Timeout{ SATELLA_MAX_TIMEOUT }
        SSL_OPTIONS
    );

    if (response.status_code == (long)ResponseCodes::OK) {
        this->session = std::make_shared<AuthSession>(
            json::parse(response.text).get<AuthSession>()
        );
        SaveSession();
        callback({ ResponseCodes::OK, "Account linked successfully." });
    } else {
        callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
    }
}

void SatellaApi::SyncUser(DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    cpr::Response response = cpr::Get(
        cpr::Url{ SATELLA_API_BASE_URL "/user" },
        cpr::Timeout{ SATELLA_MAX_TIMEOUT },
        DEFAULT_AUTH
        SSL_OPTIONS
    );

    if (response.status_code == (long) ResponseCodes::OK) {
        this->user = std::make_shared<User>(json::parse(response.text).get<User>());
        DownloadAvatar(*user);
        callback({ ResponseCodes::OK, "User synced successfully." });
    } else {
        callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
    }
}

void SatellaApi::Logout(DefaultCallback callback) {
    session.reset();
    currentPak.reset();
    friends.reset();
    paks.reset();
    SaveSession();
    callback({ ResponseCodes::OK, "Logged out successfully." });
}

void SatellaApi::DownloadAvatar(const User& _user) {
    if (_user.avatar.empty()) {
        return;
    }

    if (SatellaCache::IsImageLoaded(_user.ulid)) {
        return;
    }

    cpr::Response response = cpr::Get(
        cpr::Url{ _user.avatar },
        cpr::Timeout{ SATELLA_MAX_TIMEOUT }
        SSL_OPTIONS
    );

    if (response.status_code == (long) ResponseCodes::OK) {
        auto data = std::vector<uint8_t>(response.text.begin(), response.text.end());
        SatellaCache::LoadAvatar(_user.ulid, data);
    } else {
        GameEngine::Instance->context->GetLogger()->error("Error downloading avatar: {}", response.text);
    }
}

void SatellaApi::GetFriends(DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    cpr::Response response = cpr::Get(
        cpr::Url{ SATELLA_API_BASE_URL "/friends" },
        cpr::Timeout{ SATELLA_MAX_TIMEOUT },
        DEFAULT_AUTH
        SSL_OPTIONS
    );

    if (response.status_code == (long) ResponseCodes::OK) {
        this->friends = std::make_shared<std::vector<User>>(json::parse(response.text).get<std::vector<User>>());
        callback({ ResponseCodes::OK, "Friends list retrieved successfully." });
    } else {
        callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
    }
}

void SatellaApi::SearchFriends(const std::string& query, Callback<std::vector<User>> callback) {
    if (!session || session->token.empty()) {
        callback(std::vector<User>{});
        return;
    }

    AsyncRequest([this, query, callback]() {
        cpr::Response response = cpr::Get(
            cpr::Url{ SATELLA_API_BASE_URL "/friends/search" },
            cpr::Parameters{{ "q", query }},
            cpr::Timeout{ SATELLA_MAX_TIMEOUT },
            DEFAULT_AUTH
            SSL_OPTIONS
        );

        if (response.status_code == (long) ResponseCodes::OK) {
            auto users = json::parse(response.text).get<std::vector<User>>();
            for (auto& friendUser : users) {
                DownloadAvatar(friendUser);
            }
            callback(users);
        } else {
            callback(std::vector<User>{});
            GameEngine::Instance->context->GetLogger()->error("Error searching friends: {}", response.text);
        }
    });
}

void SatellaApi::AddFriend(User& _user, DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    AsyncRequest([this, _user, callback]() {
        cpr::Response response = cpr::Post(
            cpr::Url{ SATELLA_API_BASE_URL "/friends/add" },
            cpr::Body{ json{{ "friendId", _user.ulid }}.dump() },
            cpr::Header{ { "Content-Type", "application/json" } },
            DEFAULT_AUTH
            SSL_OPTIONS
        );

        if (response.status_code == (long) ResponseCodes::OK) {
            if(friends) {
                User copy = _user;
                copy.status = FriendRequestStatus::SENT;
                friends->push_back(copy);
            }
            callback({ ResponseCodes::OK, "Friend added successfully." });
        } else {
            callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
        }
    });
}

void SatellaApi::RemoveFriend(const std::string& friendId, DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    AsyncRequest([this, friendId, callback]() {
        cpr::Response response = cpr::Post(
            cpr::Url{ SATELLA_API_BASE_URL "/friends/remove" },
            cpr::Body{ json{{ "friendId", friendId }}.dump() },
            cpr::Header{ { "Content-Type", "application/json" } },
            DEFAULT_AUTH
            SSL_OPTIONS
        );

        if (response.status_code == (long) ResponseCodes::OK) {
            if (friends) {
                friends->erase(std::remove_if(friends->begin(), friends->end(),
                    [&friendId](const User& _user) { return _user.ulid == friendId; }), friends->end());
            }
            callback({ ResponseCodes::OK, "Friend removed successfully." });
        } else {
            callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
        }
    });
}

void SatellaApi::ModifyFriendRequest(const std::string& friendId, bool accept, DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    AsyncRequest([this, friendId, accept, callback]() {
        cpr::Response response = cpr::Post(
            cpr::Url{ SATELLA_API_BASE_URL "/friends/modify" },
            cpr::Body{ json{{ "friendId", friendId }, { "accept", accept }}.dump() },
            cpr::Header{ { "Content-Type", "application/json" } },
            DEFAULT_AUTH
            SSL_OPTIONS
        );

        if (response.status_code == (long) ResponseCodes::OK) {
            if (friends) {
                if(accept){
                    auto it = std::find_if(friends->begin(), friends->end(),
                        [&friendId](const User& _user) { return _user.ulid == friendId; });
                    if (it != friends->end()) {
                        it->status = FriendRequestStatus::ACCEPTED; // Mark as accepted
                    }
                } else {
                    friends->erase(std::remove_if(friends->begin(), friends->end(),
                        [&friendId](const User& _user) { return _user.ulid == friendId; }), friends->end());
                }
            }
            callback({ ResponseCodes::OK, "Friend request modified successfully." });
        } else {
            callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
        }
    });
}

void SatellaApi::ListPaks(DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    cpr::Response response = cpr::Get(
        cpr::Url{ SATELLA_API_BASE_URL "/cpak" },
        cpr::Timeout{ SATELLA_MAX_TIMEOUT },
        DEFAULT_AUTH
        SSL_OPTIONS
    );

    if (response.status_code == (long) ResponseCodes::OK) {
        auto data = json::parse(response.text).get<std::vector<VirtualControllerPak>>();
        if(this->paks == nullptr) {
            this->paks = std::make_shared<std::vector<VirtualControllerPak>>(data);
        } else {
            *this->paks = data;
        }
        callback({ ResponseCodes::OK, "Packs listed successfully." });
    } else {
        callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
    }
}

void SatellaApi::CreatePak(DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    AsyncRequest([this, callback]() {
        cpr::Response response = cpr::Post(
            cpr::Url{ SATELLA_API_BASE_URL "/cpak" },
            cpr::Header{ { "Content-Type", "application/json" } },
            DEFAULT_AUTH
            SSL_OPTIONS
        );

        if (response.status_code == (long) ResponseCodes::OK) {
            auto pak = json::parse(response.text).get<VirtualControllerPak>();
            if (paks) {
                paks->push_back(pak);
            }
            callback({ ResponseCodes::OK, "Pack created successfully." });
        } else {
            callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
        }
    });
}

void SatellaApi::UploadPak(const std::string& pakId, DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    if(currentPak == nullptr) {
        callback({ ResponseCodes::BAD_REQUEST, "No current pack selected." });
        return;
    }

    AsyncRequest([this, pakId, callback]() {
        std::vector<uint8_t> data = SatellaPak::SavePak(*currentPak);
        cpr::Buffer body(data.begin(), data.end(), "pak");

        cpr::Response response = cpr::Put(
            cpr::Url{ SATELLA_API_BASE_URL "/cpak/" + pakId },
            cpr::Multipart{ {"pak", body }},
            DEFAULT_AUTH
            SSL_OPTIONS
        );

        if (response.status_code == (long) ResponseCodes::OK) {
            callback({ ResponseCodes::OK, "Pack uploaded successfully." });
        } else {
            callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
        }
    });
}

void SatellaApi::UpdatePak(const VirtualControllerPak& pak, DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    AsyncRequest([this, pak, callback]() {
        cpr::Response response = cpr::Patch(
            cpr::Url{ SATELLA_API_BASE_URL "/cpak/" + pak.pakId },
            cpr::Body{json{
                { "name", pak.name }, 
                { "access", pak.access }
            }.dump()},
            cpr::Header{ { "Content-Type", "application/json" } },
            DEFAULT_AUTH
            SSL_OPTIONS
        );

        if (response.status_code == (long) ResponseCodes::OK) {
            callback({ ResponseCodes::OK, "Pack updated successfully." });
        } else {
            callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
        }
    });
}

void SatellaApi::InsertPak(const std::string& pakId, DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    AsyncRequest([this, pakId, callback]() {
        cpr::Response response = cpr::Get(
            cpr::Url{ SATELLA_API_BASE_URL "/cpak/" + pakId },
            cpr::Timeout{ SATELLA_MAX_TIMEOUT },
            DEFAULT_AUTH
            SSL_OPTIONS
        );

        if (response.status_code == (long) ResponseCodes::OK) {
            auto data = std::vector<uint8_t>(response.text.begin(), response.text.end());
            currentPak = std::make_shared<SatellaPakData>(SatellaPak::LoadPak(data));
            currentPak->pakId = pakId;
            callback({ ResponseCodes::OK, "Pack downloaded successfully." });
        } else {
            callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
        }
    });
}

void SatellaApi::DeletePak(const std::string& pakId, DefaultCallback callback) {
    if (!session || session->token.empty()) {
        callback({ ResponseCodes::UNAUTHORIZED, "No active session found." });
        return;
    }

    AsyncRequest([this, pakId, callback]() {
        cpr::Response response = cpr::Delete(
            cpr::Url{ SATELLA_API_BASE_URL "/cpak/" + pakId },
            DEFAULT_AUTH
            SSL_OPTIONS
        );

        if (response.status_code == (long) ResponseCodes::OK) {
            if (paks) {
                paks->erase(std::remove_if(paks->begin(), paks->end(),
                    [&pakId](const VirtualControllerPak& pak) { return pak.pakId == pakId; }), paks->end());
            }
            callback({ ResponseCodes::OK, "Pack deleted successfully." });
        } else {
            callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
        }
    });
}

void SatellaApi::SaveSession() {
    std::ofstream satellaFile(Ship::Context::GetPathRelativeToAppDirectory("satella.json"));

    if (!session) {
        if (satellaFile.is_open()) {
            satellaFile << "{}"; // Clear the file if no session exists
            satellaFile.close();
        }
        GameEngine::Instance->context->GetLogger()->info("No session to save, satella.json cleared.");
        return;
    }

    json satella = {
        {
            "session",
            {
                { "token", session->token },
                { "refreshToken", session->refreshToken },
                { "expiresAt", session->expiresAt }
            }
        },
        {
            "controllerPak",
            currentPak ? currentPak->pakId : ""
        }
    };

    if (satellaFile.is_open()) {
        satellaFile << satella.dump(4);
        satellaFile.close();
    } else {
        GameEngine::Instance->context->GetLogger()->error("Failed to save satella to file.");
    }
}

void SatellaApi::LoadSession() {
    std::ifstream satellaFile(Ship::Context::GetPathRelativeToAppDirectory("satella.json"));
    if (satellaFile.is_open()) {
        json satellaJson;
        satellaFile >> satellaJson;
        satellaFile.close();

        if (satellaJson.contains("session")) {
            auto sessionJson = satellaJson["session"];
            if (sessionJson.contains("token") && sessionJson.contains("refreshToken") && sessionJson.contains("expiresAt")) {
                this->session = std::make_shared<AuthSession>(sessionJson.get<AuthSession>());
                this->friends = std::make_shared<std::vector<User>>();
                SyncUser([&](const SatellaResponse& response) {
                    if (response.isValid) {
                        GameEngine::Instance->context->GetLogger()->info("User synced successfully after loading session.");
                    } else {
                        this->user = nullptr;
                        this->session = nullptr;
                        GameEngine::Instance->context->GetLogger()->warn("Failed to sync user after loading session: {}", response.message);
                    }
                });
            } else {
                GameEngine::Instance->context->GetLogger()->warn("Session file is missing required fields.");
            }
        } else {
            GameEngine::Instance->context->GetLogger()->warn("Session data not found in satella.json.");
        }

        if (satellaJson.contains("controllerPak")) {
            std::string pakId = satellaJson["controllerPak"].get<std::string>();
            if (!pakId.empty()) {
                ListPaks([this, pakId](const SatellaResponse& response) {
                    if (response.isValid && paks) {
                        auto it = std::find_if(paks->begin(), paks->end(),
                            [&pakId](const VirtualControllerPak& pak) { return pak.pakId == pakId; });
                        if (it != paks->end()) {
                            InsertPak(it->pakId, [](const SatellaResponse& response) {
                                if (!response.isValid) {
                                    GameEngine::Instance->context->GetLogger()->warn("Failed to insert pack after loading session: {}", response.message);
                                }
                            });
                        } else {
                            GameEngine::Instance->context->GetLogger()->warn("Pack with ID {} not found in loaded packs.", pakId);
                        }
                    } else {
                        GameEngine::Instance->context->GetLogger()->warn("Failed to list packs after loading session: {}", response.message);
                    }
                });
            }
        }

    } else {
        GameEngine::Instance->context->GetLogger()->warn("No session file found, starting without a session.");
    }
}
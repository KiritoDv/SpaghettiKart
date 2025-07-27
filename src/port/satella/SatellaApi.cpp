#include "SatellaApi.h"

#include "port/Engine.h"
#include <cpr/cpr.h>

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
                        { "x-refresh-token", session->refreshToken } \
                        { "x-net64-key", SATELLA_API_KEY }, \
                        { "x-net64-secret", SATELLA_API_SECRET } \
                    }

void SatellaApi::LinkAccount(std::string linkCode, DeviceType device, Callback callback) {
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

void SatellaApi::SyncUser(Callback callback) {
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
        callback({ ResponseCodes::OK, "User synced successfully." });
    } else {
        callback({ static_cast<ResponseCodes>(response.status_code), response.text, false });
    }
}

void SatellaApi::SaveSession() {
    if (!session) {
        return;
    }

    json sessionJson = *session;
    std::ofstream sessionFile(Ship::Context::GetPathRelativeToAppDirectory("session.json"));
    if (sessionFile.is_open()) {
        sessionFile << sessionJson.dump(4);
        sessionFile.close();
    } else {
        GameEngine::Instance->context->GetLogger()->error("Failed to save session to file.");
    }
}

void SatellaApi::LoadSession() {
    std::ifstream sessionFile(Ship::Context::GetPathRelativeToAppDirectory("session.json"));
    if (sessionFile.is_open()) {
        json sessionJson;
        sessionFile >> sessionJson;
        sessionFile.close();

        if (sessionJson.contains("token") && sessionJson.contains("refreshToken") && sessionJson.contains("expiresAt")) {
            this->session = std::make_shared<AuthSession>(sessionJson.get<AuthSession>());
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
        GameEngine::Instance->context->GetLogger()->warn("No session file found, starting without a session.");
    }
}
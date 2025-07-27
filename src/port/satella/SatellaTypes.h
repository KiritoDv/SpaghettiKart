#pragma once

#include <any>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include <cpr/cprtypes.h>
#include <functional>

using json = nlohmann::json;

enum class DeviceType { WINDOWS, LINUX, MAC, XBOX, WII_U, SWITCH, IOS, ANDROID };

enum class ResponseCodes {
    OK = 200,
    NOT_VERIFIED = 201,
    INVALID_CREDENTIALS = 202,
    DUPLICATED_ACCOUNT = 203,
    TOKEN_EXPIRED = 204,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERNAL_SERVER_ERROR = 500,
    SERVICE_UNAVAILABLE = 503,
};

struct AuthSession {
    std::string token;
    std::string refreshToken;
    std::string expiresAt;
};

struct User {
    std::string ulid;
    std::string username;
    std::string alias;
    std::string avatar;
    std::string accentColor;
    std::vector<std::string> favoriteGames;
};

struct SatellaResponse {
    ResponseCodes code;
    std::string message;
    bool isValid;

    SatellaResponse(ResponseCodes code, std::string message, bool isValid = true)
        : code(code), message(std::move(message)), isValid(isValid) {}
};

static std::vector<std::string> deviceNames = { "windows", "linux", "mac", "xbox", "wiiu", "switch", "ios", "android" };

// Helpers for JSON serialization/deserialization

#define LINK(type, key) j.at(#key).get_to(type.key)
#define CNV(type, key) { #key, type.key }

void to_json(json& j, const AuthSession& auth);
void from_json(const json& j, AuthSession& auth);
void from_json(const json& j, User& user);
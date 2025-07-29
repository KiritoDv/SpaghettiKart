#include "SatellaTypes.h"

void to_json(json& j, const AuthSession& auth) {
    j = json{ CNV(auth, token), CNV(auth, refreshToken), CNV(auth, expiresAt) };
}

void from_json(const json& j, AuthSession& auth) {
    LINK(auth, token);
    LINK(auth, refreshToken);
    LINK(auth, expiresAt);
}

void from_json(const json& j, User& user) {
    if(j.contains("ulid")) {
        user.ulid = j.at("ulid").get<std::string>();
    } else {
        user.ulid = j.at("userId").get<std::string>();
    }
    LINK(user, username);
    LINK(user, alias);
    LINK(user, avatar);
    LINK(user, accentColor);
    if (j.contains("favoriteGames")) {
        user.favoriteGames = j.at("favoriteGames").get<std::vector<std::string>>();
    } else {
        user.favoriteGames.clear();
    }
    if(j.contains("status")) {
        auto status = j.at("status").get<std::string>();
        user.status = status == "SENT" ? FriendRequestStatus::SENT :
                   status == "RECEIVED" ? FriendRequestStatus::RECEIVED : FriendRequestStatus::ACCEPTED;
    } else {
        user.status = FriendRequestStatus::SENT;
    }
}

void to_json(json& j, const VirtualControllerPak& pak) {
    j = json{
        CNV(pak, pakId),
        CNV(pak, ownerId),
        CNV(pak, name),
        CNV(pak, icon),
        CNV(pak, access),
        CNV(pak, createdAt),
        CNV(pak, updatedAt)
    };
}

void from_json(const json& j, VirtualControllerPak& pak) {
    LINK(pak, pakId);
    LINK(pak, ownerId);
    LINK(pak, name);
    LINK(pak, icon);
    if (j.contains("access")) {
        pak.access = j.at("access").get<std::vector<std::string>>();
    } else {
        pak.access.clear();
    }
    LINK(pak, createdAt);
    LINK(pak, updatedAt);
}
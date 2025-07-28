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
    if(j.contains("pending")) {
        user.pending = j.at("pending").get<bool>();
    } else {
        user.pending = false;
    }
}
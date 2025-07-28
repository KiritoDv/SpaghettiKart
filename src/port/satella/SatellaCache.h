#pragma once

#include <vector>
#include "port/Engine.h"
#include "SatellaTypes.h"

class SatellaCache {
public:
    SatellaCache() = default;
    ~SatellaCache() = default;

    static void LoadAvatar(const std::string& user, const std::vector<uint8_t>& data);
    static bool IsAvatarLoaded(const std::string& ulid) {
        return std::find(mAvatarCache.begin(), mAvatarCache.end(), ulid) != mAvatarCache.end();
    }
private:
    // Cache for user avatars
    static std::vector<std::string> mAvatarCache;
};
#pragma once

#include <vector>
#include "port/Engine.h"
#include "SatellaTypes.h"

class SatellaCache {
public:
    SatellaCache() = default;
    ~SatellaCache() = default;

    static void LoadPNG(const std::string& path);
    static void LoadAvatar(const std::string& user, const std::vector<uint8_t>& data);
    static bool IsImageLoaded(const std::string& path) {
        return std::find(mImageCache.begin(), mImageCache.end(), path) != mImageCache.end();
    }
private:

    static void CacheImage(const uint8_t* data, uint32_t size, const std::string& path);

    // Cache for user avatars
    static std::vector<std::string> mImageCache;
};
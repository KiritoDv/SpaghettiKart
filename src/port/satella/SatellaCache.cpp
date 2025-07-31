#include "SatellaCache.h"

#include <stb_image.h>

std::vector<std::string> SatellaCache::mImageCache;

void SatellaCache::CacheImage(const uint8_t* data, uint32_t size, const std::string& path) {
    SPDLOG_INFO("Caching image: {}", path);
    if (IsImageLoaded(path)) {
        return;
    }

    SPDLOG_INFO("Image not found in cache, loading: {}", path);
    auto texture = std::make_shared<Fast::Texture>();
    SPDLOG_INFO("Created texture for caching: {}", path);
    int height, width = 0;
    SPDLOG_INFO("Image data size: {}", (uint32_t) size);
    texture->ImageData = stbi_load_from_memory((const stbi_uc*)data, data, &width, &height, nullptr, 4);
    texture->Width = width;
    texture->Height = height;
    texture->Type = Fast::TextureType::RGBA32bpp;
    texture->ImageDataSize = texture->Width * texture->Height * 4;
    texture->Flags = TEX_FLAG_LOAD_AS_IMG;
    SPDLOG_INFO("Image loaded with dimensions: {}x{}", width, height);

    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    SPDLOG_INFO("Loading image into GUI: {}", path);
    gui->UnloadTexture(path);
    SPDLOG_INFO("Unloaded previous texture for path: {}", path);
    gui->LoadGuiTexture(path, *texture, ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
    SPDLOG_INFO("Loaded texture into GUI: {}", path);

    SatellaCache::mImageCache.push_back(path);
    SPDLOG_INFO("Image cached successfully: {}", path);
}

void SatellaCache::LoadPNG(const std::string& path) {
    if (IsImageLoaded(path)) {
        return;
    }

    auto file = Ship::Context::GetInstance()->GetResourceManager()->LoadFileProcess(path);
    if (!file->IsLoaded || file == nullptr || file->Buffer == nullptr) {
        GameEngine::Instance->context->GetLogger()->error("Failed to load PNG file: {}", path);
        return;
    }

    // std::vector<uint8_t> data(file->Buffer->begin(), file->Buffer->end());
    SPDLOG_INFO("Buffer size: {}", file->Buffer->size());
    char* buffer = file->Buffer->data();
    SPDLOG_INFO("Buffer begin ptr: {}", (uintptr_t) buffer);
    uint8_t* data = new uint8_t[file->Buffer->size()];
    SPDLOG_INFO("Allocated {} bytes for image data", file->Buffer->size());
    memcpy(data, buffer, file->Buffer->size());
    SPDLOG_INFO("Copied data to allocated buffer");

    CacheImage(data, file->Buffer->size(), path);
    delete[] data;
    GameEngine::Instance->context->GetLogger()->info("Loaded PNG file: {}", path);
}

void SatellaCache::LoadAvatar(const std::string& ulid, const std::vector<uint8_t>& data) {
    if (IsImageLoaded(ulid)) {
        return;
    }
    
    return;
    CacheImage(data.data(), data.size(), ulid);
    GameEngine::Instance->context->GetLogger()->info("Loaded avatar for user: {}", ulid);
}
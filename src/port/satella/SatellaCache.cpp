#include "SatellaCache.h"

#include <stb_image.h>

std::vector<std::string> SatellaCache::mImageCache;

void SatellaCache::CacheImage(const std::vector<uint8_t>& data, const std::string& path) {
    if (IsImageLoaded(path)) {
        return;
    }

    auto file = Ship::Context::GetInstance()->GetResourceManager()->LoadFileProcess(path);
    auto texture = std::make_shared<Fast::Texture>();

    int height, width = 0;
    texture->ImageData = stbi_load_from_memory((const stbi_uc*)data.data(), data.size(), &width, &height, nullptr, 4);
    texture->Width = width;
    texture->Height = height;
    texture->Type = Fast::TextureType::RGBA32bpp;
    texture->ImageDataSize = texture->Width * texture->Height * 4;
    texture->Flags = TEX_FLAG_LOAD_AS_IMG;

    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    gui->UnloadTexture(path);
    gui->LoadGuiTexture(path, *texture, ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });

    SatellaCache::mImageCache.push_back(path);
    GameEngine::Instance->context->GetLogger()->info("Loaded image from path", path);
}

void SatellaCache::LoadPNG(const std::string& path) {
    if (IsImageLoaded(path)) {
        return;
    }

    auto file = Ship::Context::GetInstance()->GetResourceManager()->LoadFileProcess(path);
    if (file == nullptr || file->Buffer == nullptr) {
        GameEngine::Instance->context->GetLogger()->error("Failed to load PNG file: {}", path);
        return;
    }

    std::vector<uint8_t> data(file->Buffer->begin(), file->Buffer->end());
    CacheImage(data, path);
    GameEngine::Instance->context->GetLogger()->info("Loaded PNG file: {}", path);
}

void SatellaCache::LoadAvatar(const std::string& ulid, const std::vector<uint8_t>& data) {
    if (IsImageLoaded(ulid)) {
        return;
    }

    CacheImage(data, ulid);
    GameEngine::Instance->context->GetLogger()->info("Loaded avatar for user: {}", ulid);
}
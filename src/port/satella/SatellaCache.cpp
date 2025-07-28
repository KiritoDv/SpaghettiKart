#include "SatellaCache.h"

#include <stb_image.h>

std::vector<std::string> SatellaCache::mAvatarCache;

void SatellaCache::LoadAvatar(const std::string& ulid, const std::vector<uint8_t>& data) {
    if (IsAvatarLoaded(ulid)) {
        return;
    }

    auto texture = std::make_shared<Fast::Texture>();
    int height, width = 0;
    texture->ImageData = stbi_load_from_memory((const stbi_uc*) data.data(), data.size(), &width, &height, nullptr, 4);
    texture->Width = width;
    texture->Height = height;
    texture->Type = Fast::TextureType::RGBA32bpp;
    texture->ImageDataSize = texture->Width * texture->Height * 4;
    texture->Flags = TEX_FLAG_LOAD_AS_IMG;

    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    gui->UnloadTexture(ulid);
    gui->LoadGuiTexture(ulid, *texture, ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });

    SatellaCache::mAvatarCache.push_back(ulid);
    GameEngine::Instance->context->GetLogger()->info("Loaded avatar for user: {}", ulid);
}
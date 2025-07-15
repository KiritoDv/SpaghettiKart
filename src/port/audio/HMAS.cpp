#include "HMAS.h"

#define MINIAUDIO_IMPLEMENTATION
#include "audio/miniaudio.h"
#include <spdlog/spdlog.h>
#include "port/Engine.h"

HMAS::HMAS() {
    ma_result result;
    ma_engine_config engine = ma_engine_config_init();

    engine.channels   = 2;
    engine.noDevice   = MA_TRUE;
    engine.sampleRate = GameEngine_GetSampleRate();

    result = ma_engine_init(&engine, &gAudioEngine);
    if (result != MA_SUCCESS) {
        SPDLOG_ERROR("Failed to initialize audio engine: {}", ma_result_description(result));
        return;
    }

    this->RegisterSound(HMAS_AudioId::DEMO, "/Volumes/Moon/Instances/MacMini-F3/Downloads/circuit-mario.wav");
}

void HMAS::RegisterSound(HMAS_AudioId id, const std::string& filePath) {
    if (gRegistry.find(id) != gRegistry.end()) {
        SPDLOG_WARN("Sound with ID {} already registered", static_cast<int>(id));
        return;
    }

    ma_result result = ma_sound_init_from_file(&gAudioEngine, filePath.c_str(), 0, NULL, NULL, &gRegistry[id].sound);
    if (result != MA_SUCCESS) {
        SPDLOG_ERROR("Failed to load sound from file {}: {}", filePath, ma_result_description(result));
        return;
    }
}

void HMAS::Play(HMAS_AudioId id, float speed, bool loop) {

    if(gCurrentSound != nullptr){
        this->Stop();
    }

    auto& sample = gRegistry[id];

    ma_sound_set_looping(&sample.sound, loop);
    ma_sound_set_pitch(&sample.sound, speed);
    ma_result result = ma_sound_start(&sample.sound);
    if (result != MA_SUCCESS) {
        SPDLOG_ERROR("Failed to start sound: {}", ma_result_description(result));
        return;
    }

    gCurrentSound = &sample.sound;
}

void HMAS::Stop() {
    if (gCurrentSound == nullptr) {
        return;
    }

    ma_sound_stop(gCurrentSound);
    ma_sound_uninit(gCurrentSound);
    gCurrentSound = nullptr;
}

bool HMAS::IsPlaying() {
    return ma_sound_is_playing(gCurrentSound);
}

void HMAS::CreateBuffer(uint8_t *samples, uint32_t bufferSizeInBytes) {
    ma_uint32 bufferSizeInFrames = bufferSizeInBytes / ma_get_bytes_per_frame(ma_format_f32, ma_engine_get_channels(&gAudioEngine));
    ma_engine_read_pcm_frames(&gAudioEngine, samples, bufferSizeInFrames, NULL);
}

HMAS::~HMAS() {
    for (auto& pair : gRegistry) {
        ma_sound_uninit(&pair.second.sound);
    }
    gRegistry.clear();
    ma_engine_uninit(&gAudioEngine);
}

// Expose C API for HMAS

extern "C" void HMAS_Play(HMAS_AudioId id, float speed, bool loop) {
    GameEngine::Instance->gHMAS->Play(id, speed, loop);
}

extern "C" void HMAS_Stop() {
    GameEngine::Instance->gHMAS->Stop();
}

extern "C" bool HMAS_IsPlaying() {
    return GameEngine::Instance->gHMAS->IsPlaying();
}
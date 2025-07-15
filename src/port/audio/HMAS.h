#pragma once

enum HMAS_AudioId {
    DEMO = 0
};

#ifdef __cplusplus

#include <string>
#include <cstdint>
#include <unordered_map>
#include "audio/miniaudio.h"

struct HMAS_Sample {
    ma_sound sound;
};

class HMAS {
public:
    HMAS();
    ~HMAS();

    void RegisterSound(HMAS_AudioId id, const std::string& filePath);

    void Play(HMAS_AudioId id, float speed = 1.0f, bool loop = false);
    void Stop();
    bool IsPlaying();

    void CreateBuffer(uint8_t* samples, uint32_t num_samples);

private:
    ma_engine gAudioEngine;
    ma_sound* gCurrentSound;
    std::unordered_map<HMAS_AudioId, HMAS_Sample> gRegistry;
};

extern "C" {
#endif

void HMAS_Play(enum HMAS_AudioId id, float speed, bool loop);
void HMAS_Stop();
bool HMAS_IsPlaying();

#ifdef __cplusplus
}
#endif

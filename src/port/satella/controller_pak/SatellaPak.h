#pragma once

#include <vector>
#include <cstdint>

struct SatellaPakHeader {
    std::string pakId;
    uint32_t size = 0;
    uint32_t gameCode = 0;
    uint16_t companyCode = 0;
    char extName[4] = { 0 };
    char gameName[16] = { 0 };
};

struct SatellaPakFile {
    std::vector<uint8_t> data = {};
};

struct SatellaPakData {
    SatellaPakHeader header;
    SatellaPakFile files[16];
};

class SatellaPak {
public:
    SatellaPak() = default;

    static SatellaPakData LoadPak(std::vector<uint8_t> data);
    static std::vector<uint8_t> SavePak(const SatellaPakData& pakData);
};
#pragma once

#include <vector>
#include <string>
#include <cstdint>

enum class PfsResult {
    PAK_NO_ERROR,
    ERR_NOPACK,
    ERR_NEW_PACK,
    ERR_INCONSISTENT,
    ERR_CONTRFAIL,
    ERR_INVALID,
    ERR_BAD_DATA,
    DATA_FULL,
    DIR_FULL,
    ERR_EXIST,
    ERR_ID_FATAL,
    ERR_DEVICE,
    ERR_NO_GBCART,
    ERR_NEW_GBCART
};

struct SatellaPakHeader {
    uint32_t size = 0;
    uint32_t gameCode = 0;
    uint16_t companyCode = 0;
    uint8_t extName[4] = { 0 };
    uint8_t gameName[16] = { 0 };
};

struct SatellaPakFile {
    std::vector<uint8_t> data = {};
};

struct SatellaPakData {
    std::string pakId;
    SatellaPakHeader header[16];
    SatellaPakFile files[16];
};

class SatellaPak {
public:
    SatellaPak() = default;

    static bool IsVirtualPak();

    static SatellaPakData LoadPak(std::vector<uint8_t> data);
    static std::vector<uint8_t> SavePak(const SatellaPakData& pakData);

    static PfsResult FreeBlocks(int32_t* bytes_not_used);
    static PfsResult AllocateFile(uint16_t company_code, uint32_t game_code, uint8_t* game_name, uint8_t* ext_name, uint32_t file_size_in_bytes, int32_t* file_no);
    static PfsResult FileState(int32_t file_no, SatellaPakHeader* state);
    static PfsResult FindFile(uint16_t company_code, uint32_t game_code, uint8_t* game_name, uint8_t* ext_name, int32_t* file_no);
    static PfsResult ReadWriteFile(int32_t file_no, uint8_t flag, int offset, int size_in_bytes, uint8_t* data_buffer);
    static PfsResult NumFiles(int32_t* max_files, int32_t* files_used);
    static PfsResult DeletePakFile(uint16_t company_code, uint32_t game_code, uint8_t* game_name, uint8_t* ext_name);

private:
    static bool PakHeaderRead(uint32_t* file_size, uint32_t* game_code, uint16_t* company_code, uint8_t* ext_name, uint8_t* game_name, uint8_t fileIndex);
    static bool PakHeaderWrite(uint32_t* file_size, uint32_t* game_code, uint16_t* company_code, uint8_t* ext_name, uint8_t* game_name, uint8_t fileIndex);
};
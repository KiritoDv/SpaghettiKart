#include "SatellaPak.h"

#include "classes.h"
#include "port/Engine.h"

const uint32_t MaxFiles = 16;
const uint32_t ExtNameSize = 4;
const uint32_t GameNameSize = 16;

bool SatellaPak::IsVirtualPak() {
    return GameEngine::Instance->gSatellaApi->GetCurrentPak() != nullptr;
}

SatellaPakData SatellaPak::LoadPak(std::vector<uint8_t> data) {
    if(data.empty()){
        return SatellaPakData {};
    }

    auto reader = Ship::BinaryReader((char*) data.data(), data.size());
    SatellaPakData pak;

    for(size_t i = 0; i < MaxFiles; i++){
        pak.header[i].size = reader.ReadUInt32();
        pak.header[i].gameCode = reader.ReadUInt32();
        pak.header[i].companyCode = reader.ReadUInt16();
        reader.Read((char*) pak.header[i].extName, ExtNameSize);
        reader.Read((char*) pak.header[i].gameName, MaxFiles);

        uint32_t size = reader.ReadUInt32();
        if(size == 0){
            continue;
        }
        char* file = new char[size];
        reader.Read(file, size);
        pak.files[i].data = std::vector<uint8_t>((uint8_t*) file, (uint8_t*) (file + size));
        delete[] file;
    }

    return pak;
}

std::vector<uint8_t> SatellaPak::SavePak(const SatellaPakData& pak) {
    auto writer = Ship::BinaryWriter();

    // Write pak header
    
    for(size_t i = 0; i < MaxFiles; i++){
        writer.Write(pak.header[i].size);
        writer.Write(pak.header[i].gameCode);
        writer.Write(pak.header[i].companyCode);
        writer.Write((char*) pak.header[i].extName, ExtNameSize);
        writer.Write((char*) pak.header[i].gameName, MaxFiles);

        auto& file = pak.files[i];
        writer.Write((uint32_t) file.data.size());
        if(file.data.empty()) {
            continue;
        }
        writer.Write((char*) file.data.data(), file.data.size());
    }

    std::vector<char> wr = writer.ToVector();
    std::vector<uint8_t> data(wr.begin(), wr.end());
    return data;
}

bool SatellaPak::PakHeaderWrite(uint32_t* file_size, uint32_t* game_code, uint16_t* company_code, uint8_t* ext_name, uint8_t* game_name, uint8_t fileIndex) {
    auto pak = GameEngine::Instance->gSatellaApi->GetCurrentPak();

    if (!pak) {
        return false;
    }

    pak->header[fileIndex].size = *file_size;
    pak->header[fileIndex].gameCode = *game_code;
    pak->header[fileIndex].companyCode = *company_code;
    memcpy(pak->header[fileIndex].extName, ext_name, ExtNameSize);
    memcpy(pak->header[fileIndex].gameName, game_name, MaxFiles);

    return true;
}

bool SatellaPak::PakHeaderRead(uint32_t* file_size, uint32_t* game_code, uint16_t* company_code, uint8_t* ext_name, uint8_t* game_name, uint8_t fileIndex) {
    auto pak = GameEngine::Instance->gSatellaApi->GetCurrentPak();

    if (!pak || fileIndex >= sizeof(pak->files) / sizeof(pak->files[0])) {
        return false;
    }

    *file_size = pak->files[fileIndex].data.size();
    *game_code = pak->header[fileIndex].gameCode;
    *company_code = pak->header[fileIndex].companyCode;
    memcpy(ext_name, pak->header[fileIndex].extName, ExtNameSize);
    memcpy(game_name, pak->header[fileIndex].gameName, MaxFiles);

    return true;
}

PfsResult SatellaPak::FreeBlocks(int32_t* bytes_not_used) {
    int32_t usedSpace = 0;
    for (size_t i = 0; i < MaxFiles; i++) {
        uint32_t file_size = 0;
        uint32_t game_code = 0;
        uint16_t company_code = 0;
        char ext_name[ExtNameSize] = { 0 };
        char game_name[GameNameSize] = { 0 };

        if (!PakHeaderRead(&file_size, &game_code, &company_code, (uint8_t*) ext_name, (uint8_t*) game_name, i)) {
            return PfsResult::ERR_INVALID;
        }

        if ((company_code == 0) || (game_code == 0)) {
            continue;
        } else {
            usedSpace += file_size >> 8;
        }
    }

    *bytes_not_used = (123 - usedSpace) << 8;

    return PfsResult::PAK_NO_ERROR;
}

PfsResult SatellaPak::AllocateFile(uint16_t company_code, uint32_t game_code, uint8_t* game_name, uint8_t* ext_name, uint32_t file_size_in_bytes, int32_t* file_no) {
    auto pak = GameEngine::Instance->gSatellaApi->GetCurrentPak();

    if ((company_code == 0) || (game_code == 0)) {
        return PfsResult::ERR_INVALID;
    }

    uint8_t freeFileIndex = 0;
    for (size_t i = 0; i < MaxFiles; i++) {
        uint32_t file_size_ = 0;
        uint32_t game_code_ = 0;
        uint16_t company_code_ = 0;
        char ext_name_[ExtNameSize] = { 0 };
        char game_name_[GameNameSize] = { 0 };

        if (!PakHeaderRead(&file_size_, &game_code_, &company_code_, (uint8_t*) ext_name_, (uint8_t*) game_name_, i)) {
            return PfsResult::ERR_INVALID;
        }

        if ((company_code_ == 0) && (game_code_ == 0)) {
            freeFileIndex = i;
            break;
        }
    }

    if (freeFileIndex >= MaxFiles) {
        return PfsResult::DATA_FULL;
    }

    if(!PakHeaderWrite(&file_size_in_bytes, &game_code, &company_code, ext_name, game_name, freeFileIndex)) {
        return PfsResult::ERR_INVALID;
    }

    file_size_in_bytes = (file_size_in_bytes + 31) & ~31;
    pak->files[freeFileIndex].data.resize(file_size_in_bytes);

    *file_no = freeFileIndex;
    return PfsResult::PAK_NO_ERROR;
}

PfsResult SatellaPak::FileState(int32_t file_no, SatellaPakHeader* state) {
    auto pak = GameEngine::Instance->gSatellaApi->GetCurrentPak();

    u32 file_size = 0;
    u32 game_code = 0;
    u16 company_code = 0;
    char ext_name[ExtNameSize] = { 0 };
    char game_name[GameNameSize] = { 0 };

    if (!PakHeaderRead(&file_size, &game_code, &company_code, (uint8_t*) ext_name, (uint8_t*) game_name, file_no)) {
        return PfsResult::ERR_INVALID;
    }

    state->size = file_size;
    state->companyCode = game_code;
    state->gameCode = game_code;

    memcpy(state->extName, ext_name, ExtNameSize);
    memcpy(state->gameName, game_name, MaxFiles);

    return PfsResult::PAK_NO_ERROR;
}

PfsResult SatellaPak::FindFile(uint16_t company_code, uint32_t game_code, uint8_t* game_name, uint8_t* ext_name, int32_t* file_no) {
    for (size_t i = 0; i < MaxFiles; i++) {
        uint32_t file_size_ = 0;
        uint32_t game_code_ = 0;
        uint16_t company_code_ = 0;
        char ext_name_[ExtNameSize] = { 0 };
        char game_name_[GameNameSize] = { 0 };

        if (!PakHeaderRead(&file_size_, &game_code_, &company_code_, (uint8_t*) ext_name_, (uint8_t*) game_name_, i)) {
            return PfsResult::ERR_INVALID;
        }

        if ((company_code_ == 0) || (game_code_ == 0)) {
            continue;
        } else {
            if ((game_code == game_code_) && (company_code == company_code_) &&
                (strcmp((const char*) game_name, (const char*) game_name_) == 0) &&
                (strcmp((const char*) ext_name, (const char*) ext_name_) == 0)) {
                *file_no = i;
                return PfsResult::PAK_NO_ERROR;
            }
        }
    }

    return PfsResult::ERR_INVALID;
}

PfsResult SatellaPak::ReadWriteFile(int32_t file_no, uint8_t write, int offset, int size_in_bytes, uint8_t* data_buffer) {
    auto pak = GameEngine::Instance->gSatellaApi->GetCurrentPak();

    if (file_no < 0 || file_no >= MaxFiles) {
        return PfsResult::ERR_INVALID;
    }

    if (write == 0) {
        if (offset + size_in_bytes >= pak->files[file_no].data.size()) {
            pak->files[file_no].data.resize(offset + size_in_bytes);
        }
        std::memcpy(data_buffer, pak->files[file_no].data.data() + offset, size_in_bytes);
    } else if (write == 1) {
        if (offset + size_in_bytes >= pak->files[file_no].data.size()) {
            pak->files[file_no].data.resize(offset + size_in_bytes);
        }
        std::memcpy(pak->files[file_no].data.data() + offset, data_buffer, size_in_bytes);
    } else {
        return PfsResult::ERR_INVALID;
    }

    return PfsResult::PAK_NO_ERROR;
}

PfsResult SatellaPak::NumFiles(int32_t* max_files, int32_t* files_used) {
    auto pak = GameEngine::Instance->gSatellaApi->GetCurrentPak();
    uint8_t files = 0;

    for (size_t i = 0; i < MaxFiles; i++) {
        uint32_t file_size_ = 0;
        uint32_t game_code_ = 0;
        uint16_t company_code_ = 0;
        char ext_name_[ExtNameSize] = { 0 };
        char game_name_[GameNameSize] = { 0 };

        if (!PakHeaderRead(&file_size_, &game_code_, &company_code_, (uint8_t*) ext_name_, (uint8_t*) game_name_, i)) {
            return PfsResult::ERR_INVALID;
        }

        if ((company_code_ != 0) && (game_code_ != 0)) {
            (*files_used)++;
        }
    }

    *files_used = files;
    *max_files = MaxFiles;

    return PfsResult::PAK_NO_ERROR;
}

PfsResult SatellaPak::DeletePakFile(uint16_t company_code, uint32_t game_code, uint8_t* game_name, uint8_t* ext_name) {
    auto pak = GameEngine::Instance->gSatellaApi->GetCurrentPak();

    if (company_code == 0 || game_code == 0) {
        return PfsResult::ERR_INVALID;
    }

    for (size_t i = 0; i < MaxFiles; i++) {
        uint32_t file_size_ = 0;
        uint32_t game_code_ = 0;
        uint16_t company_code_ = 0;
        char ext_name_[ExtNameSize] = { 0 };
        char game_name_[GameNameSize] = { 0 };

        if (!PakHeaderRead(&file_size_, &game_code_, &company_code_, (uint8_t*) ext_name_, (uint8_t*) game_name_, i)) {
            return PfsResult::ERR_INVALID;
        }
        
        if ((company_code_ == 0) || (game_code_ == 0)) {
            continue;
        } else if ((game_code == game_code_) && (strcmp((const char*) game_name, (const char*) game_name_) == 0) &&
                strcmp((const char*) ext_name, (const char*) ext_name_) == 0) {
            pak->files[i].data.clear();
            pak->header[i] = {};
            return PfsResult::PAK_NO_ERROR;
        }
    }

    return PfsResult::PAK_NO_ERROR;
}
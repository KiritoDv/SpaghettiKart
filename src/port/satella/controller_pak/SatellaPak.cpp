#include "SatellaPak.h"

#include "classes.h"

SatellaPakData SatellaPak::LoadPak(std::vector<uint8_t> data) {
    if(data.empty()){
        return SatellaPakData {};
    }

    auto reader = Ship::BinaryReader((char*) data.data(), data.size());
    SatellaPakData pak;

    pak.header.size = reader.ReadUInt32();
    pak.header.gameCode = reader.ReadUInt32();
    pak.header.companyCode = reader.ReadUInt16();
    reader.Read(pak.header.extName, 4);
    reader.Read(pak.header.gameName, 16);

    for(size_t i = 0; i < sizeof(pak.files) / sizeof(pak.files[0]); i++){
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
    writer.Write(pak.header.size);
    writer.Write(pak.header.gameCode);
    writer.Write(pak.header.companyCode);
    writer.Write((char*) pak.header.extName, 4);
    writer.Write((char*) pak.header.gameName, 16);

    for(size_t i = 0; i < sizeof(pak.files) / sizeof(pak.files[0]); i++){
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
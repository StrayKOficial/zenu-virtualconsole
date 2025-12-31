#include "loader.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

bool Loader::load_boc(const std::string& path, std::vector<uint8_t>& rom_out, Manifest& manifest_out) {
    if (!fs::exists(path) || !fs::is_directory(path)) {
        std::cerr << "Invalid .boc path: " << path << std::endl;
        return false;
    }

    std::string rom_path = path + "/rom.bin";
    std::ifstream rom_file(rom_path, std::ios::binary);
    if (!rom_file) {
        std::cerr << "Could not find rom.bin in " << path << std::endl;
        return false;
    }

    rom_out.assign((std::istreambuf_iterator<char>(rom_file)), std::istreambuf_iterator<char>());
    
    // Use the folder name as the game name
    manifest_out.name = fs::path(path).filename().string();
    manifest_out.version = "1.0.0";

    return true;
}

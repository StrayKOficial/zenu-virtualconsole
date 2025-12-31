#ifndef LOADER_HPP
#define LOADER_HPP

#include <string>
#include <vector>
#include <cstdint>

struct Manifest {
    std::string name;
    std::string version;
};

class Loader {
public:
    static bool load_boc(const std::string& path, std::vector<uint8_t>& rom_out, Manifest& manifest_out);
};

#endif

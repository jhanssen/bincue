#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace CueParser {

struct Length
{
    uint8_t mm { 0 };
    uint8_t ss { 0 };
    uint8_t ff { 0 };
};

struct Index
{
    uint32_t number { 0 };
    Length length { };
};

struct Track
{
    enum class Type {
        Audio,
        CDG,
        Mode1_2048,
        Mode1_2352,
        Mode2_2048,
        Mode2_2324,
        Mode2_2336,
        Mode2_2352,
        CDI_2336,
        CDI_2352
    };

    uint32_t trackNo { 0 };
    Type type { };

    std::optional<Length> pregap { };
    std::vector<Index> index { };
    std::optional<Length> postgap { };
};

struct File
{
    enum class Type {
        Binary,
        Motorola,
        Aiff,
        Wave,
        Mp3
    };

    std::string filename { };
    Type type { };

    std::vector<Track> tracks { };
};

struct CueSheet
{
    std::vector<File> files;
    std::optional<uint64_t> catalog { };
};

CueSheet parseFile(const std::filesystem::path& filename);
CueSheet parse(const std::string& data);

} // namespace CueParser

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

    enum class Flag {
        DCP  = 0x1,
        CH4  = 0x2, // C++ doesn't allow 4CH since identifiers can't start with a number
        PRE  = 0x4,
        SCMS = 0x8
    };

    uint32_t number { 0 };
    Type type { };
    Flag flags { };

    std::optional<Length> pregap { };
    std::vector<Index> index { };
    std::optional<Length> postgap { };
    std::string title { };
    std::string performer { };
    std::string songwriter { };
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
    std::vector<File> files { };
    std::string cdtextfile { };
    std::optional<uint64_t> catalog { };
};

CueSheet parseFile(const std::filesystem::path& filename);
CueSheet parse(const std::string& data);

template<typename T>
class BoolConvertible
{
public:
    BoolConvertible(T t)
        : mT(t)
    {
    }

    explicit operator bool() const { return static_cast<bool>(mT); }
    operator T() const { return mT; }

private:
    const T mT;
};

inline BoolConvertible<Track::Flag> operator&(Track::Flag f1, Track::Flag f2)
{
    return BoolConvertible<Track::Flag>(
        static_cast<Track::Flag>(
            static_cast<std::underlying_type_t<Track::Flag>>(f1)
            & static_cast<std::underlying_type_t<Track::Flag>>(f2))
        );
}

inline BoolConvertible<Track::Flag> operator|(Track::Flag f1, Track::Flag f2)
{
    return BoolConvertible<Track::Flag>(
        static_cast<Track::Flag>(
            static_cast<std::underlying_type_t<Track::Flag>>(f1)
            | static_cast<std::underlying_type_t<Track::Flag>>(f2))
        );
}

inline bool operator==(Track::Flag f1, std::underlying_type_t<Track::Flag> f2)
{
    return static_cast<std::underlying_type_t<Track::Flag>>(f1) == f2;
}

inline bool operator!=(Track::Flag f1, std::underlying_type_t<Track::Flag> f2)
{
    return static_cast<std::underlying_type_t<Track::Flag>>(f1) != f2;
}

} // namespace CueParser

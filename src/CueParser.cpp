#include "CueParser.h"
#include <array>
#include <cstring>
#include <cstdlib>
#include <fstream>

namespace fs = std::filesystem;

namespace {

enum { MaxTokens = 5 };

inline void operator|=(CueParser::Track::Flag& f1, CueParser::Track::Flag f2)
{
    f1 = (static_cast<CueParser::Track::Flag>(
              static_cast<std::underlying_type_t<CueParser::Track::Flag>>(f1)
              | static_cast<std::underlying_type_t<CueParser::Track::Flag>>(f2))
        );
}

std::pair<CueParser::File::Type, bool> fileType(const char* type)
{
    if (!strncmp(type, "BINARY", 6))
        return std::make_pair(CueParser::File::Type::Binary, true);
    else if (!strncmp(type, "WAVE", 4))
        return std::make_pair(CueParser::File::Type::Wave, true);
    else if (!strncmp(type, "MP3", 3))
        return std::make_pair(CueParser::File::Type::Mp3, true);
    else if (!strncmp(type, "AIFF", 4))
        return std::make_pair(CueParser::File::Type::Aiff, true);
    else if (!strncmp(type, "MOTOROLA", 8))
        return std::make_pair(CueParser::File::Type::Motorola, true);
    return std::make_pair(CueParser::File::Type::Binary, false);
}

std::pair<CueParser::Track::Type, bool> trackType(const char* type)
{
    if (!strncmp(type, "AUDIO", 5))
        return std::make_pair(CueParser::Track::Type::Audio, true);
    else if (!strncmp(type, "MODE1/2048", 10))
        return std::make_pair(CueParser::Track::Type::Mode1_2048, true);
    else if (!strncmp(type, "MODE1/2352", 10))
        return std::make_pair(CueParser::Track::Type::Mode1_2352, true);
    else if (!strncmp(type, "MODE2/2048", 10))
        return std::make_pair(CueParser::Track::Type::Mode2_2048, true);
    else if (!strncmp(type, "MODE2/2324", 10))
        return std::make_pair(CueParser::Track::Type::Mode2_2324, true);
    else if (!strncmp(type, "MODE2/2336", 10))
        return std::make_pair(CueParser::Track::Type::Mode2_2336, true);
    else if (!strncmp(type, "MODE2/2352", 10))
        return std::make_pair(CueParser::Track::Type::Mode2_2352, true);
    else if (!strncmp(type, "CDG", 3))
        return std::make_pair(CueParser::Track::Type::CDG, true);
    else if (!strncmp(type, "CDI/2336", 8))
        return std::make_pair(CueParser::Track::Type::CDI_2336, true);
    else if (!strncmp(type, "CDI/2352", 8))
        return std::make_pair(CueParser::Track::Type::CDI_2352, true);
    return std::make_pair(CueParser::Track::Type::Audio, false);
}

struct State
{
    State(const std::string& data)
        : mData(data)
    {
    }

    bool nextLine();
    const char* token(size_t n);

    template<typename Int>
    std::enable_if_t<std::is_unsigned_v<Int>, std::pair<Int, bool>> number(size_t n);

    template<typename Int>
    std::enable_if_t<!std::is_unsigned_v<Int>, std::pair<Int, bool>> number(size_t n);

    std::pair<CueParser::Length, bool> mmssff(size_t n);

    std::string mData;

    size_t mOffset { 0 };

    size_t mNumTokens { 0 };
    std::array<size_t, MaxTokens> mTokens { };
};

const char* State::token(size_t n)
{
    if (n >= mNumTokens)
        return nullptr;
    return mData.data() + mTokens[n];
}

template<typename Int>
std::enable_if_t<!std::is_unsigned_v<Int>, std::pair<Int, bool>> State::number(size_t n)
{
    const char* t = token(n);
    if (t == nullptr)
        return std::make_pair(Int {}, false);
    char* endptr;
    const auto nn = strtoll(t, &endptr, 10);
    if (*endptr != '\0')
        return std::make_pair(Int {}, false);
    return std::make_pair(static_cast<Int>(nn), true);
}

template<typename Int>
std::enable_if_t<std::is_unsigned_v<Int>, std::pair<Int, bool>> State::number(size_t n)
{
    const char* t = token(n);
    if (t == nullptr)
        return std::make_pair(Int {}, false);
    char* endptr;
    const auto nn = strtoull(t, &endptr, 10);
    if (*endptr != '\0')
        return std::make_pair(Int {}, false);
    return std::make_pair(static_cast<Int>(nn), true);
}

std::pair<CueParser::Length, bool> State::mmssff(size_t n)
{
    const char* t = token(n);
    if (t == nullptr)
        return std::make_pair(CueParser::Length {}, false);

    // parse ss
    char* endptr;
    const auto ss = strtoul(t, &endptr, 10);
    if (ss >= 100 || *endptr != ':')
        return std::make_pair(CueParser::Length {}, false);

    // parse mm
    const auto mm = strtoul(endptr + 1, &endptr, 10);
    if (mm >= 100 || *endptr != ':')
        return std::make_pair(CueParser::Length {}, false);

    // parse ff
    const auto ff = strtoul(endptr + 1, &endptr, 10);
    if (ff >= 100 || *endptr != '\0')
        return std::make_pair(CueParser::Length {}, false);

    return std::make_pair(CueParser::Length {
        static_cast<uint8_t>(ss),
        static_cast<uint8_t>(mm),
        static_cast<uint8_t>(ff)
    }, true);
}

bool State::nextLine()
{
    // find '\n'
    if (mOffset + 1 >= mData.size()) {
        return false;
    }

    const auto size = mData.size();
    auto data = mData.data();
    auto pos = strchr(data + mOffset, '\n');
    if (pos == nullptr) {
        // use the rest of the data as the remainer
        pos = data + size;
    }

    auto start = mOffset;
    mOffset = (pos - data) + 1;

    if (mOffset - start == 1) {
        // skip empty lines
        mNumTokens = 0;
        return true;
    }

    // split on whitespace
    auto skipWS = [data, this](size_t from) {
        while (from < mOffset && isspace(*(data + from)))
            ++from;
        return from;
    };

    auto skipToken = [data, this](size_t from) {
        bool quote = false;
        if (*(data + from) == '"') {
            // skip until next quote
            ++from;
            while (from < mOffset && *(data + from) != '"')
                ++from;
            if (from < mOffset)
                ++from;
            quote = true;
        } else {
            // skip until next ws
            while (from < mOffset && !isspace(*(data + from)))
                ++from;
        }
        return std::make_pair(from, quote);
    };

    // find up to MaxTokens tokens
    mNumTokens = 0;
    for (size_t t = 0; t < MaxTokens; ++t) {
        const auto next = skipWS(start);
        if (next >= mOffset)
            break;
        const auto [ end, quote ] = skipToken(next);
        if (end >= mOffset)
            break;
        if (quote) {
            data[end - 1] = '\0';
            mTokens[mNumTokens++] = next + 1;
        } else {
            data[end] = '\0';
            mTokens[mNumTokens++] = next;
        }
        // printf("got token %s\n", data + mTokens[mNumTokens - 1]);
        start = end + 1;
    }

    return true;
}

} // anonymous namespace

namespace CueParser {

CueSheet parse(const std::string& data)
{
    State state(data);

    CueSheet out;

    while (state.nextLine()) {
        const auto token = state.token(0);
        if (!token)
            continue;
        if (!strncmp(token, "FILE", 4)) {
            // we should have three tokens, FILE <filename> <type>
            const auto filename = state.token(1);
            const auto type = state.token(2);
            if (!filename || !type)
                continue;
            const auto [ typee, typeok ] = fileType(type);
            if (!typeok)
                continue;
            out.files.push_back(CueParser::File { std::string(filename), typee });
        } else if (!strncmp(token, "TRACK", 5)) {
            if (out.files.empty())
                continue;
            // we should have three tokens, TRACK <number> <type>
            const auto [ number, numberok ] = state.number<uint32_t>(1);
            const auto type = state.token(2);
            if (!numberok || !type)
                continue;
            const auto [ typee, typeok ] = trackType(type);
            if (!typeok)
                continue;
            out.files.back().tracks.push_back(CueParser::Track { number, typee });
        } else if (!strncmp(token, "INDEX", 5)) {
            if (out.files.empty())
                continue;
            auto& file = out.files.back();
            if (file.tracks.empty())
                continue;
            // we should have three tokens, INDEX <number> <mm::ss::ff>
            const auto [ number, numberok ] = state.number<uint32_t>(1);
            const auto [ length, lengthok ] = state.mmssff(2);
            if (!numberok || !lengthok)
                continue;
            file.tracks.back().index.push_back(CueParser::Index { number, length });
        } else if (!strncmp(token, "PREGAP", 6)) {
            if (out.files.empty())
                continue;
            auto& file = out.files.back();
            if (file.tracks.empty())
                continue;
            // we should have two tokens, PREGAP <mm::ss::ff>
            const auto [ length, lengthok ] = state.mmssff(1);
            if (!lengthok)
                continue;
            file.tracks.back().pregap = length;
        } else if (!strncmp(token, "POSTGAP", 7)) {
            if (out.files.empty())
                continue;
            auto& file = out.files.back();
            if (file.tracks.empty())
                continue;
            // we should have two tokens, POSTGAP <mm::ss::ff>
            const auto [ length, lengthok ] = state.mmssff(1);
            if (!lengthok)
                continue;
            file.tracks.back().postgap = length;
        } else if (!strncmp(token, "TITLE", 5)) {
            // we should have two tokens, TITLE title
            const auto title = state.token(1);
            if (!title)
                continue;
            if (out.files.empty()) {
                out.title = title;
                continue;
            }
            auto& file = out.files.back();
            if (file.tracks.empty()) {
                out.title = title;
                continue;
            }
            file.tracks.back().title = title;
        } else if (!strncmp(token, "PERFORMER", 9)) {
            // we should have two tokens, PERFORMER performer
            const auto performer = state.token(1);
            if (!performer)
                continue;
            if (out.files.empty()) {
                out.performer = performer;
                continue;
            }
            auto& file = out.files.back();
            if (file.tracks.empty()) {
                out.performer = performer;
                continue;
            }
            file.tracks.back().performer = performer;
        } else if (!strncmp(token, "SONGWRITER", 10)) {
            // we should have two tokens, SONGWRITER songwriter
            const auto songwriter = state.token(1);
            if (!songwriter)
                continue;
            if (out.files.empty()) {
                out.songwriter = songwriter;
                continue;
            }
            auto& file = out.files.back();
            if (file.tracks.empty()) {
                out.songwriter = songwriter;
                continue;
            }
            file.tracks.back().songwriter = songwriter;
        } else if (!strncmp(token, "ISRC", 4)) {
            if (out.files.empty())
                continue;
            auto& file = out.files.back();
            if (file.tracks.empty())
                continue;
            // we should have two tokens, ISRC CCOOOYYSSSSS
            const auto isrct = state.token(1);
            if (!isrct)
                continue;
            const auto len = strlen(isrct);
            if (len != 12)
                continue;

            // convert the serial
            char* endl;
            const auto serial = strtoul(isrct + 7, &endl, 10);
            if (*endl != '\0')
                continue;

            ISRC isrc;
            isrc.country[0] = isrct[0];
            isrc.country[1] = isrct[1];
            isrc.owner[0] = isrct[2];
            isrc.owner[1] = isrct[3];
            isrc.owner[2] = isrct[4];
            isrc.year[0] = isrct[5];
            isrc.year[1] = isrct[6];
            isrc.serial = static_cast<uint32_t>(serial);

            file.tracks.back().isrc = isrc;
        } else if (!strncmp(token, "FLAGS", 5)) {
            if (out.files.empty())
                continue;
            auto& file = out.files.back();
            if (file.tracks.empty())
                continue;
            // we should have one or more tokens, FLAGS [flag1 [flag2 ...]]
            Track::Flag flags = {};
            for (uint32_t idx = 1;; ++idx) {
                const auto f = state.token(idx);
                if (f == nullptr)
                    break;
                if (!strncmp(f, "DCP", 3))
                    flags |= Track::Flag::DCP;
                else if (!strncmp(f, "4CH", 3))
                    flags |= Track::Flag::CH4;
                else if (!strncmp(f, "PRE", 3))
                    flags |= Track::Flag::PRE;
                else if (!strncmp(f, "SCMS", 4))
                    flags |= Track::Flag::SCMS;
            }
            file.tracks.back().flags = flags;
        } else if (!strncmp(token, "CATALOG", 7)) {
            // we should have two tokens, CATALOG <number>
            const auto [ number, numberok ] = state.number<uint64_t>(1);
            if (numberok) {
                out.catalog = number;
            }
        } else if (!strncmp(token, "CDTEXTFILE", 10)) {
            // we should have two tokens, CDTEXTFILE filename
            const auto fn = state.token(1);
            if (fn) {
                out.cdtextfile = fn;
            }
        }
    }

    return out;
}

CueSheet parseFile(const fs::path& filename)
{
    std::string data;
    {
        std::ifstream f(filename, std::ios::in | std::ios::binary);

        const auto sz = fs::file_size(filename);

        data.resize(sz);
        f.read(data.data(), sz);
    }
    return parse(data);
}

} // namespace CueParser

#include <CueParser.h>
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc == 1) {
        fprintf(stderr, "need a .cue file name\n");
        return 1;
    }

    const auto sheet = CueParser::parseFile(argv[1]);
    if (sheet.catalog) {
        printf("catalog %llu\n", static_cast<unsigned long long>(*sheet.catalog));
    }
    for (const auto& file : sheet.files) {
        printf("file '%s' type 0x%x\n", file.filename.c_str(), static_cast<uint32_t>(file.type));
        for (const auto& track : file.tracks) {
            printf("- track %u type 0x%x\n", track.trackNo, static_cast<uint32_t>(track.type));
            if (track.pregap) {
                printf(" pregap %u:%u:%u\n", track.pregap->mm, track.pregap->ss, track.pregap->ff);
            }
            for (const auto& idx : track.index) {
                printf(" - index %u %u:%u:%u\n", idx.number, idx.length.mm, idx.length.ss, idx.length.ff);
            }
            if (track.postgap) {
                printf(" postgap %u:%u:%u\n", track.postgap->mm, track.postgap->ss, track.postgap->ff);
            }
        }
    }

    return 0;
}

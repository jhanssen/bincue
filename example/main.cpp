#include <CueParser.h>
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc == 1) {
        fprintf(stderr, "need a .cue file name\n");
        return 1;
    }

    const auto sheet = CueParser::parseFile(argv[1]);
    if (!sheet.cdtextfile.empty()) {
        printf("cdtextfile '%s'\n", sheet.cdtextfile.c_str());
    }
    if (sheet.catalog) {
        printf("catalog %llu\n", static_cast<unsigned long long>(*sheet.catalog));
    }
    if (!sheet.title.empty()) {
        printf("title '%s'\n", sheet.title.c_str());
    }
    if (!sheet.performer.empty()) {
        printf("performer '%s'\n", sheet.performer.c_str());
    }
    if (!sheet.songwriter.empty()) {
        printf("songwriter '%s'\n", sheet.songwriter.c_str());
    }
    for (const auto& file : sheet.files) {
        printf("file '%s' type 0x%x\n", file.filename.c_str(), static_cast<uint32_t>(file.type));
        for (const auto& track : file.tracks) {
            printf("- track %u type 0x%x\n", track.number, static_cast<uint32_t>(track.type));
            if (track.pregap) {
                printf(" pregap %u:%u:%u\n", track.pregap->mm, track.pregap->ss, track.pregap->ff);
            }
            for (const auto& idx : track.index) {
                printf(" - index %u %u:%u:%u\n", idx.number, idx.length.mm, idx.length.ss, idx.length.ff);
            }
            if (track.postgap) {
                printf(" postgap %u:%u:%u\n", track.postgap->mm, track.postgap->ss, track.postgap->ff);
            }
            if (track.flags != 0) {
                printf(" flag");
                if (track.flags & CueParser::Track::Flag::DCP) {
                    printf(" dcp");
                }
                if (track.flags & CueParser::Track::Flag::CH4) {
                    printf(" 4ch");
                }
                if (track.flags & CueParser::Track::Flag::PRE) {
                    printf(" pre");
                }
                if (track.flags & CueParser::Track::Flag::SCMS) {
                    printf(" scms");
                }
                printf("\n");
            }
        }
    }
    for (const auto& comment : sheet.comments) {
        printf("rem '%s' '%s'\n", comment.tag.c_str(), comment.value.c_str());
    }

    return 0;
}

#ifndef READINFO_H
#define READINFO_H

#include <string>
/**
 * Reading meta info of a track
 */
struct AppState;

class MetaReader {
public:
    MetaReader()=default;
    virtual ~MetaReader()=default;

    void startreading();

    void readinto(AppState *state);

    void stopreading();

    void configchanged();

protected:
    void read(AppState *state);
    void fill_from(std::string item, AppState *state);
    FILE *file{nullptr};
    int fd{0};
    bool opened{false};
    std::string accumulator;
    char buf[100001];
};

extern MetaReader MetaReaderInstance;

#endif

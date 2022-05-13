#ifndef READINFO_H
#define READINFO_H

#include <string>

struct AppState;

/// Reading meta info of a track
class MetaReader {
public:
    ///default constructor
    MetaReader()=default;

    ///default destructor
    virtual ~MetaReader()=default;

    ///sets up start of reading
    void startreading();

    ///reads into variable
    void readinto(AppState *state);

    ///finalizes reading
    void stopreading();

    ///signal to reread config
    void configchanged();

protected:
    ///internal implementation of read
    void read(AppState *state);

    ///parse data line
    void fill_from(std::string item, AppState *state);

    ///meta file
    FILE *file{nullptr};

    ///meta file file descriptor
    int fd{0};

    ///is meta file opened
    bool opened{false};

    ///accomulator of meta lines
    std::string accumulator;

    ///utility buffer
    char buf[100001];
};

extern MetaReader MetaReaderInstance;

#endif

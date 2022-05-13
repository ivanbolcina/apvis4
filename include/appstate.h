#ifndef _APPSTATE
#define _APPSTATE

#include <vector>
#include <chrono>
#include <ctime>

/// Tracks application state
struct AppState {

    ///default constructor
    AppState() = default;

    ///id of image
    int image_id;

    ///id of track
    uint64_t id;

    ///termporary id, for in-processing, before final message is received. Then it is copied to id
    uint64_t tmpid;

    ///name of song
    std::string song_name;

    ///image of album
    std::vector<unsigned char> image;

    ///name of artist
    std::string artist;

    ///name of album
    std::string album;

    ///progress of song from 0..1
    float progress;

    ///start of playing
    std::chrono::system_clock::time_point t_start;

    ///end of playing
    std::chrono::system_clock::time_point t_end;

    ///current time pointing
    std::chrono::system_clock::time_point t_current;

    ///is song playing now
    bool playing;

    ///Read data from meta file. This should be done periodically.
    void read();

    ///recalculate progress
    void update_progres();
};

///Variable with global application state
extern AppState state;

#endif
 

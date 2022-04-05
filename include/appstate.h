#ifndef _APPSTATE
#define _APPSTATE

#include <vector>
#include <chrono>
#include <ctime>

/**
 * Tracks application state
 */
struct AppState {
    AppState() = default;
    int image_id;
    uint64_t id;
    uint64_t tmpid;
    std::string song_name;
    std::vector<unsigned char> image;
    std::string artist;
    std::string album;
    float progress;
    std::chrono::system_clock::time_point t_start;
    std::chrono::system_clock::time_point t_end;
    std::chrono::system_clock::time_point t_current;
    bool playing;
    void read();
    void updateprogres();
};

extern AppState state;

#endif
 

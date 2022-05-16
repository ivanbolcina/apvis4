#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <plog/Log.h>

#include "readinfo.h"
#include "appstate.h"
#include "utils.h"

using json = nlohmann::json;
using namespace std;

constexpr uint32_t _mper=1836082546;
constexpr uint32_t _mden=1835296110;
constexpr uint32_t _prgr=1886545778;
constexpr uint32_t _prsm=1886548845;
constexpr uint32_t _pbeg=1885496679;
constexpr uint32_t _pend=1885695588;
constexpr uint32_t _abeg=1633838439;
constexpr uint32_t _aend=1634037348;
constexpr uint32_t _pffr=1885759090;
constexpr uint32_t _asul=1634956652;
constexpr uint32_t _asal=1634951532;
constexpr uint32_t _asar=1634951538;
constexpr uint32_t _ascm=1634952045;
constexpr uint32_t _minm=1835626093;
constexpr uint32_t _PICT=1346978644;
constexpr uint32_t _VOLU=1886809964;

void MetaReader::fill_from(string item, AppState *state) {

    uint32_t type, code, length;
    int ret = sscanf(item.c_str(), "<item><type>%8x</type><code>%8x</code><length>%u</length>", &type, &code, &length);
    if (ret == 3) {
        size_t outputlength = 0;
        char *payload = nullptr;
        if (length > 0) {
            auto idx = item.find("<data");
            auto idxafter = item.find(">", idx) + 1;
            auto idxend = item.find("</data", idxafter);
            auto base64 = item.substr(idxafter, idxend - idxafter);
            outputlength = length;
            payload = (char *) malloc(length + 1);
            if (base64_decode(base64.c_str(), base64.length(), (unsigned char *) payload, &outputlength) != 0) {
                free(payload);
                PLOG_ERROR << "Failed to decode it.";
                return;
            }
            payload[length] = 0;
        } else {
            payload = (char *) malloc(1);
            payload[0] = 0;
        }
        PLOG_DEBUG.printf("type=%8x,code=%8x", type, code);
        if (payload != nullptr && strlen(payload) < 1000) { PLOG_DEBUG<<"data:"<<payload; }
        switch (code) {
            case _mper: {
                uint64_t vl = ntohl(*(uint32_t *) payload);
                vl = vl << 32;
                uint64_t ul = ntohl(*(uint32_t *) (payload + sizeof(uint32_t)));
                vl = vl + ul;
                state->tmpid = vl;
            }
                break;
            case _mden: {
                state->id = state->tmpid;
            }
                break;
            case _prgr:
                {
                    //s,cur,end
                    string s = payload;
                    string delimiter = "/";
                    size_t pos = 0;
                    std::string token;
                    double vals[3];
                    int vcnt = 0;
                    while ((pos = s.find(delimiter)) != std::string::npos) {
                        token = s.substr(0, pos);
                        PLOG_INFO << "progress token:" << token;
                        vals[vcnt++] = atoll(token.c_str()) / 44100.0;
                        s.erase(0, pos + delimiter.length());
                    }
                    if (s.length() != 0) vals[vcnt++] = atol(s.c_str()) / 44100.0;
                    if (vcnt == 3) {
                        state->playing = true;
                        state->t_current = std::chrono::system_clock::now();
                        auto already_playing = vals[1] - vals[0];
                        auto total_playing = vals[2] - vals[0];
                        state->t_start = state->t_current - std::chrono::milliseconds((long) (1000 * already_playing));
                        state->t_end = state->t_start + std::chrono::milliseconds((long) (1000 * total_playing));
                        vals[1] -= vals[0];
                        vals[2] -= vals[0];
                        if (vals[2] != 0)
                            state->progress = vals[1] / vals[2];
                        else
                            state->progress = 0.0f;
                    } else state->progress = 0.0f;
                }
                break;
            case _VOLU: {
                PLOG_INFO << "volume:" << payload;
                string s = payload;
                string delimiter = ",";
                size_t pos = 0;
                std::string token;
                double vals[3];
                int vcnt = 0;
                while ((pos = s.find(delimiter)) != std::string::npos) {
                    token = s.substr(0, pos);
                    PLOG_INFO << "progress token:" << token;
                    vals[vcnt++] = atof(token.c_str());
                    s.erase(0, pos + delimiter.length());
                }
                if (vcnt == 3) {
                    state->volume_in_decibels=vals[0];
                }
            }
                break;
            case _prsm:
                state->playing = true;
                break;
            case _pbeg:
                state->playing = true;
                break;
            case _pend:
                state->playing = false;
                break;
            case _abeg:
                state->playing = true;
                break;
            case _aend:
                state->playing = false;
                break;
            case _pffr:
                state->playing = true;
                break;
            case _asul:
                //printf("URL: \"%s\".\n", payload);
                //fflush(stdout);
                break;
            case _asal:
                state->album = payload;
                break;
            case _asar:
                state->artist = payload;
                break;
            case _ascm:
                //printf("Comment: \"%s\".\n",payload);
                break;
            case _minm:
                state->song_name = payload;
                break;
            case _PICT:
                {
                    PLOG_INFO <<"****************************************picture";
                    if (outputlength>2000) {
                        state->image_id = 0;
                        state->image.clear();
                        state->image.reserve(outputlength);
                        std::copy(payload, payload + outputlength, std::back_inserter(state->image));
                        for (int i = 0; i < 2000 && i < outputlength; i++) state->image_id += payload[i];
                    }
                }
                break;
        }
        free(payload);
    }
}

void MetaReader::read(AppState *state) {
    try {
        if (!opened) {
            std::ifstream inputconfig("settings.json");
            string metafile = "";
            if (inputconfig.good()) {
                json settings;
                inputconfig >> settings;
                metafile = settings["metafile"];
            }
            if (metafile=="") metafile="/tmp/shairport-sync-metadata";
            fd = open(metafile.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd <= 0) return;
            file = fdopen(fd, "r");
            opened = true;
        }
        auto buflen = 100000;
        auto fetched = false;
        auto gotlines = 0;
        while (!fetched) {
            memset(buf, 0, buflen + 1);
            fgets(buf, buflen, file);
            string line{buf};
            if (!line.empty() && line[line.length() - 1] == '\n')
                line.erase(line.length() - 1);
            if (line.empty()) break;
            gotlines++;
            accumulator.append(line);
            fetched = (accumulator.find("<item>") != string::npos && accumulator.find("</item>") != string::npos);
            if (gotlines > 200) break;
        }
        while (fetched) {
            auto idx = accumulator.find("<item>");
            auto idx2 = accumulator.find("</item>");
            auto item = accumulator.substr(idx, idx2 - idx + 7);
            fill_from(item, state);
            accumulator.erase(0, idx2 + 7);
            fetched = (accumulator.find("<item>") != string::npos && accumulator.find("</item>") != string::npos);
        }
    }
    catch (std::exception &ex) {
        PLOG_ERROR <<"error "<<ex.what();
    }
}


void MetaReader::startreading() {
    opened = false;
}

void MetaReader::readinto(AppState *state) {
    read(state);
}

void MetaReader::stopreading() {
    if (fd > 0) fclose(file);
}

void MetaReader::configchanged() {
    stopreading();
    fd = 0;
    opened = false;
}

MetaReader MetaReaderInstance;
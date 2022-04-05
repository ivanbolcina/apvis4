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
#include "readinfo.h"
#include "appstate.h"
#include "utils.h"

using json = nlohmann::json;
using namespace std;

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
                printf("Failed to decode it.\n");
                return;
            }
            payload[length] = 0;
        } else {
            payload = (char *) malloc(1);
            payload[0] = 0;
        }
        printf("\n\ntype=%8x,code=%8x=\n", type, code);
        if (payload != nullptr && strlen(payload) < 1000) printf("data=%s\n", payload);
        fflush(stdout);
        switch (code) {
            case 'mper': {
                uint64_t vl = ntohl(*(uint32_t *) payload);
                vl = vl << 32;
                uint64_t ul = ntohl(*(uint32_t *) (payload + sizeof(uint32_t)));
                vl = vl + ul;
                state->tmpid = vl;
            }
                break;
            case 'mden': {
                state->id = state->tmpid;
            }
                break;
            case 'prgr':
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
                        std::cout << token << std::endl;
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
            case 'prsm':
                state->playing = true;
                break;
            case 'pbeg':
                state->playing = true;
                break;
            case 'pend':
                state->playing = false;
                break;
            case 'abeg':
                state->playing = true;
                break;
            case 'aend':
                state->playing = false;
                break;
            case 'pffr':
                state->playing = true;
                break;
            case 'asul':
                printf("URL: \"%s\".\n", payload);
                fflush(stdout);
                break;
            case 'asal':
                state->album = payload;
                break;
            case 'asar':
                state->artist = payload;
                break;
            case 'ascm':
                //printf("Comment: \"%s\".\n",payload);
                break;
            case 'minm':
                state->song_name = payload;
                break;
            case 'PICT':
                printf("picture: \n");
                fflush(stdout);
                state->image_id++;
                state->image.clear();
                state->image.reserve(outputlength);
                std::copy(payload, payload + outputlength, std::back_inserter(state->image));
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
        printf("error\n");
        fflush(stdout);
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
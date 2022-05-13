#ifndef VUMETER_H
#define VUMETER_H

#include <vector>

/**
 * Channel info
 */
class ChannelInfo {
public:
    ChannelInfo(const std::string &l);
    std::string label;
    double fraction;
};

/**
 * Data from channel
 */
class SampleData {
public:
    SampleData() = default;
    virtual ~SampleData() = default;
    std::vector<float> data;
    std::vector<float> freq;
    int lenght;
    int channels;
    double volume_in_decibels;
    void addData(const float *d, unsigned samples);
};

/**
 * Reading data from sound buffers
 */
class VU {
    void *meter;
public:
    VU();
    virtual ~VU();
    void start();
    void end();
    void tick();
    std::vector<ChannelInfo *> get();
    std::shared_ptr<SampleData> getSample();
};

#endif

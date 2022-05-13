#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <string>
#include <string.h>
#include <vector>
#include <deque>
#include <cmath>
#include <pulse/pulseaudio.h>
#include <iostream>
#include <memory>
#include <plog/Log.h>
#include "vumeter.h"

using namespace std;

#define LOGARITHMIC 1

/**
 * worker class
 */
class VUMeter {
    friend class VU;

public:
    VUMeter();
    virtual ~VUMeter();
    pa_mainloop *ml;
    pa_context *context = nullptr;
    pa_stream *stream = nullptr;
    char *device_name = nullptr;
    char *device_description = nullptr;
    void start();
    void stop();
    virtual void tick();
    void add(const pa_channel_map &map, const char *source_name, const char *description);

protected:
    int running;
    bool started;
    float *levels;
    std::vector<ChannelInfo *> channels;
    virtual bool on_display_timeout();
    virtual bool on_calc_timeout();
    virtual void decayChannels();
    pa_usec_t latency;

    class LevelInfo {
    public:
        LevelInfo(float *levels, pa_usec_t l);
        virtual ~LevelInfo();
        bool elapsed();
        struct timeval tv;
        float *levels;
    };

    std::deque<LevelInfo *> levelQueue;
    std::shared_ptr<SampleData> sampleData;
public:
    virtual void pushData(const float *d, unsigned l);
    virtual void showLevels(const LevelInfo &i);
    virtual void updateLatency(pa_usec_t l);
};

/**
 * Info about channel
 */
ChannelInfo::ChannelInfo(const string &l) {
    label = l;
}

/**
 * Log errors
 */
void show_error(const char *txt, bool show_pa_error, VUMeter *vuMeter) {
    char buf[256];
    if (show_pa_error)
        snprintf(buf, sizeof(buf), "%s: %s", txt, pa_strerror(pa_context_errno(vuMeter->context)));
    PLOG_ERROR << buf;
}

/**
 * callback for getting stream latency and stores it
 */
static void stream_update_timing_info_callback(pa_stream *s, int success, void *ref) {
    pa_usec_t t;
    VUMeter *vuMeter = static_cast<VUMeter *>(ref);
    int negative = 0;
    if (!success || pa_stream_get_latency(s, &t, &negative) < 0) {
        show_error("Failed to get latency information", true, vuMeter);
        return;
    }
    if (!vuMeter) return;
    vuMeter->updateLatency(negative ? 0 : t);
}

/**
 * Gets stream latency and stores it
 */
static bool latency_func(void *ref) {
    VUMeter *vuMeter = static_cast<VUMeter *>(ref);
    pa_operation *o;
    if (!vuMeter->stream) return false;
    if (!(o = pa_stream_update_timing_info(vuMeter->stream, stream_update_timing_info_callback, vuMeter))) {
        show_error("latency timing error", true, vuMeter);
    } else
        pa_operation_unref(o);
    return true;
}

/**
 * Callback for getting stream data
 */
static void stream_read_callback(pa_stream *s, size_t l, void *ref) {
    const void *p;
    VUMeter *vuMeter = static_cast<VUMeter *>(ref);
    if (pa_stream_peek(s, &p, &l) < 0) {
        PLOG_ERROR << "peek error";
        return;
    }
    vuMeter->pushData((const float *) p, l / sizeof(float));
    pa_stream_drop(s);
}

/**
 * Callback for getting stream state
 */
static void stream_state_callback(pa_stream *s, void *ref) {
    PLOG_INFO << "got stream callback";
    VUMeter *vuMeter = static_cast<VUMeter *>(ref);
    switch (pa_stream_get_state(s)) {
        case PA_STREAM_UNCONNECTED:
        case PA_STREAM_CREATING:
            break;
        case PA_STREAM_READY:
            PLOG_INFO <<"stream ready";
            vuMeter->add(*pa_stream_get_channel_map(s), vuMeter->device_name, vuMeter->device_description);
            latency_func(ref);
            break;
        case PA_STREAM_FAILED:
            show_error("Connection failed", true, vuMeter);
            break;
        case PA_STREAM_TERMINATED:
            //TODO: what
            break;
    }
}

/**
 * Creates stream, after get sink info callback
 */
static void
create_stream(const char *name, const char *description, const pa_sample_spec &ss, const pa_channel_map &cmap,
              VUMeter *vuMeter) {
    pa_sample_spec nss;
    vuMeter->device_name = strdup(name);
    vuMeter->device_description = strdup(description);
    PLOG_INFO << "creating stream";
    nss.format = PA_SAMPLE_FLOAT32;
    nss.rate = ss.rate;
    nss.channels = ss.channels;
    vuMeter->stream = pa_stream_new(vuMeter->context, "PulseAudio Volume Meter", &nss, &cmap);
    pa_stream_set_state_callback(vuMeter->stream, stream_state_callback, vuMeter);
    pa_stream_set_read_callback(vuMeter->stream, stream_read_callback, vuMeter);
    auto res = pa_stream_connect_record(vuMeter->stream, name, NULL, (enum pa_stream_flags) 0);
    if (res < 0) {
        show_error("connect erro...", true, vuMeter);
    }
}

/**
 * After getting info about sink -> create stream
 */
static void context_get_sink_info_callback(pa_context *, const pa_sink_info *si, int is_last, void *ref) {
    VUMeter *vuMeter = static_cast<VUMeter *>(ref);
    if (is_last < 0) {
        show_error("Failed to get sink information 2", true, vuMeter);
        return;
    }
    if (!si) return;
    create_stream(si->monitor_source_name, si->description, si->sample_spec, si->channel_map, vuMeter);
}

/**
 * After get info about PA server -> get default sink
 */
static void context_get_server_info_callback(pa_context *c, const pa_server_info *si, void *ref) {
    VUMeter *vuMeter = static_cast<VUMeter *>(ref);
    if (!si) {
        show_error("Failed to get server information", true, vuMeter);
        return;
    }
    if (!si->default_sink_name) {
        show_error("No default sink set.", false, vuMeter);
        return;
    }
    PLOG_INFO << "default sink name:" << si->default_sink_name;
    pa_operation_unref(
            pa_context_get_sink_info_by_name(c, si->default_sink_name, context_get_sink_info_callback, vuMeter));
}

/**
 * After context state ready -> get infor about PA server
 */
static void context_state_callback(pa_context *c, void *ref) {
    VUMeter *vuMeter = static_cast<VUMeter *>(ref);
    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
        case PA_CONTEXT_READY:
            pa_operation_unref(pa_context_get_server_info(c, context_get_server_info_callback, vuMeter));
            break;
        case PA_CONTEXT_FAILED:
            show_error("Connection failed", true, vuMeter);
            break;
        case PA_CONTEXT_TERMINATED:
            break;
    }
}

/**
 * Creates void VUMeter::add(const pa_channel_map& map, const char* source_name, const char* description)
 */
VUMeter::VUMeter() : latency(0), running(0), started(false), levels(nullptr), stream(nullptr), context(nullptr),
                     sampleData(make_shared<SampleData>()) {
}

/**
 * Stops and destructs
 */
VUMeter::~VUMeter() {
    stop();
}

/**
 * Adds channel
 */
void VUMeter::add(const pa_channel_map &map, const char *, const char *description) {
    latency = 0;
    char t[256];
    int n;
    for (n = 0; n < map.channels; n++) {
        snprintf(t, sizeof(t), "<b>%s</b>", pa_channel_position_to_pretty_string(map.map[n]));
        PLOG_INFO << "Channel:" <<t;
        channels.push_back(new ChannelInfo(t));
    }
    running = 1;
}

/**
 * Starts connecting
 */
void VUMeter::start() {
    started = true;
    device_name = strdup("PULSE_SINK");
    ml = pa_mainloop_new();
    context = pa_context_new(pa_mainloop_get_api(ml), "PulseAudio Volume Meter");
    pa_context_set_state_callback(context, context_state_callback, this);
    pa_context_connect(context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
}

/**
 * Remove connections
 */
void VUMeter::stop() {
    try {
        while (channels.size() > 0) {
            ChannelInfo *i = channels.back();
            channels.pop_back();
            delete i;
        }
        while (levelQueue.size() > 0) {
            LevelInfo *i = levelQueue.back();
            levelQueue.pop_back();
            delete i;
        }
        if (levels)
            delete[] levels;
        if (stream)
            pa_stream_unref(stream);
        if (context)
            pa_context_unref(context);
        if (ml)
            pa_mainloop_quit(ml, 0);
    } catch (std::exception &ex) {
        show_error("stop error", true, this);
    }
    levels = nullptr;
    stream = nullptr;
    context = nullptr;
    ml = nullptr;
}

/**
 * Main loop iterate
 */
void VUMeter::tick() {
    int retval;
    pa_mainloop_iterate(ml, 0, &retval);
    if (started) {
        on_display_timeout();
        on_calc_timeout();
    }
    if (running == 1) {
        latency_func(this);
        running = 2;
    }
}

/**
 * Push max data into from stream into levels current val, in callback of reading stream data
 */
void VUMeter::pushData(const float *d, unsigned samples) {
    unsigned nchan = channels.size();
    if (!levels) {
        levels = new float[nchan];
        for (unsigned c = 0; c < nchan; c++)
            levels[c] = 0;
    }
    sampleData->channels = nchan;
    sampleData->lenght = samples;
    sampleData->addData(d, samples);
    while (samples >= nchan) {
        for (unsigned c = 0; c < nchan; c++) {
            float v = fabs(d[c]);
            if (v > levels[c]) {
                levels[c] = v;
            }
        }
        d += nchan;
        samples -= nchan;
    }
}

/**
 * Copy data from level info into channel info
 */
void VUMeter::showLevels(const LevelInfo &i) {
    unsigned nchan = channels.size();
    for (unsigned n = 0; n < nchan; n++) {
        double level;
        ChannelInfo *c = channels[n];
        level = i.levels[n];
#ifdef LOGARITHMIC
        level = log10(level * 9 + 1);
#endif
        c->fraction = (level > 1 ? 1 : level);
    }
}

#define DECAY_LEVEL (0.005)

/**
 * Dacays channel values
 */
void VUMeter::decayChannels() {
    unsigned nchan = channels.size();
    for (unsigned n = 0; n < nchan; n++) {
        double level;
        ChannelInfo *c = channels[n];
        level = c->fraction;
        if (level <= 0)
            continue;
        level = level > DECAY_LEVEL ? level - DECAY_LEVEL : 0;
        c->fraction = level;
    }
}

/**
 * Prepares levels for display:
 * - it decals
 * - finds un-elapsed chunk, erases others
 * - shows level from it
 */

bool VUMeter::on_display_timeout() {
    LevelInfo *i = NULL;
    if (levelQueue.empty()) {
        decayChannels();
        return true;
    }
    while (levelQueue.size() > 0) {
        if (i)
            delete i;
        i = levelQueue.back();
        levelQueue.pop_back();
        if (!i->elapsed())
            break;
    }
    if (i) {
        showLevels(*i);
        delete i;
    }
    return true;
}

/**
 * If some levels were received, save them in queue
 */
bool VUMeter::on_calc_timeout() {
    if (levels) {
        levelQueue.push_front(new LevelInfo(levels, latency));
        levels = NULL;
    }
    return true;
}

/**
 * Saves latency
 */
void VUMeter::updateLatency(pa_usec_t l) {
    latency = l;
}

/**
 * Adds some micro-seconds to time inteval
 */
static void timeval_add_usec(struct timeval *tv, pa_usec_t v) {
    uint32_t sec = v / 1000000;
    tv->tv_sec += sec;
    v -= sec * 1000000;
    tv->tv_usec += v;
    while (tv->tv_usec >= 1000000) {
        tv->tv_sec++;
        tv->tv_usec -= 1000000;
    }
}

/**
 * level data
 */
VUMeter::LevelInfo::LevelInfo(float *l, pa_usec_t latency) {
    levels = l;
    gettimeofday(&tv, NULL);
    timeval_add_usec(&tv, latency);
}

/**
 * clean up of level data
 */
VUMeter::LevelInfo::~LevelInfo() {
    delete[] levels;
}

/**
 * is level elapsed
 */
bool VUMeter::LevelInfo::elapsed() {
    struct timeval now;
    gettimeofday(&now, NULL);
    if (now.tv_sec != tv.tv_sec)
        return now.tv_sec > tv.tv_sec;
    return now.tv_usec >= tv.tv_usec;
}


void SampleData::addData(const float *d, unsigned int samples) {
    int w = 4096;
    int curr = data.size();
    samples /= channels;
    int newdatalen = samples;
    if (newdatalen > w) newdatalen = w;
    //int idx = 0;
    auto idx=0;
    int offset = curr + newdatalen - w;
    int tomove = w - offset;
    for (idx = 0; idx < tomove && idx < data.size(); idx++)
        data[idx] = data[idx + offset];
    for (int i = 0; i < newdatalen; i++) {
        auto pos = samples - newdatalen + i;
        pos *= channels;
        float var = d[pos];
        if (idx >= data.size())
            data.push_back(var);
        else
            data[idx++] = var;
    }
}


///////////////////////////////////////////////////////////////
// user level api
///////////////////////////////////////////////////////////////

VU::VU() {
    meter = new VUMeter();
}

VU::~VU() {
    static_cast<VUMeter *>(meter)->stop();
    delete static_cast<VUMeter *>(meter);
}

void VU::start() {
    static_cast<VUMeter *>(meter)->start();
}

void VU::end() {
    static_cast<VUMeter *>(meter)->stop();
}

void VU::tick() {
    static_cast<VUMeter *>(meter)->tick();
}

std::vector<ChannelInfo *> VU::get() {
    return static_cast<VUMeter *>(meter)->channels;
}

std::shared_ptr<SampleData> VU::getSample() {
    return static_cast<VUMeter *>(meter)->sampleData;
}

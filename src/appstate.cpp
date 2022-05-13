#include <string>

#include "appstate.h"
#include "readinfo.h"

AppState state;

void AppState::read() {
    MetaReaderInstance.readinto(this);
}

void AppState::update_progres() {
    if (playing) {
        auto curr = std::chrono::system_clock::now();
        auto diff = curr - state.t_start;
        auto all = state.t_end - state.t_start;
        if (all != std::chrono::milliseconds::zero()) {
            auto proc = 1. * diff / all;
            if (proc < 0.) proc = 0.;
            if (proc > 1.) proc = 1.;
            state.progress = proc;
        }
    }
}

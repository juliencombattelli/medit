#ifndef MEDIT_UI_SDL3_PERF_COUNTER_H_
#define MEDIT_UI_SDL3_PERF_COUNTER_H_

#include <SDL3/SDL.h>

// Measure input-to-display latency.
//
// Measurement should be started by perf_counter_frame_begin just after receiving any input, and
// stopped by perf_counter_frame_end after the rendering is completed. Any unwanted frame (ie. input
// that do not change the screen) might be discarded by perf_counter_frame_discard.

typedef struct PerfCounter PerfCounter;
struct PerfCounter {
    Uint64 frame_count;
    Uint64 frame_start_ns;
    Uint64 accumulated_ns;
    SDL_TimerID timer;
    void (*report_cb)(PerfCounter* pc, void* userdata);
    void* userdata;
};

void perf_counter_frame_begin(PerfCounter* perf_counter);
void perf_counter_frame_end(PerfCounter* perf_counter);
void perf_counter_frame_discard(PerfCounter* perf_counter);

void perf_counter_start_periodic_report(
    PerfCounter* perf_counter,
    Uint32 report_interval_ms,
    void (*report_cb)(PerfCounter* pc, void* userdata),
    void* userdata);
void perf_counter_stop_periodic_report(PerfCounter* perf_counter);

#endif // MEDIT_UI_SDL3_PERF_COUNTER_H_

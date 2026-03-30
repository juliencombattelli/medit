#include "perf_counter.h"

#include "utils.h"

static Uint32 perf_counter_report_cb(void* userdata, SDL_TimerID timer_id, Uint32 interval)
{
    MEDIT_UNUSED(timer_id);

    PerfCounter* perf_counter = userdata;

    if (perf_counter->report_cb != NULL) {
        (*perf_counter->report_cb)(perf_counter, perf_counter->userdata);
    }

    perf_counter->frame_count = 0;
    perf_counter->accumulated_ns = 0;

    return interval;
}

void perf_counter_frame_begin(PerfCounter* perf_counter)
{
    perf_counter->frame_start_ns = SDL_GetTicksNS();
}

void perf_counter_frame_end(PerfCounter* perf_counter)
{
    if (perf_counter->frame_start_ns != 0) {
        perf_counter->frame_count++;
        Uint64 now = SDL_GetTicksNS();
        perf_counter->accumulated_ns += now - perf_counter->frame_start_ns;
    }
}

void perf_counter_frame_discard(PerfCounter* perf_counter)
{
    perf_counter->frame_start_ns = 0;
}

void perf_counter_start_periodic_report(
    PerfCounter* perf_counter,
    Uint32 report_interval_ms,
    void (*report_cb)(PerfCounter* pc, void* userdata),
    void* userdata)
{
    *perf_counter = (PerfCounter) {
        .report_cb = report_cb,
        .userdata = userdata,
    };
    perf_counter->timer = SDL_AddTimer(report_interval_ms, perf_counter_report_cb, perf_counter);
}

void perf_counter_stop_periodic_report(PerfCounter* perf_counter)
{
    SDL_RemoveTimer(perf_counter->timer);
}

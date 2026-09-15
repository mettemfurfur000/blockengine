#ifdef __linux__
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include <stdio.h>

void perf_write_start(void)
{
    FILE *f = fopen("perf_frame.txt", "w");
    if (f)
    {
        fprintf(f, "frame_ms,merged_draw,dirty\n");
        fclose(f);
    }
}

void perf_write_frame(f32 ms, u32 merged_count, int dirty)
{
    static int count = 0;
    if (count >= 120)
        return;
    count++;
    FILE *f = fopen("perf_frame.txt", "a");
    if (f)
    {
        fprintf(f, "%.3f,%u,%d\n", ms, merged_count, dirty);
        fclose(f);
    }
}

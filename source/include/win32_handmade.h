/* date = March 16th 2021 6:40 am */

#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

struct win32_offscreen_buffer 
{
    BITMAPINFO info;
    void* memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct win32_window_dimension 
{
    int width;
    int height;
};

struct win32_sound_output 
{
    // NOTE(1337): for sound test
    int sample_per_second;
    u32 running_sample_index;
    int bytes_per_sample;
    int secondary_buffer_size;
    f32 t_sine;
    int latency_sample_count;
};

#endif //WIN32_HANDMADE_H

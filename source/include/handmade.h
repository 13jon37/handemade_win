/* date = December 22nd 2020 4:39 am */

#ifndef HANDMADE_H
#define HANDMADE_H

/*
 NOTE(Jon): 
HANDMADE_INTERNAL:
0 - Build for public release
1 - Build for devleoper only

HANDMADE_SLOW:
0 - No slow code allowed!
1 - Slow code welcome!
*/

#if HANDEMADE_SLOW
#define Assert(expression) if (!(expression)) {*(int*)0 = 0;}
#else
#define Assert(expression)
#endif
// If expression isn't true right to null pointer and crash the program

#define Kilobytes(value) ((value)*1024)
#define Megabytes(value) (Kilobytes(value)*1024)
#define Gigabytes(value) (Megabytes(value)*1024)
#define Terabytes(value) (Gigabytes(value)*1024)

#define array_count(array) (sizeof(array) / sizeof((array)[0]))

inline u32
safe_truncate_u64(u64 value)
{
    Assert(value <= 0xFFFFFFFF);
    u32 result = (u32)value;
    return result;
}

#if HANDMADE_INTERNAL
/* IMPORTANT(Jon): 
These are for not for doing anything in the shipping game. 
They are blocking and the write doesn't protect against lost data.
*/
struct debug_read_file_result_t 
{
    u32 content_size;
    void *contents;
};

// NOTE(1337): Services that the game platform layer provides to the game.
internal debug_read_file_result_t DEBUG_platform_read_entire_file(char *file_name);
internal void DEBUG_platform_free_file_memory(void *memory);
internal b32 DEBUG_platform_write_entire_file(char *file_name, u32 memory_size, void *memory);
#endif

// NOTE(1337): Services that the game provides to the platform layer.

// Four things - timing, controls, bitmap buffer to use, sound buffer to use

struct game_offscreen_buffer_t 
{
    void *memory;
    int width;
    int height;
    int pitch;
};

struct game_sound_output_buffer_t
{
    int samples_per_second;
    int sample_count;
    i16* samples;
};

struct game_button_state_t
{
    int half_transition_count;
    b32 ended_down;
};

struct game_controller_input_t
{
    b32 is_analog;
    
    f32 start_x;
    f32 start_y;
    
    f32 min_x;
    f32 min_y;
    
    f32 max_x;
    f32 max_y;
    
    f32 end_x;
    f32 end_y;
    
    union
    {
        game_button_state_t buttons[6];
        struct 
        {
            game_button_state_t up;
            game_button_state_t down;
            game_button_state_t left;
            game_button_state_t right;
            game_button_state_t left_shoulder;
            game_button_state_t right_shoulder;
            
        };
    };
};

struct game_input_t
{
    // TODO(Jon): Insert clock value here
    game_controller_input_t controllers[4];
};

struct game_memory_t 
{
    b32 is_initialized; 
    u64 permanent_storage_size;
    void *permanent_storage; // NOTE(Jon): Required to be cleared to zero at startup
    u64 transient_storage_size;
    void *transient_storage;
    
};

internal void 
game_update_and_render(game_memory_t *memory, game_input_t *input, game_offscreen_buffer_t *buffer,  game_sound_output_buffer_t *sound_buffer);

//////

struct game_state_t
{
    int tone_hz;
    int green_offset;
    int blue_offset;
};

#endif //HANDMADE_H

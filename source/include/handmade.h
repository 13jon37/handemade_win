/* date = December 22nd 2020 4:39 am */

#ifndef HANDMADE_H
#define HANDMADE_H

#define array_count(array) (sizeof(array) / sizeof((array)[0]))

// TODO(1337): Services that the game platform layer provides to the game.

// NOTE(1337): Services that the game provides to the platform layer.

// Four things - timing, controls, bitmap buffer to use, sound buffer to use

struct game_offscreen_buffer 
{
    void* memory;
    int width;
    int height;
    int pitch;
};

struct game_sound_output_buffer
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
    game_controller_input_t controllers[4];
    
};

internal void 
game_update_and_render(game_input_t* input, game_offscreen_buffer* buffer,  game_sound_output_buffer* sound_buffer);

#endif //HANDMADE_H

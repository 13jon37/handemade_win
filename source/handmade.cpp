#include "include/handmade.h"

internal void
game_output_sound(game_sound_output_buffer_t *sound_buffer, int tone_hz)
{
    local_persist f32 t_sine;
    i16 tone_volume = 3000;
    int wave_period = sound_buffer->samples_per_second / tone_hz;
    
    i16* sample_out = sound_buffer->samples;
    for (int sample_index = 0;
         sample_index < sound_buffer->sample_count; 
         ++sample_index)
    {
        f32 sine_value = sinf(t_sine);
        i16 sample_value = (i16)(sine_value *  tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        
        t_sine += 2.0f*pi_32*1.0f/(f32)wave_period;
    }
}

internal void
render_weird_gradient(game_offscreen_buffer_t *buffer, int x_offset, int y_offset)
{
    // TODO(1337): see what optimizer does
    
    u8* row = (u8*)buffer->memory;
    
    for (i32 y = 0; y < buffer->height; ++y)
    {
        u32* pixel = (u32*)row;
        
        for (i32 x = 0; x < buffer->width; ++x)
        {
            /*
mem: BB GG RR xx
regiter: xx RR GG BB

Pixel (32 bits)
*/
            
            u8 blue = (x + x_offset);
            u8 green = (y + y_offset);
            
            *pixel++ = ((green << 8) | blue);
        }
        
        row += buffer->pitch;
    }
}

internal void
game_update_and_render(game_memory_t *memory, game_input_t *input, game_offscreen_buffer_t *buffer, game_sound_output_buffer_t *sound_buffer)
{
    Assert(sizeof(game_state_t) <= memory->permanent_storage_size);
    
    game_state_t* game_state = (game_state_t*)memory->permanent_storage;
    if (!memory->is_initialized)
    {
        game_state->tone_hz = 256;
        game_state->green_offset = 0;
        game_state->blue_offset = 0;
        
        const char *file_name = __FILE__;
        
        debug_read_file_result_t file = DEBUG_platform_read_entire_file(file_name);
        if (file.contents)
        {
            DEBUG_platform_write_entire_file("test.out", file.content_size, file.contents);
            DEBUG_platform_free_file_memory(file.contents);
        }
        
        memory->is_initialized = true;
    }
    
    game_controller_input_t* input_0 = &input->controllers[0];
    if (input_0->is_analog)
    {
        // NOTE(Jon): Use analog movement tuning
        game_state->blue_offset += (int)4.0f * (input_0->end_x);
        game_state->tone_hz = 256 + (int)(128.0f * (input_0->end_y));
    }
    else
    {
        // NOTE(Jon): Use digital movement tuning
    }
    
    if (input_0->down.ended_down)
    {
        game_state->green_offset += 1;
    }
    
    // TODO(1337): Allow sample offsets here for more robust platform options
    game_output_sound(sound_buffer, game_state->tone_hz);
    render_weird_gradient(buffer, game_state->blue_offset, game_state->green_offset);
}
// TODO(1337): 

/*
 - Saved game locations
- Getting a handle to our own executable
- Asset loading path
- Threading (launch a screen)
- Raw Input (support for multiple keyboards)
- Sleep/timeBeginPeriod
- ClipCursor() for multimonitor supprt
- Fullscren support
- WM_SETCURSOR control cursor visibility
- QueryCancelAutoplay
- WM_ACTIVATEAPP (for when we are not active application)
- Blit Speed improvements
- Hardware accel
- GetKeyBoardLayout
*/

// NOTE(Jon): Next Ep: 15

#include <Windows.h>
#include <xinput.h>
#include <malloc.h>
#include <dsound.h>

// TODO(Jon): implement sine ourselves
#include <math.h>

#define pi_32 3.1415926535f

#include "include/language_layer.h"
#include "include/win32_handmade.h"
#include "handmade.cpp"

// NOTE(1337): a global for now
global_variable bool global_running; 
global_variable win32_offscreen_buffer global_back_buffer;
global_variable LPDIRECTSOUNDBUFFER global_secondary_buffer;

// NOTE(1337):  XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(1337): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI (name)(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
win32_load_input(void)
{
    HMODULE x_input_library = LoadLibraryA("xinput1_4.dll");
    if (!x_input_library)
    {
        // TODO(1337):  Diagnostic
        x_input_library = LoadLibraryA("xinput1_3.dll");
    }
    
    if (x_input_library)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(x_input_library, "XInputGetState");
        if (!XInputGetState) { XInputGetState = XInputGetStateStub; }
        
        XInputSetState = (x_input_set_state*)GetProcAddress(x_input_library, "XInputSetState");
        if (!XInputSetState) { XInputSetState = XInputSetStateStub; }
        
        // TODO(1337): Diagnostic
    }
    else
    {
        // TODO(1337): Diagnostic
    }
}

internal void
win32_init_dsound(HWND window, i32 samples_per_second, i32 buffer_size)
{
    // Load the library
    HMODULE dsound_library = LoadLibraryA("dsound.dll");
    
    if (dsound_library) {
        // Get Direct Sound object
        direct_sound_create* DirectSoundCreate = (direct_sound_create*) GetProcAddress(dsound_library, "DirectSoundCreate");
        
        // TODO(1337): Check if this works on earlier window versions
        LPDIRECTSOUND direct_sound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &direct_sound, 0)))
        {
            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.nSamplesPerSec = samples_per_second;
            wave_format.wBitsPerSample = 16;
            wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
            wave_format.cbSize = 0;
            
            if (SUCCEEDED(direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC buffer_description = {};
                buffer_description.dwSize = sizeof(buffer_description);
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                LPDIRECTSOUNDBUFFER primary_buffer;
                if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &primary_buffer, 0)))
                {
                    HRESULT error = primary_buffer->SetFormat(&wave_format);
                    if (SUCCEEDED(error))
                    {
                        // Finally have set the format
                        OutputDebugStringA("Primary Buffer format was set.\n");
                    }
                    else
                    {
                        // TODO(1337): Diagnostic
                        OutputDebugStringA("Failed to create primary sound buffer.\n");
                    }
                }
                else
                {
                    // TODO(1337):  Diagnostic
                }
            }
            else
            {
                // TODO(1337): Diagnostic
            }
            
            DSBUFFERDESC buffer_description = {};
            buffer_description.dwSize = sizeof(buffer_description);
            buffer_description.dwFlags = 0;
            buffer_description.dwBufferBytes = buffer_size;
            buffer_description.lpwfxFormat = &wave_format;
            
            HRESULT error = direct_sound->CreateSoundBuffer(&buffer_description, &global_secondary_buffer, 0);
            if (SUCCEEDED(error))
            {
                OutputDebugStringA("Created secondary sound buffer.\n");
            }
            
            // Start it playing
        }
        else
        {
            // TODO(1337): Diagnostic
        }
    }
}

internal win32_window_dimension
get_window_dimension(HWND window)
{
    win32_window_dimension result;
    
    RECT client_rect;
    GetClientRect(window, &client_rect); 
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;
    
    return result;
}

internal void
win32_resize_dib_section(win32_offscreen_buffer* buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    
    buffer->width = width;
    buffer->height = height;
    buffer->bytes_per_pixel = 4;
    
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;
    
    int bitmap_memory_size = (buffer->width * buffer->height) * buffer->bytes_per_pixel;
    
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width * buffer->bytes_per_pixel;
}

internal void
win32_disply_buffer_in_window(win32_offscreen_buffer* buffer, HDC device_context, 
                              int window_width, int window_height,
                              int x, int y, int width, int height)
{
    StretchDIBits(device_context,
                  /*
                  x, y, width, height, 
                  x, y, width, height,
    */
                  0, 0, window_width, window_height,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &buffer->info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

internal LRESULT CALLBACK
win32_main_window_callback(HWND   window,
                           UINT   message,
                           WPARAM w_param,
                           LPARAM l_param)
{
    LRESULT result = 0; 
    
    switch (message)
    {
        case WM_SIZE: 
        {
            OutputDebugStringA("WM_SIZE\n");
        } break;
        
        case WM_CLOSE:
        {
            global_running = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;
        
        case WM_DESTROY:
        {
            global_running = false;
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        
        case WM_SYSKEYDOWN:
        {
            
        } break;
        
        case WM_SYSKEYUP:
        {
            
        } break;
        
        case WM_KEYDOWN:
        {
            
        } break;
        
        case WM_KEYUP:
        {
            u32 vk_code = w_param;
            
            bool was_down = ((l_param & (1 << 30)) != 0);
            bool is_down =  ((l_param & (1 << 31)) == 0);
            
            if (was_down != is_down)
            {
                
                if (vk_code == 'W')
                {
                    
                }
                else if (vk_code == 'A')
                {
                    
                }
                else if (vk_code == 'S')
                {
                    
                }
                else if (vk_code == 'D')
                {
                    
                }
                else if (vk_code == VK_UP)
                {
                    
                }
                else if (vk_code == VK_DOWN)
                {
                    
                }
                else if (vk_code == VK_LEFT)
                {
                    
                }
                else if (vk_code == VK_DOWN)
                { 
                    
                }
                else if (vk_code == VK_SPACE)
                {
                    
                }
            }
            
            b32 alt_key_was_down = (l_param & (1 << 29));
            
            if ((vk_code == VK_F4) && alt_key_was_down)
            {
                global_running = false;
            }
            
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            
            win32_window_dimension dimension = get_window_dimension(window);
            win32_disply_buffer_in_window(&global_back_buffer, device_context, 
                                          dimension.width, dimension.height,
                                          x, y, width, height);
            
            PatBlt(device_context, x, y, width, height, BLACKNESS);
            EndPaint(window, &paint);
            
        } break;
        
        default:
        {
            result = DefWindowProc(window, message, w_param, l_param); 
        } break;
    }
    
    return result;
}

internal void
win32_clear_buffer(win32_sound_output* sound_output)
{
    VOID* region1;
    DWORD region1_size;
    VOID* region2;
    DWORD region2_size;
    
    if(SUCCEEDED(global_secondary_buffer->Lock(0, sound_output->secondary_buffer_size,
                                               &region1, &region1_size,
                                               &region2, &region2_size,
                                               0)))
    {
        u8* dest_sample = (u8*)region1;
        for (DWORD byte_index = 0; byte_index < region1_size; ++byte_index)
        {
            *dest_sample++ = 0;
        }
        
        dest_sample = (u8*)region2;
        for (DWORD byte_index = 0; byte_index < region2_size; ++byte_index)
        {
            *dest_sample++ = 0;
        }
        
        global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
    }
}

internal void
win32_fill_sound_buffer(win32_sound_output* sound_output, DWORD byte_to_lock, DWORD bytes_to_write, game_sound_output_buffer* source_buffer)
{
    VOID* region1;
    DWORD region1_size;
    VOID* region2;
    DWORD region2_size;
    if(SUCCEEDED(global_secondary_buffer->Lock(byte_to_lock, bytes_to_write,
                                               &region1, &region1_size,
                                               &region2, &region2_size,
                                               0)))
    {
        // TODO(Jon):  assert that region1size/region2size is valid
        i16* dest_sample = (i16*)region1;
        i16* source_sample = source_buffer->samples;
        DWORD region1_sample_count = region1_size / sound_output->bytes_per_sample;
        for (DWORD sample_index = 0; sample_index < region1_sample_count; ++sample_index)
        {
            *dest_sample++ = *source_sample++;
            *dest_sample++ = *source_sample++;
            ++sound_output->running_sample_index;
        }
        
        dest_sample = (i16*)region2;
        DWORD region2_sample_count = region2_size / sound_output->bytes_per_sample;
        for (DWORD sample_index = 0;
             sample_index < region2_sample_count; ++sample_index)
        {
            *dest_sample++ = *source_sample++;
            *dest_sample++ = *source_sample++;
            ++sound_output->running_sample_index;
        }
        global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
    }
}

internal void
process_xinput_digital_button(DWORD xinput_button_state, game_button_state_t* old_state, DWORD button_bit, game_button_state_t* new_state)
{
    new_state->ended_down = ((xinput_button_state & button_bit) == button_bit);
    new_state->half_transition_count = (old_state->ended_down != new_state->ended_down) ? 1 : 0;
}

int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prev_instance,
        LPSTR cmd_line, 
        int show_code)
{
    LARGE_INTEGER perf_counter_frequency_result;
    QueryPerformanceFrequency(&perf_counter_frequency_result);
    i64 perf_counter_frequency = perf_counter_frequency_result.QuadPart;
    
    win32_load_input();
    
    WNDCLASS window_class = {};
    
    win32_resize_dib_section(&global_back_buffer, 1280, 720);
    
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = win32_main_window_callback;
    window_class.hInstance = instance;
    //window_class->hIcon = ;
    window_class.lpszClassName = "Handmade classname";    
    
    if (RegisterClass(&window_class))
    {
        HWND window = CreateWindowExA(0,
                                      window_class.lpszClassName,
                                      "Handmade Hero",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      0,
                                      0,
                                      instance,
                                      0);
        if (window)
        {
            HDC device_context = GetDC(window);
            
            win32_sound_output sound_output = { };
            
            sound_output.sample_per_second = 48000;
            sound_output.running_sample_index = 0;
            sound_output.bytes_per_sample = sizeof(i16) * 2;
            sound_output.secondary_buffer_size = sound_output.sample_per_second * sound_output.bytes_per_sample;
            
            win32_init_dsound(window, sound_output.sample_per_second, sound_output.secondary_buffer_size);
            win32_clear_buffer(&sound_output);
            global_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);
            
            global_running = true;
            
            i16* samples = (i16*)VirtualAlloc(0, sound_output.secondary_buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
#if HANDMADE_INTERNAL
            LPVOID based_address = (LPVOID)Terabytes((u64)2);
#else
            LPVOID based_address = 0;
#endif
            game_memory_t game_memory = { };
            game_memory.permanent_storage_size = Megabytes(64);
            game_memory.transient_storage_size = Gigabytes((u64)4); // Intergral promotion
            
            u64 total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
            game_memory.permanent_storage = VirtualAlloc(based_address, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            game_memory.transient_storage = ((u8*)game_memory.permanent_storage + game_memory.permanent_storage_size);
            
            if (samples && game_memory.permanent_storage)
            {
                game_input_t input[2] = { };
                game_input_t* new_input = &input[0];
                game_input_t* old_input = &input[1];
                
                LARGE_INTEGER last_counter;
                QueryPerformanceCounter(&last_counter);
                i64 last_cycle_count =  __rdtsc();
                while (global_running)
                {
                    MSG msg;
                    
                    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                    {
                        if (msg.message == WM_QUIT)
                        {
                            global_running = false;
                        }
                        
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    
                    DWORD max_controller_count = XUSER_MAX_COUNT;
                    
                    if (max_controller_count > array_count(new_input->controllers))
                    {
                        max_controller_count = array_count(new_input->controllers);
                    }
                    
                    for (DWORD controller_index = 0; 
                         controller_index < max_controller_count;
                         ++controller_index)
                    {
                        game_controller_input_t* old_controller = &old_input->controllers[controller_index];
                        game_controller_input_t* new_controller = &new_input->controllers[controller_index];
                        
                        XINPUT_STATE controller_state;
                        if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS)
                        {
                            // NOTE(1337): this controller is plugged in
                            XINPUT_GAMEPAD* pad = &controller_state.Gamepad;
                            
                            // TODO(Jon): Dpad
                            bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                            bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                            bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                            bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                            
                            new_controller->is_analog = true;
                            new_controller->start_x = old_controller->end_x;
                            new_controller->start_y = old_controller->end_y;
                            
                            // TODO(Jon):  Min/Max Macros
                            f32 x;
                            if (pad->sThumbLX < 0)
                            {
                                x = (f32)pad->sThumbLX / 32768.0f;
                            }
                            else
                            {
                                x = (f32)pad->sThumbLX / 32767.0f;
                            }
                            new_controller->min_x = new_controller->max_x = new_controller->end_x = x;
                            
                            f32 y;
                            if (pad->sThumbLY < 0)
                            {
                                y= (f32)pad->sThumbLY / 32768.0f;
                            }
                            else
                            {
                                y = (f32)pad->sThumbLY / 32767.0f;
                            }
                            new_controller->min_y = new_controller->max_y = new_controller->end_y = y;
                            
                            process_xinput_digital_button(pad->wButtons, &old_controller->down, XINPUT_GAMEPAD_A, &new_controller->down);
                            process_xinput_digital_button(pad->wButtons, &old_controller->right, XINPUT_GAMEPAD_B, &new_controller->right);
                            process_xinput_digital_button(pad->wButtons, &old_controller->left, XINPUT_GAMEPAD_X, &new_controller->left);
                            process_xinput_digital_button(pad->wButtons, &old_controller->up, XINPUT_GAMEPAD_Y, &new_controller->up);
                            process_xinput_digital_button(pad->wButtons, &old_controller->left_shoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &new_controller->left_shoulder);
                            process_xinput_digital_button(pad->wButtons, &old_controller->right_shoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &new_controller->right_shoulder);
                            
                            //bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
                            //bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
                        }
                        else
                        {
                            // NOTE(1337): this controller is not available
                        }
                    }
                    
                    // NOTE(1337): DirectSound output test
                    DWORD byte_to_lock;
                    DWORD bytes_to_write;
                    DWORD target_cursor;
                    DWORD play_cursor;
                    DWORD write_cursor;
                    b32 sound_is_valid = false;
                    
                    // TODO(1337): Tighten up sound logic so that we know where we should be 
                    // Writing to and can anticipate the time spent in the game update.
                    
                    if (SUCCEEDED(global_secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor)))
                    {
                        byte_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;
                        
                        target_cursor = ((play_cursor + (sound_output.latency_sample_count*sound_output.bytes_per_sample)) %
                                         sound_output.secondary_buffer_size);
                        
                        
                        if (byte_to_lock > play_cursor)
                        {
                            bytes_to_write = (sound_output.secondary_buffer_size - byte_to_lock);
                            bytes_to_write += target_cursor;
                        }
                        else
                        {
                            bytes_to_write = target_cursor - byte_to_lock;
                        }
                        
                        sound_is_valid = true;
                    }
                    
                    
                    game_sound_output_buffer sound_buffer = { 0 };
                    sound_buffer.samples_per_second = sound_output.sample_per_second;
                    sound_buffer.sample_count = bytes_to_write / sound_output.bytes_per_sample;
                    sound_buffer.samples = samples;
                    
                    game_offscreen_buffer game_buffer = { 0 };
                    game_buffer.memory = global_back_buffer.memory;
                    game_buffer.width = global_back_buffer.width;
                    game_buffer.height = global_back_buffer.height;
                    game_buffer.pitch = global_back_buffer.pitch;
                    
                    game_update_and_render(&game_memory, input, &game_buffer, &sound_buffer);
                    
                    if (sound_is_valid)
                    {
                        win32_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write, &sound_buffer);
                    }
                    
                    win32_window_dimension dimension = get_window_dimension(window);
                    win32_disply_buffer_in_window(&global_back_buffer, device_context, dimension.width, dimension.height, 0, 0, dimension.width, dimension.height);
                    
                    i64 end_cycle_count =  __rdtsc();
                    
                    LARGE_INTEGER end_counter;
                    QueryPerformanceCounter(&end_counter);
                    
                    i64 cycles_elapsed = end_cycle_count - last_cycle_count;
                    i64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
                    i32 ms_per_frame  = (i32)((1000 * counter_elapsed) / perf_counter_frequency);
                    i32 fps = perf_counter_frequency / counter_elapsed;
                    i32 mcpf = (i32)(cycles_elapsed / ( 1000 * 1000));
                    
                    char buffer[256];
                    wsprintf(buffer, "%dms/f  %dfps %dmc/f\n", ms_per_frame, fps, mcpf);
                    OutputDebugStringA(buffer);
                    
                    last_counter = end_counter;
                    last_cycle_count = end_cycle_count;
                    
                    game_input_t* temp = new_input;
                    new_input = old_input;
                    old_input = temp;
                    // TODO(Jon): should I clear these?
                }
                ReleaseDC(window, device_context);
            }
            else
            {
                // NOTE(Jon): Logging
            }
        }
        else
        {
            // TODO(1337): 
        }
    }
    else
    {
        // TODO(1337): 
    }
    
    return 0;
}
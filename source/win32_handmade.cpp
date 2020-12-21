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


#include <Windows.h>
#include <xinput.h>
#include <dsound.h>

// TODO(1337): implement sine ourselves
#include <math.h>

#include "include/language_layer.h"

#define pi_32 3.1415926535f

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

// NOTE(1337): a global for now
global_variable bool running; 
global_variable win32_offscreen_buffer global_back_buffer;
global_variable LPDIRECTSOUNDBUFFER global_secondary_buffer;
4
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
render_weird_gradient(win32_offscreen_buffer* buffer, int x_offset, int y_offset)
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
    
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
    
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
            running = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;
        
        case WM_DESTROY:
        {
            running = false;
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
                running = false;
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


struct win32_sound_output 
{
    // NOTE(1337): for sound test
    int sample_per_second;
    int hz ;
    i16 tone_volume;
    u32 running_sample_index;
    int wave_period;
    int bytes_per_sample;
    int secondary_buffer_size;
};

internal void
win32_fill_sound_buffer(win32_sound_output* sound_output, DWORD byte_to_lock, DWORD bytes_to_write)
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
        // TODO(1337):  assert that region1size/region2size is valid
        i16* sample_out = (i16*)region1;
        DWORD region1_sample_count = region1_size / sound_output->bytes_per_sample;
        for (DWORD sample_index = 0; sample_index < region1_sample_count; ++sample_index)
        {
            f32 t =  2.0f * pi_32 *(f32) sound_output->running_sample_index / (f32) sound_output->wave_period;
            f32 sine_value = sinf(t);
            i16 sample_value = (i16)(sine_value *  sound_output->tone_volume);
            *sample_out++ = sample_value;
            *sample_out++ = sample_value;
            
            ++ sound_output->running_sample_index;
        }
        
        sample_out = (i16*)region2;
        DWORD region2_sample_count = region2_size / sound_output->bytes_per_sample;
        for (DWORD sample_index = 0;
             sample_index < region2_sample_count; ++sample_index)
        {
            f32 t =  2.0f * pi_32 *(f32) sound_output->running_sample_index / (f32) sound_output->wave_period;
            f32 sine_value = sinf(t);
            i16 sample_value = (i16)(sine_value *  sound_output->tone_volume);
            *sample_out++ = sample_value;
            *sample_out++ = sample_value;
            
            ++ sound_output->running_sample_index;
        }
        global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
    }
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
        HWND window_handle = CreateWindowExA(0,
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
        if (window_handle)
        {
            HDC device_context = GetDC(window_handle);
            
            // NOTE(1337): for graphics test
            int x = 0;
            int y = 0;
            
            win32_sound_output sound_output = {};
            
            sound_output.sample_per_second = 48000;
            sound_output.hz = 256;
            sound_output.tone_volume = 6000;
            sound_output.running_sample_index = 0;
            sound_output.wave_period = sound_output.sample_per_second / sound_output.hz;
            sound_output.bytes_per_sample = sizeof(i16) * 2;
            sound_output.secondary_buffer_size = sound_output.sample_per_second * sound_output.bytes_per_sample;
            
            win32_init_dsound(window_handle, sound_output.sample_per_second, sound_output.secondary_buffer_size);
            win32_fill_sound_buffer(&sound_output, 0, sound_output.secondary_buffer_size);
            global_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);
            
            //bool sound_is_playing = true;
            
            LARGE_INTEGER last_counter;
            QueryPerformanceCounter(&last_counter);
            
            i64 last_cycle_count =  __rdtsc();
            
            running = true;
            while (running)
            {
                MSG msg;
                while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                    {
                        running = false;
                    }
                    
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                
                for (DWORD controller_index = 0; 
                     controller_index < XUSER_MAX_COUNT;
                     ++controller_index)
                {
                    XINPUT_STATE controller_state;
                    
                    if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS)
                    {
                        // NOTE(1337): this controller is plugged in
                        XINPUT_GAMEPAD* pad = &controller_state.Gamepad;
                        
                        bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
                        bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool left_shoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool right_shoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool a_button = (pad->wButtons & XINPUT_GAMEPAD_A);
                        bool b_button = (pad->wButtons & XINPUT_GAMEPAD_B);
                        bool x_button = (pad->wButtons & XINPUT_GAMEPAD_X);
                        bool y_button = (pad->wButtons & XINPUT_GAMEPAD_Y);
                        
                        i16 stick_x = pad->sThumbLX;
                        i16 stick_y = pad->sThumbLY;
                        
                        if (a_button)
                        {
                            y += 2;
                        }
                    }
                    else
                    {
                        // NOTE(1337): this controller is not available
                    }
                    
                }
                
                render_weird_gradient(&global_back_buffer, x, y);
                
                // NOTE(1337): DirectSound output test
                DWORD play_cursor;
                DWORD write_cursor;
                if (SUCCEEDED(global_secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor))) 
                {
                    DWORD byte_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;
                    DWORD bytes_to_write;
                    if (byte_to_lock == play_cursor)
                    {
                        bytes_to_write = 0;
                    }
                    else if (byte_to_lock > play_cursor)
                    {
                        bytes_to_write = (sound_output.secondary_buffer_size - byte_to_lock);
                        bytes_to_write += play_cursor;
                    }
                    else
                    {
                        bytes_to_write = play_cursor - byte_to_lock;
                    }
                    
                    win32_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write);
                }
                
                win32_window_dimension dimension = get_window_dimension(window_handle);
                win32_disply_buffer_in_window(&global_back_buffer, device_context, dimension.width, dimension.height, 0, 0, dimension.width, dimension.height);
                ++x;
                
                LARGE_INTEGER end_counter;
                QueryPerformanceCounter(&end_counter);
                
                i64 end_cycle_count =  __rdtsc();
                
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
            }
            ReleaseDC(window_handle, device_context);
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
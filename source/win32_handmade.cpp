#include <Windows.h>
#include <xinput.h>

#include "include/language_layer.h"

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

// NOTE(1337):  XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return 0;
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;

// NOTE(1337): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return 0;
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void
win32_load_input(void)
{
    HMODULE x_input_library = LoadLibraryA("xinput1_3.dll");
    
    if (x_input_library)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(x_input_library, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(x_input_library, "XInputSetState");
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
            OutputDebugStringA("WM_SIZE");
        } break;
        
        case WM_CLOSE:
        {
            running = false;
            OutputDebugStringA("WM_CLOSE");
        } break;
        
        case WM_DESTROY:
        {
            running = false;
            OutputDebugStringA("WM_DESTROY");
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
            
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP");
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
            win32_disply_buffer_in_window(global_back_buffer, device_context, 
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

int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prev_instance,
        LPSTR cmd_line, 
        int show_code)
{
    WNDCLASS window_class = {};
    
    win32_load_input();
    
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
            running = true;
            
            int x = 0;
            int y = 0;
            
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
                
                HDC device_context = GetDC(window_handle);
                
                win32_window_dimension dimension = get_window_dimension(window_handle);
                
                win32_disply_buffer_in_window(&global_back_buffer, device_context, dimension.width, dimension.height, 0, 0, dimension.width, dimension.height);
                
                ReleaseDC(window_handle, device_context);
                ++x;
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